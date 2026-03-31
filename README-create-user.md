# Creating a Certificate for an Employee

This example walks through issuing an MTC certificate for an employee, Joe Bosfitch, at a fictional company AJAX, Inc.

## 1. Start the CA/Log server

```bash
cd server
python3 server.py
```

## 2. Bootstrap trust (first time only)

```bash
cd client
python3 main.py bootstrap
```

This fetches the CA's Ed25519 public key and stores it in the local trust store. In production, this would be distributed out-of-band (e.g. shipped with the OS or installed by IT).

## 3. Enroll Joe

```bash
python3 main.py enroll "urn:ajax-inc:employee:joe.bosfitch" --algorithm EC-P256
```

This generates a key pair for Joe, sends the public key to your CA, and gets back a standalone MTC certificate. The private key stays local.

## 4. Or via the API directly

For integration with an HR system or automated provisioning:

```bash
curl -X POST http://localhost:8443/certificate/request \
  -H 'Content-Type: application/json' \
  -d '{
    "subject": "urn:ajax-inc:employee:joe.bosfitch",
    "public_key_pem": "<Joe's public key PEM here>",
    "key_algorithm": "EC-P256",
    "validity_days": 90,
    "extensions": {
      "organization": "AJAX Inc.",
      "role": "employee",
      "department": "Engineering",
      "employee_id": "EMP-4821"
    }
  }'
```

The response gives Joe a **standalone certificate** containing:

- His identity (`urn:ajax-inc:employee:joe.bosfitch`)
- An inclusion proof into the company's Merkle tree log
- A cosignature from the CA's Ed25519 key
- A trust anchor ID (`32473.1`)

If a landmark is available, Joe also receives a **landmark certificate** — smaller, with no signatures, usable by relying parties that have cached the landmark subtree hash.

## 5. Verify Joe's certificate

```bash
python3 main.py verify <index>
```

Any relying party (VPN gateway, internal service, etc.) that trusts the CA's public key can verify Joe's certificate offline — no live call to the CA needed during verification.

## What happens under the hood

1. The CA validates the request
2. A `TBSCertificateLogEntry` is created with Joe's subject, public key hash, validity period, and extensions
3. The entry is appended to the issuance log (Merkle tree)
4. A checkpoint is created and the CA cosigns the subtree
5. An inclusion proof is generated from Joe's entry to the subtree root
6. The standalone certificate is assembled: TBS data + inclusion proof + cosignatures
7. Everything is persisted to the PostgreSQL database

## Production considerations

In a real deployment:

- Joe's private key would live in a **TPM, secure enclave, or hardware token** on his device
- Enrollment would be triggered through an **HR or identity management system**, not manually
- The subject identifier should be tied to a **device or app instance** rather than a human name, with the human identity in extensions
- Servers requesting Joe's identity would send a TLS `CertificateRequest` and advertise acceptable trust anchor IDs
- Joe's client would select the smallest compatible certificate form (landmark if the server supports it, standalone otherwise)
- Certificates should be **short-lived** (e.g. 47-90 days) with automated renewal

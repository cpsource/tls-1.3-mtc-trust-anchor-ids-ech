If “Bob” is a **user/client identity** in a system using **TLS 1.3 + MTC + Trust Anchor IDs + ECH**, you do **not** create his certificate the same way as a classic X.509 leaf and stop there. The cleaner mental model is:

* **ECH** hides parts of `ClientHello`; it does **not** change how Bob’s identity is issued. ([IETF Datatracker][1])
* **Trust Anchor IDs** help the relying party and server/client negotiate which trust anchors or certification paths are acceptable. ([IETF Datatracker][2])
* **MTC** changes how the certificate is represented and validated, especially for standalone vs. landmark certificates, and the current design is still an **IETF Internet-Draft**, not a finished RFC. ([IETF Datatracker][3])

So, in practical terms, creating “Bob’s certificate” would look like this.

## 1. Decide what Bob is

First decide whether Bob is:

* a **human user** authenticating with client certificates,
* a **device** Bob owns,
* or a **service account** acting on Bob’s behalf.

That matters because the certificate subject and authorization model should identify the right thing. In a modern design, I would usually bind the cert to a **device or app instance** and keep Bob’s human identity in an extension or in the authorization layer, rather than stuffing too much human meaning into the subject DN.

## 2. Generate Bob’s key pair

Bob needs a key pair first. That private key should ideally live in:

* a TPM,
* a smart card,
* a secure enclave,
* or at least an OS-protected keystore.

MTC does **not** remove the need for Bob to prove possession of the private key during TLS. Even with MTC, TLS still relies on the handshake’s proof-of-possession signature step; MTC mainly changes the trust packaging around the certificate. That follows from TLS 1.3’s certificate/authentication structure and the MTC draft’s integration into TLS rather than replacing TLS authentication entirely. ([IETF Datatracker][3])

## 3. Create Bob’s identity record in your CA

Your CA or registration service needs a record for Bob, for example:

* user ID: `bob`
* allowed roles: `employee`, `vpn-user`
* allowed device IDs: `laptop-017`
* validity policy
* revocation / suspension policy

At this stage you are deciding **who is allowed to get a cert**, not yet building the final certificate bytes.

## 4. Build the certificate contents

For Bob’s MTC leaf, I would include at least:

* Bob’s public key
* a subject or SAN that uniquely identifies Bob or Bob’s device
* key usage / extended key usage suitable for **client authentication**
* validity period
* any authorization-relevant extensions you need

In MTC, the public key remains in the certificate, while trust evidence is shifted into the MTC structure and log integration rather than the old “just a CA signature on a leaf plus CT bolted on afterward” model. ([Google Groups][4])

For a private deployment, keep Bob’s identity compact and explicit. For example, use a URI SAN like:

```text
urn:example:user:bob
```

or a device-centric identity like:

```text
urn:example:device:laptop-017
```

## 5. Submit Bob’s certificate data to the issuance log

With MTC, issuance is tied to an **issuance log**. The current draft says an ACME server moves the order to `valid` once the corresponding entry is **sequenced in the issuance log**, and then serves the standalone certificate. ([IETF Datatracker][3])

Conceptually, that means:

1. Your CA accepts Bob’s enrollment request.
2. It prepares Bob’s leaf data.
3. It inserts or sequences Bob’s entry into the issuance log.
4. Only then does it produce Bob’s MTC certificate object.

## 6. Issue Bob’s **standalone** certificate first

The MTC draft currently says the ACME flow serves the **standalone certificate** first, and may later expose a **landmark certificate** at an alternate URL once the next landmark is available. ([IETF Datatracker][3])

So Bob initially gets a **standalone MTC certificate**. This is the safest first form because it is broadly acceptable to relying parties that trust the issuing CA/log. The draft says a standalone certificate generally will be accepted by relying parties that trust the issuing CA, and that parties should use the `trust_anchors` extension to determine whether it would be acceptable. ([IETF Datatracker][3])

In plain English: Bob’s first usable cert should usually be the standalone one.

## 7. Optionally issue Bob’s **landmark** certificate later

If your clients are kept up to date with trusted landmark subtrees, Bob can later use a **landmark certificate**, which is expected to be smaller. The draft says relying parties configured with trusted subtrees should advertise the highest supported landmark in `trust_anchors`, and that when both landmark and standalone are supported, the authenticating party should prefer the landmark certificate. ([IETF Datatracker][3])

So Bob may end up with two usable representations of the same identity:

* standalone MTC for broad compatibility
* landmark MTC for smaller handshakes when the peer supports it

## 8. Assign the right Trust Anchor ID metadata

This is where **Trust Anchor IDs** enter.

The trust-anchor-ids draft defines a TLS extension for relying parties to convey trusted certification authorities more compactly than the traditional `certificate_authorities` extension, and it also supports a mode where servers describe available certification paths and clients select from them. Servers may also advertise trust anchor IDs in DNS via SVCB/HTTPS. ([IETF Datatracker][2])

For MTC specifically:

* a **standalone certificate** has the trust anchor ID of the corresponding **log ID**
* a **landmark certificate** has a trust anchor ID derived from the base ID plus the landmark number
* landmark certs may also carry additional trust anchor ranges so later landmarks can imply trust in earlier ones. ([IETF Datatracker][3])

So for Bob, your CA has to provision:

* Bob’s cert bytes
* Bob’s trust anchor ID metadata
* possibly additional trust anchor ranges if Bob’s cert is landmark-based

## 9. Install Bob’s certificate and private key in the client

Now Bob’s client device gets:

* Bob’s private key
* Bob’s standalone MTC cert
* optionally Bob’s landmark MTC cert
* any local config telling it which trust anchors and landmark subtrees it trusts

At this point Bob can authenticate as a TLS client.

## 10. Configure the server to request client auth

For Bob to use the cert, the server must send a TLS `CertificateRequest` and the server-side policy must accept Bob’s issuing authority / Trust Anchor ID.

That is where Trust Anchor IDs help: the relying party can signal which trust anchors it accepts in `ClientHello` or `CertificateRequest`, and the authenticating party should choose a certification path whose trust anchor ID matches or falls within an advertised acceptable range. ([IETF Datatracker][2])

In your case, that means:

* server requests client certificate
* server advertises acceptable trust anchors
* Bob’s client chooses standalone or landmark MTC depending on compatibility
* TLS continues with proof of private-key possession

## 11. ECH is orthogonal

If this connection also uses ECH, that only affects the privacy of the **server-facing ClientHello metadata**, such as SNI and ALPN. RFC 9849 says ECH encrypts the `ClientHello` to protect SNI and other sensitive fields, and RFC 9848 says the client learns ECH configuration through DNS SVCB/HTTPS records. ([IETF Datatracker][1])

For Bob’s certificate issuance, ECH changes nothing important. It is a transport privacy feature, not a certificate-enrollment feature.

## 12. A concrete “Bob” workflow

Here is the compact version of how I would do it:

1. Generate Bob’s key pair on Bob’s device.
2. Enroll Bob with your CA/RA and verify his identity/device.
3. Create Bob’s certificate contents:

   * public key
   * SAN = `urn:example:user:bob`
   * EKU = clientAuth
   * short validity
4. Submit Bob’s entry to the MTC issuance log.
5. Once sequenced, issue Bob’s **standalone MTC**.
6. Assign Bob’s standalone **trust anchor ID** from the corresponding log ID. ([IETF Datatracker][3])
7. Optionally, once landmarks are available, issue Bob’s **landmark MTC** too, with the landmark-derived trust anchor ID and range metadata. ([IETF Datatracker][3])
8. Install the cert(s) and key on Bob’s device.
9. Configure servers to request client auth and advertise accepted trust anchors.
10. Let Bob’s client choose the smallest compatible cert form at handshake time.

## 13. What I would actually recommend for you

Because MTC and Trust Anchor IDs are still drafts, I would not build a production user-PKI around them yet unless you are deliberately experimenting. Today, the sane architecture is:

* classic private X.509 for actual deployment,
* short-lived client certs,
* automated renewal,
* hardware-protected keys where possible,
* and a design that could later swap the certificate representation to MTC if the drafts stabilize. ([IETF Datatracker][3])

So the honest answer is:

> To create “Bob’s certificate” in a TLS 1.3 + MTC + Trust Anchor IDs + ECH world, you would issue Bob an MTC leaf through the issuance log, usually obtain a standalone MTC first, optionally a landmark MTC later, attach the correct Trust Anchor ID metadata, and then use normal TLS client-auth with private-key proof of possession. ECH is separate and only protects the ClientHello.

If you want, I can sketch a concrete **private-PKI version** of Bob’s certificate in pseudo-ASN.1 / JSON terms, or show the exact handshake sequence for Bob authenticating to a server.

[1]: https://datatracker.ietf.org/doc/rfc9849/ "
            
        RFC 9849 - TLS Encrypted Client Hello

        "
[2]: https://datatracker.ietf.org/doc/draft-ietf-tls-trust-anchor-ids/ "
            
    
        draft-ietf-tls-trust-anchor-ids-03 - TLS Trust Anchor Identifiers
    

        "
[3]: https://datatracker.ietf.org/doc/draft-ietf-plants-merkle-tree-certs/ "
            
    
        draft-ietf-plants-merkle-tree-certs-02 - Merkle Tree Certificates
    

        "
[4]: https://groups.google.com/a/chromium.org/g/ct-policy/c/47gNi0rlClg/m/FrcqfuVVAwAJ?utm_source=chatgpt.com "Merkle Tree Certificates (a.k.a. Photosynthesis)"

---

# 13. Reference Implementation: `server/`

A working MTC CA/Log server lives in the `server/` directory. It implements the core architecture from draft-ietf-plants-merkle-tree-certs-02 in Python, with all state persisted to a Neon PostgreSQL database.

## Components

| File | Role |
|---|---|
| `merkle.py` | Merkle Tree (RFC 9162 / MTC Section 4): append-only tree, SHA-256 leaf/node hashing, inclusion proofs, consistency proofs, verification |
| `ca.py` | Certificate Authority + Issuance Log (MTC Sections 5-6): entry management, Ed25519 CA cosigning, standalone and landmark certificate issuance, trust anchor IDs |
| `db.py` | PostgreSQL persistence layer: reads `MERKLE_NEON` from `~/.env`, stores entries/checkpoints/landmarks/certificates/CA key in Neon |
| `server.py` | HTTP REST API (stdlib `http.server`, no framework dependencies) |
| `client_demo.py` | Demo client exercising the full workflow |

## Database Schema (Neon PostgreSQL)

| Table | Contents |
|---|---|
| `mtc_log_entries` | Every log entry: index, type, TBS data (JSONB), serialized bytes, leaf hash |
| `mtc_checkpoints` | Checkpoint snapshots: tree size, root hash, timestamp |
| `mtc_landmarks` | Landmark tree sizes |
| `mtc_certificates` | Issued certificates (full JSON including proofs and cosignatures) |
| `mtc_ca_config` | CA private key (Ed25519 PEM) and other config |

On startup, the Merkle tree is rebuilt in memory by replaying all stored entries from the database. This is deterministic since the log is append-only.

## API Endpoints

| Method | Path | Description |
|---|---|---|
| GET | `/` | Server info and endpoint listing |
| GET | `/log` | Current log state (tree size, root hash, landmarks, checkpoints) |
| GET | `/log/entry/<index>` | Get a specific log entry |
| GET | `/log/proof/<index>` | Generate and verify an inclusion proof |
| GET | `/log/checkpoint` | Latest checkpoint |
| GET | `/log/consistency?old=N&new=M` | Consistency proof between tree sizes |
| POST | `/certificate/request` | Request a new MTC certificate |
| GET | `/certificate/<index>` | Retrieve an issued certificate |
| GET | `/trust-anchors` | List all trust anchor IDs (standalone + landmark) |
| GET | `/ca/public-key` | CA's Ed25519 public key for cosignature verification |

## Certificate Types

The server issues two certificate forms per the MTC draft:

- **Standalone certificate**: includes an inclusion proof to a cosigned subtree. Usable immediately by any relying party that trusts the CA cosigner. Trust anchor ID = log ID.
- **Landmark certificate**: includes an inclusion proof to a landmark subtree (allocated every 16 entries). No signatures needed -- trust comes from predistributed landmark hashes. Trust anchor ID = log ID + landmark number.

## Running

```bash
cd server
python3 server.py                # starts on :8443, connects to Neon
python3 client_demo.py           # in another terminal
```

Requires: Python 3.12+, `cryptography`, `psycopg2-binary`. Database connection string is read from `MERKLE_NEON` in `~/.env`.

## What This Demonstrates

This implementation shows the core MTC architecture is buildable without massive infrastructure:

- An append-only issuance log backed by a Merkle tree
- CA cosigning with Ed25519 (domain-separated per MTC Section 5.4.1)
- Standalone and landmark certificate issuance
- Inclusion and consistency proofs that verify correctly
- Trust anchor ID assignment per the draft
- Full persistence across server restarts via PostgreSQL
- The tree is rebuilt deterministically from stored entries on startup

It intentionally does **not** implement: external cosigners, ACME integration, DER/ASN.1 encoding, TLS handshake integration, or log pruning. Those are the next layers of complexity described in the sections above.


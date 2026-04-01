# MTC Public API

User-facing API for Merkle Tree Certificate operations. Available in Python and C with identical interfaces. Applications use this API instead of dealing with proofs, cosignatures, or trust stores directly.

## API Functions

| Function | Purpose |
|---|---|
| `MTC_Connect` | Connect to a CA/Log server and bootstrap trust |
| `MTC_Enroll` | Generate a key pair, request a certificate, store in `~/.TPM` |
| `MTC_Verify` | Verify a certificate (inclusion proof + cosignature + expiry) |
| `MTC_Find` | Search certificates by subject (case-insensitive substring) |
| `MTC_List` | List all local certificates in `~/.TPM` |
| `MTC_Status` | Get server and trust store status |
| `MTC_Renew` | Renew a certificate with fresh keys, archive the old one |
| `MTC_Revoke` | Request revocation of a certificate |

## Python API

### Setup

No build step needed. The Python API imports directly from the `client/` directory.

### Usage

```python
from mtc import MTC_Connect, MTC_Enroll, MTC_Verify
from mtc import MTC_Find, MTC_List, MTC_Status, MTC_Renew, MTC_Revoke

# Connect to the CA/Log server
conn = MTC_Connect("http://localhost:8443")

# Enroll a new identity
cert = MTC_Enroll(conn, "urn:ajax-inc:app:myapp",
                  extensions={"human_id": "Cal Page",
                              "app_instance": "myapp"})
print(f"Issued: index #{cert.index}, stored in {cert.local_path}")

# Verify a certificate
result = MTC_Verify(conn, index=cert.index)
print(f"Valid: {result.valid}")
print(f"  Inclusion proof: {result.inclusion_proof}")
print(f"  Cosignature:     {result.cosignature_valid}")
print(f"  Not expired:     {result.not_expired}")

# Search by subject
matches = MTC_Find(conn, "myapp")
for m in matches:
    print(f"  #{m['index']}  {m['subject']}")

# List local certificates in ~/.TPM
for cert in MTC_List():
    print(f"  #{cert.index}  {cert.subject}  ({cert.local_path})")

# Check server status
status = MTC_Status(conn)
print(f"Tree size: {status['tree_size']}, Landmarks: {status['landmarks']}")

# Renew before expiry
new_cert = MTC_Renew(conn, index=cert.index)
print(f"Renewed: index #{new_cert.index}")

# Request revocation
result = MTC_Revoke(conn, index=42)
print(f"Status: {result['status']}")
```

### Return types

| Function | Returns |
|---|---|
| `MTC_Connect` | `MTCConnection` dataclass |
| `MTC_Enroll` | `MTCCertificate` dataclass |
| `MTC_Verify` | `MTCVerifyResult` dataclass |
| `MTC_Find` | `list[dict]` with `index` and `subject` keys |
| `MTC_List` | `list[MTCCertificate]` |
| `MTC_Status` | `dict` with server info, log state, trust store summary |
| `MTC_Renew` | `MTCCertificate` dataclass (the new cert) |
| `MTC_Revoke` | `dict` with status |

## C API

### Dependencies

- **libcurl** — HTTP client
- **json-c** — JSON parsing
- **OpenSSL** — key generation and crypto

On Ubuntu/Debian:

```bash
sudo apt-get install libcurl4-openssl-dev libjson-c-dev libssl-dev
```

### Building

```bash
cd api/c
autoreconf --install
./configure
make
```

This produces:
- `libmtc.a` — static library to link against
- `mtc.h` — header to include
- `mtc-example` — example program

### Linking your application

```bash
gcc myapp.c -o myapp -lmtc -lcurl -ljson-c -lssl -lcrypto
```

Or with pkg-config after install:

```bash
gcc myapp.c -o myapp $(pkg-config --cflags --libs libcurl json-c openssl) -lmtc
```

### Usage

```c
#include "mtc.h"

int main(void) {
    /* Connect */
    mtc_conn_t *conn = MTC_Connect("http://localhost:8443");
    if (!conn) {
        fprintf(stderr, "Failed: %s\n", MTC_Last_Error());
        return 1;
    }

    /* Enroll with extensions */
    mtc_extensions_t *ext = MTC_Extensions_New();
    MTC_Extensions_Add(ext, "human_id", "Cal Page");
    MTC_Extensions_Add(ext, "app_instance", "myapp");

    mtc_cert_t *cert = MTC_Enroll(conn,
        "urn:ajax-inc:app:myapp",   /* subject */
        "EC-P256",                   /* algorithm */
        90,                          /* validity days */
        ext);
    MTC_Free_Extensions(ext);

    printf("Issued: index #%d\n", cert->index);
    printf("Subject: %s\n", cert->subject);
    printf("Stored in: %s\n", cert->local_path);

    /* Verify */
    mtc_verify_t *v = MTC_Verify(conn, cert->index);
    printf("Valid: %s\n", v->valid ? "yes" : "no");
    printf("  Inclusion proof: %s\n", v->inclusion_proof ? "PASS" : "FAIL");
    printf("  Cosignature:     %s\n", v->cosignature_valid ? "PASS" : "FAIL");
    printf("  Not expired:     %s\n", v->not_expired ? "PASS" : "FAIL");
    MTC_Free_Verify(v);

    /* Find */
    mtc_find_results_t *found = MTC_Find(conn, "myapp");
    for (int i = 0; i < found->count; i++)
        printf("  #%d  %s\n", found->results[i].index,
               found->results[i].subject);
    MTC_Free_Find(found);

    /* Status */
    mtc_status_t *s = MTC_Status(conn);
    printf("Tree size: %d, Landmarks: %d\n",
           s->tree_size, s->landmark_count);
    MTC_Free_Status(s);

    /* Renew */
    mtc_cert_t *renewed = MTC_Renew(conn, cert->index, 90);
    printf("Renewed: index #%d\n", renewed->index);
    MTC_Free_Cert(renewed);

    /* Cleanup */
    MTC_Free_Cert(cert);
    MTC_Disconnect(conn);
    return 0;
}
```

### Memory management

The C API allocates memory for return values. The caller must free them:

| Allocated by | Free with |
|---|---|
| `MTC_Enroll`, `MTC_Renew` | `MTC_Free_Cert()` |
| `MTC_Verify` | `MTC_Free_Verify()` |
| `MTC_Find` | `MTC_Free_Find()` |
| `MTC_Status` | `MTC_Free_Status()` |
| `MTC_Extensions_New` | `MTC_Free_Extensions()` |
| `MTC_Connect` | `MTC_Disconnect()` |

Use `MTC_Last_Error()` to get the last error message when a function returns NULL.

### Connection struct

The `mtc_conn_t` struct is opaque. Use accessors to read connection state:

```c
const char *MTC_Conn_CA_Name(const mtc_conn_t *conn);
const char *MTC_Conn_Log_ID(const mtc_conn_t *conn);
int         MTC_Conn_Tree_Size(const mtc_conn_t *conn);
```

## Key storage

Both APIs store keys and certificates in `~/.TPM/<subject>/`:

```
~/.TPM/urn_ajax-inc_app_myapp/
├── private_key.pem      # mode 600, owner-only
├── public_key.pem
├── certificate.json     # full MTC cert with proofs and cosignatures
└── index                # log entry index for quick lookup
```

On renewal, old files are archived with a `.old` suffix before new ones are written.

## Relationship to client/ and server/

```
server/    ← CA/Log internals (issuance log, Merkle tree, cosigning)
client/    ← relying party internals (proof verification, trust store)
api/       ← user-facing API wrapping both into clean calls
  python/  ←   Python: import mtc
  c/       ←   C: #include "mtc.h", link with -lmtc
```

The API layer is what applications should use. The `client/` and `server/` directories contain the implementation details.

# Certificate Revocation in MTC

MTC handles revocation fundamentally differently from traditional CRLs.

## Traditional CRL/OCSP

- CA publishes a list of revoked certificate serial numbers
- Clients download the list (CRL) or query online (OCSP) before trusting a cert
- Scales badly — the list grows forever, every client must fetch it

## MTC: Revocation by Index (Draft Section 7.5)

MTC doesn't use CRLs at all. Instead, each certificate has an **index** in the append-only log (like our `index #59`). Revocation is just a **list of revoked index ranges** maintained by the relying party:

```
Revoked: [0, 15)        <- expired/pruned entries
Revoked: [42, 43)       <- one compromised cert
Revoked: [100, 200)     <- CA misbehaved during this period
```

The client checks: is this cert's index in a revoked range? If yes, reject. That's it.

## Why this is better

- **Tiny** — a handful of integer ranges vs thousands of serial numbers
- **No online check** — baked into the client's trust store, updated in the background
- **Handles CA misbehavior** — you can revoke a whole range of indices if a CA is caught cheating, without knowing what's in those entries
- **Pruning integration** — as old entries expire and the log is pruned, those indices are automatically revoked so nobody can replay them

## How the draft specifies it

From Section 7.5:

> For each supported Merkle Tree CA, the relying party maintains a list of revoked ranges of indices. This allows a relying party to efficiently revoke entries of an issuance log, even if the contents are not necessarily known.

When a relying party first trusts a CA, it should revoke all entries from zero up to the first available unexpired certificate. As entries expire and logs are pruned, the revoked range is extended. This prevents replay of old or unavailable entries.

## Use cases

| Scenario | Action |
|---|---|
| Certificate compromised | Revoke that single index: `[42, 43)` |
| Log entries pruned | Extend revoked range to cover pruned indices |
| CA caught misbehaving | Revoke the range of suspect indices |
| CA fully distrusted | Remove the CA entirely, or revoke all indices after some point |

## For our implementation

Revocation would be a list of `(start, end)` ranges stored in the client's trust store. The verify step adds one check:

```python
if any(start <= index < end for start, end in revoked_ranges):
    reject
```

The revoked ranges would be updated in the background alongside landmark fetches — part of the same periodic trust store update that relying parties already need to do.

## Who revokes?

The client never revokes their own certificate — that would be like tearing up your own ID badge. The **relying party** (the server/service verifying the cert) maintains the revoked ranges. Those ranges come from a trusted authority.

```
CA or security team: "indices [42, 43) are revoked"
        |
        v
    Update channel (browser update, config push, API)
        |
        v
    Relying party's trust store gets updated revoked ranges
        |
        v
    Next time cert #42 is presented: rejected
```

The client (Joe with his cert in `~/.TPM`) has no say in this. The revocation decision comes from:

- **The CA** — "we misissued this" or "key compromise reported"
- **The organization's security team** — "Joe was terminated" or "this device was lost"
- **Policy automation** — "these entries were pruned from the log, revoke them automatically"

The client still has the cert locally. It still looks valid in `~/.TPM`. But every relying party that has received the revocation update will reject it.

## Comparison

| | Traditional CRL | OCSP | MTC Revocation by Index |
|---|---|---|---|
| Data format | List of serial numbers | Online query/response | List of integer ranges |
| Size | Grows with revocations | Per-query | Tiny (a few ranges) |
| Online check needed | Download periodically | Yes, per cert | No |
| Handles unknown entries | No | No | Yes (revoke by range) |
| Handles CA distrust | Remove CA entirely | Remove CA entirely | Revoke range, keep valid entries |

## How the server identifies callers

By default, every HTTP request to the server looks the same — the server can't tell a CA cosigner from a random attacker. The server needs a way to authenticate **who** is making each request.

### Who talks to the server

| Caller | What they want | Access level |
|---|---|---|
| **Authenticating party** (client/employee) | Request a certificate | Write (dangerous) |
| **Cosigner** | Sign subtrees after verifying the log | Write (privileged) |
| **Monitor** | Read the log to watch for misissued certs | Read-only (safe) |
| **Relying party** | Fetch log state, landmarks, proofs | Read-only (safe) |

Read-only endpoints (`/log`, `/trust-anchors`, `/certificate/<n>`) are fine to leave open — the log is meant to be public. The dangerous endpoint is `POST /certificate/request`.

### mTLS — the standard answer

Mutual TLS requires the caller to present a client certificate during the TLS handshake. The server checks:

1. Is this certificate signed by a CA I trust?
2. Is the subject authorized to request certificates?
3. What role does this caller have? (cosigner vs enrollment agent vs monitor)

For cosigners specifically, the MTC draft identifies them by their **cosigner ID** (a trust anchor ID) and **public key**. The server verifies the cosigner's identity by checking their TLS client cert or by verifying signatures on the messages they send.

### Authentication methods by robustness

| Method | Protects | How |
|---|---|---|
| **API tokens** | `/certificate/request` | Bearer token in header, different tokens per role |
| **mTLS** | All endpoints | Caller presents a client cert, server checks against an allowlist |
| **Signed requests** | `/certificate/request` | Caller signs the request body with their registered key, server verifies |
| **ACME account keys** | `/certificate/request` | Per RFC 8555, each ACME account has a key pair — requests are signed with the account key |

### The bootstrap problem

The irony: to secure the CA, you need certificates. To get certificates, you need the CA.

Typically solved by having a **separate, pre-existing PKI** (even a simple self-signed CA) just for server-to-server authentication, distinct from the MTC certificates being issued to end users. The infrastructure certificates that protect the CA are not the same as the user/app certificates the CA issues.

# Controlling Who Can Get Certificates

Right now, anyone who can reach the server on port 8443 can request a certificate. In production, the CA must validate requests before adding them to the issuance log.

## What the spec says

The MTC draft (Section 9) says certificate requests should go through **ACME** (RFC 8555) — the same protocol Let's Encrypt uses. The CA validates the request before adding it to the log.

The validation depends on what the subject represents.

### For domains (like example.com)

ACME challenges — the requester proves they control the domain via DNS records or HTTP tokens. This is how Let's Encrypt works today.

### For internal identities (employees, apps, devices)

The draft says the CA "validates each incoming issuance request" but leaves the method to the CA operator. For a private PKI, you'd implement a **Registration Authority (RA)** layer that checks:

- Is this request authenticated? (API key, OAuth token, mTLS client cert)
- Is this subject authorized? (employee exists in HR system, app is registered)
- Are the requested extensions allowed for this requester?

## Options for locking down the server

| Method | How it works |
|---|---|
| **API tokens** | Require a `Bearer` token header, issued to authorized enrollment agents |
| **mTLS** | Require a client certificate to talk to the CA (bootstrap problem, but works for renewal) |
| **ACME integration** | Full ACME flow with challenges per RFC 8555 |
| **Network isolation** | Only HR/provisioning systems can reach the CA endpoint |

## What a production flow looks like

1. Employee joins the company — HR system creates an identity record
2. Provisioning system calls the CA with an API token and the employee's details
3. CA validates the token, checks the employee exists in the directory
4. CA generates the certificate entry, adds it to the issuance log
5. Certificate is returned to the provisioning system and installed on the employee's device

The key principle: **the CA should never issue a certificate based solely on an unauthenticated request**. The issuance log is append-only — once something is in the Merkle tree, it cannot be removed. A misissued certificate is permanently visible to monitors.

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

## Root CAs — the grand poobah

There's a hierarchy of authority in PKI. A **Root CA** sits at the top and can revoke an entire intermediate CA:

```
Root CA (the grand poobah)
   |
   ├── Intermediate CA 1  (issues certs for employees)
   ├── Intermediate CA 2  (issues certs for devices)
   └── Intermediate CA 3  (issues certs for services)
```

The Root CA doesn't issue end-user certs directly. It signs the intermediate CAs' certificates, delegating authority. If an intermediate CA is compromised or misbehaves, the Root CA revokes its certificate — and every cert that intermediate issued becomes untrusted.

This is exactly how the web works today:

- Your browser ships with ~150 root CAs (Mozilla's root program, Apple's, Microsoft's)
- Each root has signed intermediates that do the actual issuance
- When a CA gets caught misbehaving (like DigiNotar in 2011, or Symantec in 2017), the root programs remove them and everything they issued becomes untrusted

### How this maps to MTC

| Traditional | MTC equivalent |
|---|---|
| Root CA | The trust store's list of trusted cosigner IDs |
| Intermediate CA | The issuance log + CA cosigner |
| Revoking an intermediate | Removing the cosigner ID from trusted list, or revoking all its indices |
| Root signs intermediate | Cosigner's public key is predistributed to relying parties |

The "grand poobah" in MTC is really **whoever controls the relying party's trust store** — the browser vendor, the OS vendor, or in a private PKI, your organization's IT security team. They decide which cosigner IDs to trust and can revoke an entire CA by removing it.

## What the MTC draft says about CA trust and distrust

The MTC draft addresses CA hierarchy and distrust directly, though the model is flatter than traditional PKI.

### Trusted cosigners (Section 7.3)

The relying party decides which cosigners to trust — this is the equivalent of the root trust store. The relying party's "cosigner policy" determines which sets of cosigners must sign a log view before it's accepted. Relying parties can require a **quorum** of multiple cosigners, reducing trust in any single one (though larger quorums mean bigger standalone certificates).

### Distrusting a CA (Section 7.5)

The draft gives two options when a CA is found untrustworthy:

1. **Nuclear option** — remove the cosigner ID entirely, killing all its certs
2. **Surgical option** — revoke indices after a certain point, keeping older valid certs alive

From the draft:

> "When a CA is found to be untrustworthy, relying parties SHOULD remove trust in that CA. To minimize the compatibility impact of this mitigation, index-based revocation can be used to only distrust entries after some index, while leaving existing entries accepted."

If monitors detect a CA presenting inconsistent views to different parties, relying parties can revoke the inconsistent indices and "likely remove the CA."

### No explicit Root CA hierarchy

MTC's trust model is **flat**, not hierarchical. The trust store directly lists cosigner IDs. There is no Root CA that signs intermediate CAs. The "grand poobah" is whoever controls the relying party's trust store — the browser vendor, the OS vendor, or your organization's IT security team.

The draft has a TODO noting that intermediates *could* be encoded in X.509 intermediate certificates "if an application wishes to use this with a delegation model," but adds "the security story becomes more complex." This is an open design question in the spec.

## Current state of this implementation

The reference server has no authentication on `/certificate/request`. This is intentional for development and testing. Adding API token authentication to that endpoint would be the minimal first step toward production readiness.

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

## Current state of this implementation

The reference server has no authentication on `/certificate/request`. This is intentional for development and testing. Adding API token authentication to that endpoint would be the minimal first step toward production readiness.

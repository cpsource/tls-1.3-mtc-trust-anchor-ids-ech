# How ACME Prevents Unauthorized Certificate Issuance

The ACME protocol (RFC 8555) doesn't block access to the CA server — anyone can talk to it. The control is: you only get a certificate if you **prove you own the thing you're requesting a cert for**.

## The flow

1. **Client registers an account** — creates a key pair, sends the public key to the CA. No identity check here. Anyone can make an account.

2. **Client requests a certificate** for `example.com`

3. **CA issues a challenge** — "prove you control example.com"

4. **Client completes the challenge** and tells the CA to verify

5. **CA checks** — actually reaches out to the domain/DNS to confirm

6. **Only then** does the CA sign and issue the certificate

## Challenge types

| Challenge | How it works |
|---|---|
| **HTTP-01** | CA tells you to put a specific token at `http://example.com/.well-known/acme-challenge/<token>`. CA fetches it. If it's there, you control the web server. |
| **DNS-01** | CA tells you to create a TXT record at `_acme-challenge.example.com`. CA queries DNS. If it's there, you control the domain. |
| **TLS-ALPN-01** | CA connects to your server on port 443 and checks for a specific self-signed cert containing the challenge token. Proves you control the TLS endpoint. |

## Why this works

You can't get a cert for `google.com` unless you can put files on Google's servers or modify Google's DNS records. The CA doesn't trust your identity — it trusts the **proof of control**.

This is fundamentally different from traditional identity verification (showing an ID, calling a phone number). ACME challenges are:

- **Automated** — no human in the loop
- **Cryptographic** — challenge tokens are random and signed
- **Verifiable** — the CA independently checks, not relying on the client's claim
- **Repeatable** — renewal uses the same proof mechanism

## For internal/private PKI

For our MTC server with internal identities (employees, apps, devices), there are no public domains to challenge against. The equivalent approaches are:

| Public PKI (ACME) | Private PKI (our server) |
|---|---|
| Prove you control a domain | Prove you're authorized in the org |
| HTTP-01 / DNS-01 challenges | API token from provisioning system |
| Anyone can request | Only authenticated agents can request |
| CA verifies externally | CA verifies against internal directory |

The principle is the same: **the CA must independently verify the requester's authority over the subject before issuing a certificate**. The mechanism differs because the subjects differ (public domains vs internal identities).

## What Boulder does beyond challenges

Boulder (Let's Encrypt's CA server) also enforces:

- **Rate limits** — max certificates per domain per week, preventing abuse
- **CAA records** — checks DNS CAA records to see if the domain owner has restricted which CAs may issue for them
- **Blocklists** — refuses to issue for certain high-value domains
- **Account key binding** — challenges are tied to the requesting account's key pair, preventing interception
- **Order expiry** — challenges must be completed within a time window

## Tie-in to MTC

In the MTC draft (Section 9), ACME is the assumed enrollment protocol. The flow is:

1. Client sends ACME order to the CA
2. CA validates via challenges
3. CA adds the entry to the issuance log (Merkle tree)
4. CA returns a standalone certificate once the entry is sequenced
5. Landmark certificate becomes available later

The challenge validation happens before step 3. Nothing enters the append-only log without passing validation first. This is critical because the issuance log is permanent and publicly auditable — a misissued certificate is visible to monitors forever.

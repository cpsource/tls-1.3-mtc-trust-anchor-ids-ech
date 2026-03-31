# TLS 1.3 Merkle Tree Certificates, Trust Anchor IDs, and ECH

Research notes and a working reference implementation exploring the next generation of TLS certificate infrastructure: **Merkle Tree Certificates (MTC)**, **Trust Anchor Identifiers**, and **Encrypted Client Hello (ECH)**.

This work is based on:

- [draft-ietf-plants-merkle-tree-certs-02](https://datatracker.ietf.org/doc/draft-ietf-plants-merkle-tree-certs/) — Merkle Tree Certificates
- [draft-ietf-tls-trust-anchor-ids](https://datatracker.ietf.org/doc/draft-ietf-tls-trust-anchor-ids/) — TLS Trust Anchor Identifiers
- [RFC 9849](https://datatracker.ietf.org/doc/rfc9849/) — TLS Encrypted Client Hello
- [RFC 9162](https://www.rfc-editor.org/rfc/rfc9162) — Certificate Transparency Version 2.0

## What is this?

Current Web PKI faces a scaling crisis. Post-quantum signature algorithms are 10-100x larger than today's, certificate lifetimes are shrinking (toward 47 days), and Certificate Transparency logs must store everything. Merkle Tree Certificates address this by moving trust from per-certificate signatures into a shared, append-only log structure backed by Merkle trees.

This repository contains:

1. **Research notes** covering the full landscape (PKI fundamentals through MTC architecture)
2. **A working CA/Log server** that implements the MTC issuance architecture
3. **A relying party client** that verifies MTC certificates locally
4. **Reference specifications** (the MTC Internet-Draft and RFC 9162)

## Repository structure

```
.
├── server/                  # MTC CA/Log server (Python)
│   ├── server.py            # HTTP REST API
│   ├── ca.py                # Certificate Authority + Issuance Log
│   ├── merkle.py            # Merkle Tree (RFC 9162 Section 2.1)
│   ├── db.py                # PostgreSQL persistence (Neon)
│   └── client_demo.py       # Server-side demo script
│
├── client/                  # MTC relying party client (Python)
│   ├── main.py              # CLI with bootstrap, enroll, verify, monitor commands
│   ├── mtc_client.py        # Client library (requests certs, verifies proofs)
│   ├── verify.py            # Standalone proof verification (inclusion, consistency, cosignature)
│   └── trust_store.py       # Local trust store (cosigner keys, landmark cache)
│
├── draft-ietf-plants-merkle-tree-certs-02.txt   # MTC Internet-Draft
├── rfc9162.txt                                   # Certificate Transparency v2
│
├── README-pki.md            # PKI fundamentals
├── README-ct.md             # Certificate Transparency explained
├── README-mct.md            # Merkle Tree Certificates overview
├── README-CA-log.md         # CA vs Log roles
├── README-CA.md             # How CA interaction works with MTC
├── README-trust-anchor-id.md  # Trust Anchor Identifiers
├── README-ech.md            # Encrypted Client Hello
├── README-shors.md          # Post-quantum threat (Shor's algorithm)
├── README-shors-algoritm.md # Shor's algorithm details
├── README-private-pki.md    # Private PKI considerations
├── README-implement.md      # Implementation complexity analysis
├── README-implement-1.md    # Detailed implementation notes + server docs
├── README-implement-in-tls.md  # MTC integration into TLS handshake
├── README-client-server.md  # Client-server interaction model
├── README-mct-blockchain.md # MTC vs blockchain comparison
├── README-feisty-duck.md    # Feisty Duck analysis notes
└── README-references.md     # Reference links
```

## Server

The CA/Log server implements the core MTC issuance architecture from the draft:

- **Append-only issuance log** backed by a Merkle tree (SHA-256, RFC 9162)
- **Ed25519 CA cosigning** with domain-separated signatures (MTC Section 5.4.1)
- **Standalone certificates** with inclusion proofs and cosignatures
- **Landmark certificates** with inclusion proofs to predistributed subtree hashes
- **Trust anchor ID** assignment per the draft
- **PostgreSQL persistence** (Neon) — the Merkle tree is rebuilt deterministically from stored entries on startup

### API

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/log` | Log state (tree size, root hash, landmarks) |
| GET | `/log/entry/<n>` | Individual log entry |
| GET | `/log/proof/<n>` | Inclusion proof with verification |
| GET | `/log/consistency?old=N&new=M` | Consistency proof between tree sizes |
| POST | `/certificate/request` | Issue a new MTC certificate |
| GET | `/certificate/<n>` | Retrieve an issued certificate |
| GET | `/trust-anchors` | List trust anchor IDs |
| GET | `/ca/public-key` | CA's Ed25519 public key |

### Running

```bash
cd server
python3 server.py              # starts on :8443
```

Requires: Python 3.12+, `cryptography`, `psycopg2-binary`. Database connection string is read from `MERKLE_NEON` in `~/.env`.

## Client

The relying party client verifies MTC certificates locally, without contacting the CA during verification:

- **Bootstrap trust** — fetch and store the CA's cosigner public key
- **Enroll** — generate key pairs and request certificates
- **Verify standalone certificates** — inclusion proof + cosignature + expiry
- **Verify landmark certificates** — inclusion proof against cached landmark hash (no signatures needed)
- **Monitor log consistency** — detect append-only violations between observations
- **Cache landmarks** — fetch and store landmark subtree hashes for offline verification

### Usage

```bash
cd client
python3 main.py bootstrap                          # fetch CA key
python3 main.py enroll example.com                  # generate key + request cert
python3 main.py verify 1                            # verify certificate #1
python3 main.py monitor                             # check log consistency
python3 main.py landmarks                           # cache landmark hashes
python3 main.py demo                                # run full workflow
```

## Key concepts

**Merkle Tree Certificates** replace per-certificate CA signatures with compact inclusion proofs into a signed log. This makes certificates smaller and avoids repeating large post-quantum signatures millions of times.

**Trust Anchor IDs** are compact identifiers for trusted roots, replacing full certificate chains in TLS negotiation. For standalone MTC certs, the trust anchor ID is the log ID. For landmark certs, it includes the landmark number.

**Standalone vs Landmark certificates:**
- **Standalone**: immediately usable, includes cosignatures. Larger but works with any relying party that trusts the CA.
- **Landmark**: smaller, no signatures needed. Only works with relying parties that have the landmark subtree hash cached. Available after a processing delay.

**Encrypted Client Hello (ECH)** is orthogonal — it encrypts the ClientHello to protect SNI and other metadata, but does not change certificate issuance or verification.

## Status

This is a research and learning repository. The implementation covers the core MTC architecture but intentionally does not implement: external cosigners, ACME integration, DER/ASN.1 encoding, TLS handshake integration, or log pruning.

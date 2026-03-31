# Landmarks

In MTC, a **landmark** is a periodic snapshot of the Merkle tree at a specific size that gets predistributed to clients.

## Why landmarks exist

A standalone certificate needs cosignatures, which are large — especially with post-quantum crypto. A landmark certificate avoids all signatures. It only needs an inclusion proof to a subtree hash that the client already has cached.

## How landmarks work

1. Every N entries (16 in our server), the current tree size is designated as a landmark
2. The subtree hash at that size is distributed to relying parties in the background (browser updates, config push, etc.)
3. For any certificate whose index falls before a landmark, the CA can build a landmark certificate: just the TBS data + an inclusion proof to that landmark subtree — no signatures at all
4. The client verifies by checking the inclusion proof against its cached landmark hash

## Standalone vs landmark certificates

| | Standalone | Landmark |
|---|---|---|
| Signatures | Yes (cosignatures included) | None |
| Size | Larger | Smaller |
| Works immediately | Yes | Only after landmark is distributed |
| Works for all clients | Yes | Only clients with the landmark cached |

## Why this matters

Landmarks are the key size optimization in MTC. Signatures still exist, but they're checked once when the landmark is distributed, not repeated in every certificate. The server prefers landmark certificates when the client supports them, falling back to standalone otherwise.

## Trust anchor IDs and landmarks

Each landmark gets its own trust anchor ID derived from the log ID plus the landmark number:

- Standalone certificate trust anchor ID: `32473.1` (the log ID)
- Landmark certificate trust anchor ID: `32473.1.0`, `32473.1.1`, etc.

During TLS negotiation, the client advertises which landmarks it supports. The server selects the smallest compatible certificate form.

## In this implementation

The client fetches and caches landmarks with:

```bash
python3 main.py landmarks
```

This stores landmark subtree hashes in the local trust store (`trust_store.json`). Once cached, landmark certificates can be verified offline without any signatures — trust comes entirely from the predistributed hash.

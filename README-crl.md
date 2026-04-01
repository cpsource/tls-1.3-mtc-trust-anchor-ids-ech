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

## Comparison

| | Traditional CRL | OCSP | MTC Revocation by Index |
|---|---|---|---|
| Data format | List of serial numbers | Online query/response | List of integer ranges |
| Size | Grows with revocations | Per-query | Tiny (a few ranges) |
| Online check needed | Download periodically | Yes, per cert | No |
| Handles unknown entries | No | No | Yes (revoke by range) |
| Handles CA distrust | Remove CA entirely | Remove CA entirely | Revoke range, keep valid entries |

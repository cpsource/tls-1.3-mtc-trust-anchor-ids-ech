Short answer:

> **No — a TLS handshake (even with MTC, Trust Anchor IDs, and ECH) should NOT require a live internet call to a CA or log.**

If it did, it would be too slow and fragile to work at internet scale.

---

# 🧠 1. The design principle

Modern TLS (and MTC) follow this rule:

> **All data needed to verify the peer must be available locally or sent in the handshake.**

No blocking external calls.

---

# 🔐 2. How this works today (classic TLS)

In normal TLS:

* server sends:

  * certificate chain
* client already has:

  * trusted root CAs

Client verifies locally:

```text
no internet call needed
```

(OCSP/CRL can be external, but even those are often cached or stapled.)

---

# 🌳 3. How this works with MTC

MTC keeps the same principle.

### Client already has:

* trusted **log root keys**
* possibly **landmark snapshots** (cached)

### Server sends:

* MTC certificate
* **inclusion proof**

---

### Verification flow

Client:

```text
1. Check Trust Anchor ID
2. Verify inclusion proof using cached Merkle root
3. Verify TLS signature (proof of private key)
```

👉 Still:

```text
no live network call
```

---

# ⚠️ 4. Where “external communication” comes in

The confusion comes from this phrase:

> “out-of-band communication”

That does NOT mean:

* during handshake ❌

It means:

* **before or between handshakes**

---

## Out-of-band updates

Clients periodically fetch:

* updated Merkle tree roots
* new landmark snapshots
* trust anchor mappings

This happens:

* via browser updates
* background services
* DNS / config distribution

👉 Like:

```text
OS updates trust store
```

---

# 🔄 5. When a network call *might* happen

Rare / fallback cases:

### 1. Client is out-of-date

* doesn’t recognize a landmark
* can’t validate proof

Then:

* may retry
* may fetch updated state

---

### 2. First-time trust bootstrap

* client needs initial trust anchors

---

### 3. Optional verification systems

* auditors
* monitoring tools

---

But critically:

> **These are NOT in the critical path of the handshake**

---

# 📊 6. Why this matters

If TLS required live CA/log calls:

* latency would explode
* availability would drop
* privacy would degrade
* system would fail under load

So the design avoids it.

---

# 🧠 7. Mental model

| Component           | Timing                |
| ------------------- | --------------------- |
| Certificate + proof | during handshake      |
| Trust anchor IDs    | during handshake      |
| Merkle roots        | cached locally        |
| Log updates         | background / periodic |

---

# 🔗 8. Compare to CT (today)

Certificate Transparency

CT already works this way:

* browser does NOT query CT logs during handshake
* it trusts:

  * SCTs (proofs)
  * cached log state

MTC follows the same philosophy, just more deeply integrated.

---

# 🧭 9. One-line summary

> **A TLS handshake with MTC is fully offline-verifiable; any interaction with logs or CAs happens ahead of time via cached trust data, not during the connection.**

---

# 🧠 Final insight (important for your design)

If you’re building your own system:

> **Never put external calls in the authentication path**

Instead:

* push data to clients ahead of time
* use proofs in the handshake
* design for offline verification

---

If you want, I can sketch a **minimal private implementation** where:

* you run a small “log”
* clients cache roots
* handshakes stay fast and local
  (it’s surprisingly doable)


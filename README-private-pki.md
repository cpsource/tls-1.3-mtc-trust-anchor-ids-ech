This passage is contrasting **public Web PKI** (the browser/HTTPS world) with **private PKI** (internal systems like yours), and explaining why Google’s new design (MTC) mostly targets the public side.

Let’s break it down cleanly.

---

# 🧠 1. “Private PKIs are excluded”

First, what’s a private PKI?

* **Public PKI** → internet-facing, browser-trusted certs (Google, banks, etc.)
* **Private PKI** → internal systems (VPNs, services, devices, your network auth)

👉 Key difference:

* Public PKI must follow strict global rules
* Private PKI = you control the rules

---

# 🌍 2. Why Google cares about public PKI

Google runs Chrome, so they care about:

* global HTTPS
* billions of certs
* strict requirements like **Certificate Transparency**

They’re saying:

> “We don’t want huge PQC certificates in X.509 for the public web.”

So they propose:

* **Merkle Tree Certificates (MTC)** as a replacement

---

# 🔐 3. Why private PKI is different

Private PKIs:

* do **not require CT**
* do **not require browser compatibility**
* have **smaller scale**
* operate under **controlled trust**

👉 So they don’t suffer from the same scaling crisis.

---

# ⚠️ 4. “Who would operate the complex infrastructure?”

MTC requires:

* global logs
* Merkle trees
* high-availability infrastructure
* auditing

For Google:

* doable

For private PKI:

> probably overkill

So the text is saying:

> “Even if MTC is great, most private orgs won’t want to run it.”

---

# 📦 5. Why PQC hurts public PKI more

Recall:

Post-Quantum Cryptography

### PQC impact:

* bigger public keys
* bigger signatures

---

### Public PKI (bad impact)

Each cert includes:

* public key
* CA signature
* **+ CT signatures (2 extra)**

👉 So size multiplies quickly

---

### Private PKI (less impact)

Private certs:

* no CT
* fewer signatures

👉 much smaller baseline

---

# 📊 6. Certificate size comparison

### Public cert (today)

```
public key
+ CA signature
+ CT signature 1
+ CT signature 2
```

### Private cert

```
public key
+ CA signature
```

👉 already ~2× smaller

---

# 🔄 7. Trust Anchor Identifiers (TAI)

New idea: **remove intermediate certs from handshake**

Normally TLS sends:

```
leaf cert
+ intermediate cert(s)
```

Each intermediate:

* has a public key
* has a signature

---

### With Trust Anchor Identifiers:

Trust Anchor Identifiers

* client already knows trusted roots
* server sends an **identifier**, not full chain

👉 removes:

* one public key
* one signature

---

# 📉 8. Final handshake size (private PKI)

After optimizations:

### Private PKI with PQC:

```
1 public key (leaf cert)
+ 1 CA signature (on cert)
+ 1 TLS handshake signature
```

👉 That’s it.

---

### Compare to public PKI:

* multiple certs
* multiple signatures
* CT overhead

---

# 🧠 9. What the paragraph is really saying

### Public PKI

* breaking under scale (CT + PQC + short certs)
* needs radical redesign (MTC)

### Private PKI

* simpler
* no CT
* fewer signatures
* can tolerate larger PQC sizes

👉 So:

> private PKI doesn’t need MTC (at least not urgently)

---

# 🔗 10. Why this matters to you

This directly applies to your system.

You’re building:

* cert-based auth
* internal system

👉 That is **private PKI**

---

### Implication:

You likely **don’t need**:

* CT
* MTC
* global logs

You can:

* keep X.509
* shorten cert lifetimes
* automate renewal (your earlier idea)
* maybe adopt lightweight optimizations like:

  * shorter chains
  * pinned roots
  * identity-bound certs

---

# 🧭 11. Big takeaway

> **Public PKI is hitting scaling limits and needs redesign; private PKI is simpler and can continue using traditional certificates with manageable overhead.**

---

# 🧠 One-line summary

> Private PKIs avoid the biggest scaling problems (CT and global logging), so even with larger post-quantum keys, they remain practical without needing complex new systems like MTC.

---

If you want, I can map this directly into your architecture and suggest:

* a modern private PKI design (short-lived certs, automated renewal)
* or how to “borrow” ideas from MTC without the complexity (very interesting path)


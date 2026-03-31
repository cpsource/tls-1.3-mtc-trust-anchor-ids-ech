This paragraph is basically saying:

> **MTC fixes scaling problems—but makes the system much more complicated to build, run, and debug.**

Let’s walk through it piece by piece.

---

# 🧠 1. “We’ll have two issuance platforms”

Today everything uses:

* **X.509**

With MTC:

* a **new issuance system** must be built
* alongside the old one

So for a long time, the ecosystem will run:

```text
Old world → X.509
New world → MTC
```

👉 That means:

* duplicate tooling
* duplicate processes
* compatibility headaches

---

# 🔀 2. “Web PKI will diverge from other PKIs”

This is important.

Public web PKI becomes:

* highly specialized
* dependent on CT + MTC + browser logic

Private PKI stays:

* simple
* X.509-based

👉 So:

> the “web PKI” becomes its own unique ecosystem

---

# 🔧 3. “TLS handshake must change”

TLS is how certificates are actually used in connections.

![Image](https://cf-assets.www.cloudflare.com/slt3lc6tev37/5aYOr5erfyNBq20X5djTco/3c859532c91f25d961b2884bf521c1eb/tls-ssl-handshake.png)

![Image](https://www.manageengine.com/key-manager/information-center/images/tls-handshake.png)

![Image](https://upload.wikimedia.org/wikipedia/commons/d/d3/Full_TLS_1.2_Handshake.svg)

Today:

* server sends a certificate
* client verifies it

With MTC:

* client may need:

  * inclusion proofs
  * log state
  * external validation data

👉 So TLS must evolve to:

* support new cert formats
* support new verification logic

---

# 🌐 4. “Out-of-band communication”

This is one of the biggest conceptual shifts.

### Old model

Everything needed is in the certificate:

```text
cert = self-contained proof
```

---

### New MTC model

Certificate is incomplete by itself:

```text
cert + external log data = trust
```

So:

* browsers must talk to **CT logs**
* or have cached log state

👉 That’s “out-of-band” (not part of the TLS handshake itself)

---

# ⏳ 5. “It may take days to propagate”

Because:

* CT logs update over time
* data must sync globally
* clients must learn new log states

👉 So:

* a newly issued MTC cert may not be immediately usable everywhere

---

# 🧩 6. “Standalone certificates” (fallback mode)

To handle that delay:

They introduce:

* **standalone MTC certs** (basically X.509-like)

👉 These:

* work immediately
* don’t depend on log propagation

So now you have **multiple cert types**:

---

# 🔀 7. Multiple certificate types per server

Servers must support:

```text
1. Traditional X.509 certs
2. Standalone MTC certs (fallback)
3. Landmark MTC certs (full new model)
```

👉 That means:

* multiple certs installed
* multiple validation paths

---

# 🤝 8. Complex TLS negotiation

Now the server must decide:

> “What kind of certificate does this client understand?”

Example:

| Client                        | Server sends   |
| ----------------------------- | -------------- |
| Old browser                   | X.509          |
| New browser (partial support) | standalone MTC |
| Fully updated browser         | landmark MTC   |

👉 This requires:

* feature detection
* negotiation logic
* fallback handling

---

# 🧨 9. Why this becomes painful

Troubleshooting today is already hard:

* cert chains
* trust stores
* expiration
* hostname mismatch

With MTC you add:

* multiple cert formats
* external log dependencies
* propagation delays
* negotiation paths

👉 So:

> debugging becomes significantly harder

---

# 🧠 10. What the paragraph is really warning

It’s a classic engineering tradeoff:

| Benefit           | Cost               |
| ----------------- | ------------------ |
| scalable with PQC | massive complexity |
| smaller certs     | new infrastructure |
| future-proof      | harder debugging   |
| efficient CT      | dependency on logs |

---

# 🔗 11. Tie back to your system

This is directly relevant to your earlier “flexi-cert” thinking.

You were trying to:

* reduce friction
* simplify lifecycle

MTC shows:

> **You can simplify one layer (cert size) but complexity moves elsewhere (protocol + infrastructure)**

---

# 🧭 12. Big takeaway

> MTC doesn’t remove complexity—it redistributes it:

* from certificates → to infrastructure, logs, and protocols

---

# 🧠 One-line summary

> Merkle Tree Certificates make certificates smaller and scalable, but at the cost of introducing multiple certificate types, external dependencies, and much more complex TLS negotiation and deployment.

---

If you want, I can translate this into a **design lesson for your system**—basically:

* how to get the benefits of MTC
* without inheriting all this complexity (there’s a sweet spot)


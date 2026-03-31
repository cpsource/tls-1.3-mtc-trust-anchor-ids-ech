**Shor’s algorithm** is a quantum algorithm that can efficiently break much of today’s public-key cryptography.

---

# 🧠 The core idea

Shor's algorithm

> It uses a quantum computer to solve certain math problems (factoring and discrete logarithms) **exponentially faster** than classical computers.

---

# 🔐 Why it matters

Most modern security depends on problems that are:

* **easy to do one way**
* **extremely hard to reverse**

Examples:

* Multiply primes → easy
* Factor the result → hard

Shor’s algorithm makes the “hard” part easy.

---

# 💥 What it breaks

Shor’s algorithm can break:

| System         | What it relies on       | Status under Shor |
| -------------- | ----------------------- | ----------------- |
| RSA            | factoring large numbers | ❌ broken          |
| ECC            | discrete logarithm      | ❌ broken          |
| Diffie–Hellman | discrete logarithm      | ❌ broken          |

---

# 🧩 What problem it actually solves

Two key math problems:

### 1. Integer factoring

Given:

```text
N = p × q
```

Find:

```text
p and q
```

---

### 2. Discrete logarithm

Given:

```text
g^x mod p = y
```

Find:

```text
x
```

---

# ⚙️ How it works (high level)

The magic step is:

> **period finding**

A quantum computer can:

* evaluate many values at once (superposition)
* detect patterns (periods) using interference

This is done with a quantum version of Fourier transform.

---

## Simplified flow

1. Pick a number related to the target (e.g., N for factoring)
2. Define a function with a hidden period
3. Use a quantum circuit to find that period
4. Convert the period into the solution (factors or discrete log)

---

# 📉 Why classical computers struggle

Best classical algorithms:

* take **sub-exponential time**
* still infeasible for large keys

Shor’s algorithm:

* runs in **polynomial time**

👉 That’s a massive speedup

---

# 🧠 Intuition (useful mental model)

Think of it like:

| Classical        | Quantum                       |
| ---------------- | ----------------------------- |
| try many guesses | evaluate all guesses at once  |
| search blindly   | detect hidden patterns        |
| brute force      | interference-based extraction |

---

# ⚠️ Important reality check

Shor’s algorithm is:

* proven mathematically
* demonstrated on small numbers

BUT:

👉 It requires a **large, fault-tolerant quantum computer**

We don’t have one yet at the scale needed to break real-world crypto.

---

# 🔗 Why you’re hearing about it now

Because:

* quantum hardware is improving
* estimates for required qubits are dropping
* long-lived data is at risk

So systems are moving toward:

* **Post-Quantum Cryptography**

---

# 🧭 One-line summary

> **Shor’s algorithm lets a quantum computer efficiently solve the math problems that current encryption depends on, making RSA and ECC insecure once large quantum machines exist.**

---

If you want, I can walk through:

* a concrete example of factoring using Shor (step-by-step)
* or how many qubits are estimated to break real-world keys (very relevant to your system design)


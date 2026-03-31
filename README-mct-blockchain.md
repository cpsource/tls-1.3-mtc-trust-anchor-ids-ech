Yes, **in principle**—but for MTCs, a blockchain is usually the wrong tool.

MTCs already get their core security property from **public, append-only, Merkle-tree-backed logs**. The current IETF draft describes MTCs as integrating public logging into the certificate design, and Google describes the model as a CA signing a single tree head that can cover many certificates, with clients receiving compact inclusion proofs rather than a traditional signature chain. ([IETF Datatracker][1])

The key point is that **MTC does not need global consensus to work**. It needs a log that is tamper-evident, auditable, and monitorable. Certificate Transparency already works this way: logs are publicly verifiable, append-only, and based on Merkle trees, and systems like Trillian provide proofs that a record is in the log and that the log history has not been rewritten. ([certificate.transparency.dev][2])

So if by “use blockchain” you mean “could I store MTC log state in a blockchain-like structure?”, the answer is **yes, technically**. A blockchain is one way to build an append-only authenticated data structure. But if you mean “should Web PKI MTCs be implemented on a blockchain?”, the answer is usually **no**, for a few reasons:

First, MTC is trying to **reduce size and overhead** in the face of shorter-lived certificates and larger PQC signatures. Adding blockchain consensus would typically add latency, operational complexity, and storage overhead, which cuts against that goal. The MTC draft and Google’s write-up are both about making authentication data smaller and logging more efficient, not about adding decentralized consensus. ([IETF Datatracker][1])

Second, CT and MTC security comes from **transparency plus independent monitoring**, not from “everyone agreeing on one chain.” CT’s model is that logs are auditable and third parties can detect misbehavior; Trillian even describes itself as “similar to a blockchain” while using Merkle-backed verifiable logs rather than full blockchain machinery. ([transparency.dev][3])

Third, MTC appears to be evolving toward a design where **each CA runs its own log of the certificates it issues**, which is a very different operational model from a shared global blockchain. Cloudflare’s write-up says the MTC specification makes CT a first-class feature by having each CA run its own log, and the newer draft is explicitly about reducing logging overhead compared with traditional X.509 plus CT. ([The Cloudflare Blog][4])

So the practical answer is:

* **Could blockchain be used?** Yes, as an implementation choice.
* **Would it improve MTC much?** Usually not.
* **What does MTC actually want?** A fast, auditable, append-only transparency system with compact proofs, which Merkle logs already provide without blockchain consensus. ([certificate.transparency.dev][5])

For a **private PKI**, there is one niche where a blockchain-like design might be interesting: if several mutually distrustful organizations jointly operate a shared issuance/transparency system and none wants a single operator to control the log. In that case, a consortium ledger could be used as the publication layer. But even there, I would first ask whether a simpler replicated transparency log with external auditors would do the job with less complexity. That is an inference from the architecture goals above, not something the draft specifically recommends. ([IETF Datatracker][1])

The shortest version is: **MTC already uses the part of blockchain people usually want here—the tamper-evident Merkle structure—without paying for the rest of blockchain.**

[1]: https://datatracker.ietf.org/doc/draft-davidben-tls-merkle-tree-certs/?utm_source=chatgpt.com "draft-davidben-tls-merkle-tree-certs-10"
[2]: https://certificate.transparency.dev/?utm_source=chatgpt.com "Certificate Transparency : Certificate Transparency"
[3]: https://transparency.dev/application/discourage-misbehaviour-by-third-parties-in-certificate-transparency/?utm_source=chatgpt.com "Discourage misbehaviour by third parties in Certificate ..."
[4]: https://blog.cloudflare.com/bootstrap-mtc/?utm_source=chatgpt.com "Keeping the Internet fast and secure: introducing Merkle ..."
[5]: https://certificate.transparency.dev/howctworks/?utm_source=chatgpt.com "How CT Works : Certificate Transparency"


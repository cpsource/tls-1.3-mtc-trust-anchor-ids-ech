I would not start by calling it “TLS 1.4.” Based on the current MTC work, the cleaner path is **TLS 1.3 plus new certificate formats and negotiation extensions**, not a whole new TLS version. TLS 1.3 already allows certificate-related extensibility in `CertificateEntry`, and the MTC draft explicitly describes a **recommended integration of Merkle Tree certificates into TLS** rather than a replacement protocol. ([RFC Editor][1])

A sensible “TLS 1.4 with MTCs” would look like this:

First, keep the **TLS 1.3 key schedule and handshake core** mostly unchanged. The expensive part MTC is trying to improve is not ECDHE, Finished, or record protection; it is the **certificate payload and trust machinery**. MTC leaves most X.509 fields, including `subjectPublicKeyInfo` and extensions like `subjectAltName`, unchanged for TLS use. ([IETF Datatracker][2])

Second, extend the certificate layer so the server can send **three authentication forms**:

* traditional X.509,
* **standalone MTC**, and
* **landmark MTC**.

The MTC draft already assumes TLS deployments may need to choose among a traditional X.509 CA, one MTC CA vs another, and standalone vs landmark certificates. It also says that when both landmark and standalone are supported, the server should prefer the smaller landmark form. ([IETF Datatracker][2])

Third, add an explicit **certificate-capabilities negotiation** in `ClientHello`. In today’s drafts, the nearest building block is **Trust Anchor Identifiers**, where the client conveys which trust anchors it supports and the server selects a compatible certification path; servers may also advertise available paths in DNS for lower latency. For an MTC-native redesign, I would make that negotiation richer and mandatory, so the client can say, in effect: “I support X.509, standalone-MTC, landmark-MTC; here are my supported trust anchors / landmarks / freshness constraints.” ([IETF Datatracker][3])

Fourth, make the certificate message carry **proof-oriented objects**, not just self-contained cert chains. In TLS 1.3, a `CertificateEntry` currently carries either X.509 cert bytes or a raw public key. An MTC-native TLS could define new certificate types for standalone and landmark MTC entries, each encoded as compact proof objects rather than ordinary signed leaf-plus-intermediate chains. The MTC draft notes its certificate types fit into the TLS `CertificateEntry` model. ([RFC Editor][1])

Fifth, split validation into two modes:

* **Standalone mode**: works broadly, similar to classic PKI, and is the immediate fallback.
* **Landmark mode**: optimized, smaller, but only for clients that are already up to date with the relevant trusted subtrees.

That is exactly how the current MTC design works: relying parties that already support a landmark subtree can accept a landmark certificate; otherwise the server sends a standalone certificate. The draft also says relying parties may keep a periodically updated predistributed list of active landmark subtrees. ([IETF Datatracker][2])

Sixth, make **out-of-band state** a first-class part of the protocol design. This is the big architectural change. Landmark MTCs only work well when clients regularly obtain trusted subtree state from an update service or similar mechanism. The draft leaves the transport for that state out of scope, but explicitly allows application-specific update services and says TLS certificate selection should incorporate this configuration. In a hypothetical TLS 1.4, I would standardize this better instead of leaving so much outside TLS. ([IETF Datatracker][2])

Seventh, make **retry and fallback** smoother than the current drafts. Trust Anchor Identifiers currently define a retry flow where, if the client sent a subset or empty trust-anchor list, the server can suggest acceptable anchors in `EncryptedExtensions`, and the client may reconnect once with a narrower choice. The draft itself notes that a new-connection retry is expensive and suggests future handshake modifications may avoid that. A true TLS 1.4 could build that in directly: a server-selected certificate-path or landmark choice within the same handshake, rather than forcing a second connection. ([IETF Datatracker][3])

Eighth, keep **ECH** in front of all this. If you are redesigning TLS around MTCs, you still want `ClientHello` privacy. ECH is now standardized and encrypts the `ClientHello`; that is the right place to hide any richer MTC capability advertisement and trust-anchor hints from passive observers. ([RFC Editor][4])

So, if I were sketching “TLS 1.4 with MTCs,” the handshake would be roughly:

1. Client sends ECH-protected `ClientHello` with:

   * supported cert formats: X.509 / standalone-MTC / landmark-MTC,
   * supported trust anchors / highest landmark IDs,
   * maybe policy bits for acceptable cosigner sets or freshness.
2. Server responds with `ServerHello` as usual.
3. In encrypted handshake messages, server either:

   * selects one certificate path directly, or
   * if needed, sends a compact selector hint.
4. Server `Certificate` carries:

   * traditional chain, or
   * standalone MTC proof object, or
   * landmark MTC proof object.
5. `CertificateVerify` still proves possession of the private key for the leaf public key.
6. Client validates:

   * leaf identity fields,
   * proof inclusion / trusted subtree state,
   * revocation / index policy,
   * normal expiry and TLS checks.

One subtle but important point: **MTC does not eliminate proof-of-possession in TLS.** The certificate gets smaller because the trust evidence moves toward Merkle proofs and trusted subtrees, but the handshake still needs a signature in `CertificateVerify` to prove the peer controls the corresponding private key. TLS 1.3 already defines that signed step separately from the certificate object itself. ([RFC Editor][1])

My bottom line is:

* **Best practical answer:** don’t invent TLS 1.4; use TLS 1.3 with MTC + Trust Anchor IDs + ECH.
* **Best architectural answer:** if you did invent TLS 1.4, the main changes would be **certificate negotiation, proof-carrying certificate types, standardized subtree-state distribution, and same-connection fallback**, not a new cryptographic core. ([IETF Datatracker][2])

The part I’m inferring, rather than quoting from a spec, is the “same-connection fallback” and “mandatory richer negotiation” in a hypothetical TLS 1.4. The current drafts point in that direction, but they do not define a finished TLS 1.4 design.

[1]: https://www.rfc-editor.org/rfc/rfc8446.html "RFC 8446: The Transport Layer Security (TLS) Protocol Version 1.3"
[2]: https://datatracker.ietf.org/doc/draft-ietf-plants-merkle-tree-certs/ "
            
    
        draft-ietf-plants-merkle-tree-certs-02 - Merkle Tree Certificates
    

        "
[3]: https://datatracker.ietf.org/doc/draft-ietf-tls-trust-anchor-ids/ "
            
    
        draft-ietf-tls-trust-anchor-ids-03 - TLS Trust Anchor Identifiers
    

        "
[4]: https://www.rfc-editor.org/rfc/rfc9849.html "RFC 9849: TLS Encrypted Client Hello"


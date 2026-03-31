
Feisty Duck
Cryptography & Security Newsletter
31 March 2026  |  READ ONLINE  |  UNSUBSCRIBE	135
Feisty Duck’s Cryptography & Security Newsletter is a periodic dispatch bringing you commentary and news surrounding cryptography, security, privacy, SSL/TLS, and PKI. It's designed to keep you informed about the latest developments in this space. Enjoyed every month by more than 50,000 subscribers. Written by Ivan Ristić.




Practical TLS and PKI Training
Practical TLS and PKI Training is for system administrators, developers, and IT security professionals who wish to learn how to deploy secure servers and encrypted web applications and understand the theory and practice of Internet PKI. Based on our book Bulletproof TLS and PKI. Upcoming dates: EU timezones 14-17 April 2026



Web PKI Reimagined with Merkle Tree Certificates
In the past several years, the world has been busy with the migration to post-quantum cryptography, but you couldn’t hear much of Google's plans when it comes to Web PKI. However, work has been in progress for several years, going back to at least early 2023. In late 2025, joining with other interested parties, Google migrated its work to an IETF working group called PLANTS. Work is now ongoing to refine the design and validate it in collaboration with Cloudflare. Recently, Google published a blog post to officially announce this work and provide further details about its future steps. In short, the core design is baked, and the remainder of 2026 will be spent on validating the core technology. In 2027, Google will bootstrap the next-generation Web PKI.

It’s fortunate that cryptographically relevant quantum computers don’t pose a threat against authentication on the web until they actually materialize. The effort so far has been on protecting the key exchange of the TLS handshake, which is susceptible to “store now, decrypt later” attacks. Chrome and other major browsers mitigated this immediate threat by deploying X25519MLKEM768. Now the rest of the world needs to catch up by adding support for this post-quantum safe key exchange on the server side.

Because there is time, Google is taking the opportunity to make some substantial changes to how Web PKI operates. Crucially, Google is further refining the concept of Certificate Transparency (CT) and, in fact, merging it with certificate issuance so that they are inseparable. The new design is reworking trust and introducing massive complexity into the ecosystem, but, if it’s successful, the transition to post-quantum cryptography will not introduce a performance regression for Web PKI.

Why Is a Redesign Necessary?
There are two driving forces behind the redesign effort. First, there is a problem with the massive increase in public key and signature sizes of the new post-quantum cryptographic algorithms. At present, our most efficient algorithm (EC) takes only sixty-four bytes for a public key or signature, compared to 1,312 bytes for a ML-DSA-44 public key and 2,420 bytes for a signature. A typical TLS handshake may include two public keys and five signatures. Upgrading just the algorithms would increase the size of the signature ten times. This, for Google, is an unacceptable performance regression, at least when it comes to public web traffic.

The other problem is with CT, which has been deeply embedded in Web PKI since 2018. CT has been enormously successful in terms of providing ecosystem visibility, but running it hasn’t been smooth. In fact, it remains fragile—and yet it’s at the center of certificate issuance. If CT breaks, all issuance breaks. The increasing PQC signature sizes would impact CT a great deal also, because CT log entries contain one public key and signature. A tenfold increase in the size of the logs would break the current arrangement. To make this situation worse, the ecosystem is transitioning to enforced forty-seven-day certificates in 2029, which will lead to a substantially increased number of certificates, further increasing the size of the data.

Enter Merkle Tree Certificates
Google’s solution is Merkle Tree Certificates (MTCs), an evolution of X.509 certificates that change how trust is implemented. The core idea is to switch to so-called landmark certificates (in MTC terminology) that don’t include any signatures at all. At the moment, a fully formed X.509 certificate contains a public key, one signature from a CA, and two signatures from CT. In the new design, the public key remains in the certificate, but the rest can be replaced with aninclusion proof, leading to a massive decrease in certificate size when post-quantum cryptography is taken into account. We can’t magically do away with any signatures, but they can be moved elsewhere so that they’re not used or needed most of the time.

The next-generation CT will benefit from several key design decisions: (1) CT log entries will not contain certificate public keys or CA signatures; (2) precertificates are being removed, reducing the amount of data needed for the logging; and (3) ecosystem-wide duplication will be eliminated by having CT logs contain only certificates issued by a single CA and removing public submissions (which don’t add value any more but do increase data requirements).

Private PKIs Are Excluded
Google has decided to pursue MTCs as a mitigation for the increase in the size of post-quantum cryptography and the next step in the evolution of CT. The company is on the record as saying that it won’t support PQC-enabled X.509 certificates for public properties, unless there is no other option. However, Google has also said that private PKIs will not be affected. That makes sense, but it’s not clear who would operate the complex infrastructure for private issuance.

At the same time, the increased public key and signature size of post-quantum cryptography will not affect private PKIs as much. As such, private certificates don’t use CT, meaning that they don’t need the two extra signatures that are embedded in public certificates. That also means that there isn’t any CT infrastructure to worry about. Moreover, a new standard called Trust Anchor Identifiers (developed in parallel with MTC) will make it possible to remove intermediate certificates from the TLS handshake, reducing its size by one public key and one signature. When these are added up, private PKI handshakes will need to carry only one public key (in the certificate) and two signatures (one CA signature on the certificate and one signature for the TLS handshake itself). This will be bigger than it is now, but acceptable for many use cases.

The Price Is Increased Complexity
The new design will mitigate the changes required by post-quantum cryptography—but at what cost? Rearchitecting issuance and building of an entirely new ecosystem to issue MTCs is a massive effort. Google is doing most of the work, with CAs to follow. Once MTCs are implemented, we will have two issuance platforms, the old and the new. Web PKI will further diverge from all other PKIs.

The TLS handshake also will need to change, which means that, eventually, all server platforms will need to update. It’s not going to be a straightforward update, either. The problem with landmark certificates is that they require out-of-band communication between relying parties (e.g., browsers) and future CT logs. It may take several days until the necessary information is propagated. In the meantime, standalone certificates, which are very similar to existing X.509 certificates, have to be used. To make this work, every server will need to support multiple valid certificates of different types (standard X.509, standalone MTC, landmark MTC) and support complex TLS handshake negotiation to ensure each user agent is offered a certificate they will accept. Troubleshooting TLS and PKI has never been easy, but it’s about to get much more complicated.

Google’s Quantum Breakthrough?
A couple of days ago, Google announced that it was accelerating its deadline for migrating to post-quantum cryptography, citing advances in quantum computing hardware, as well as the growing threat of store now, decrypt later attacks. Google is now planning to complete this work by 2029. Today, as we were about to publish this newsletter, the clues behind the decision came to light, and there are two of them.

First, Google Research released a strongly worded paper describing a new and significantly better algorithm that can be used to break elliptic curve cryptography. They’re claiming “nearly a 20 fold reduction over prior estimates” for attacks against secp256k1. Additionally, they say that it might be possible to achieve the break in only minutes. Google has decided not to release the details of the algorithm, instead publishing a zero-knowledge proof that they have it. As a reminder, virtually all cryptocurrencies run on top of elliptic curve cryptography.

Shor's algorithm breaks most classic cryptography—RSA, ECC, and Diffie-Hellman—but ECC falls first because its smaller key sizes require fewer quantum resources than RSA, despite harder per-bit arithmetic. Attacks against ECC have become more successful recently, and that was before we learned about the most recent research paper.

The second clue for Google’s decision might be the other research paper just published, which claims that breaking RSA requires only 10,000 reconfigurable atomic qubits, a much lower number than previously estimated. It seems that the race is on: which is breaking first, ECC or RSA?

Encrypted Client Hello
Encrypted Client Hello (ECH) has been standardized as RFC 9849. This addition to TLS 1.3 makes it possible to encrypt the very first message exchanged with a server, which in turn protects sensitive information in the TLS handshake, such as the exact identity of the server (also known as Server Name Indication, or SNI). The companion document, RFC 9848, creates a mechanism to discover ECH configuration and encryption keys via SVCB/HTTPS resource records. For a light introduction, head to our July 2025 newsletter in which we provided more information. ECH has the potential to lead to much better privacy online, but it may also impact legitimate security operations.

Short News
Post-Quantum Cryptography
Android 17, currently in beta, is adding support for post-quantum cryptography at the core of the platform, aiming to protect the operating system as well as the applications.
Marin Ivezic has released the updated version of his framework for PRC migration, as well as additional sector-specific guidance.
According to Cloudflare, 65% of its traffic (excluding bots) is being protected by X25519MLKEM768, but only about 10% of the origin servers have post-quantum support. You can track these numbers in real-time on Cloudflare’s dashboard. If you recall from last month, Jan Shaumann’s measurement is about 41% among the Tranco Top 1M domains.
The Global Risk Institute has updated its Quantum Threat Timeline Report for 2025. The expert consensus is that the timeline to “Q-Day” is accelerating and that the emergence of a cryptographically relevant quantum computer is quite possible (28–49%) within the next ten years, and likely (51–70%) in the next fifteen.
The German BSI's TR-02102 series (updated to version 2026-01) is a set of four technical guidelines providing cryptographic algorithm recommendations and key length guidance for general use, TLS, IPsec/IKEv2, and SSH.
France's ANSSI has updated its cryptographic mechanisms guide to explicitly address the quantum threat, recommending hybrid key encapsulation mechanisms (KEMs; preferring ML-KEM-768), hybrid signatures (ECDSA plus ML-DSA rather than ML-DSA alone), and RSA-3072 as a minimum.
AWS outlined a phased, four-workstream plan to migrate its services and open-source libraries to post-quantum cryptography—prioritizing hybrid ML-KEM key exchange for public endpoints first, followed by long-lived signing roots, and finally session-based certificate authentication as standards mature.
Although Akamai has deployed post-quantum hybrid key exchange for TLS, the quantum threat extends far beyond HTTPS—requiring PQC migrations across SSH, IPsec VPNs, messaging protocols, OpenPGP, and DNSSEC, each with varying degrees of urgency and complexity.
Google announced that OpenTitan—the first open-source silicon Root of Trust, featuring post-quantum cryptography support via SLH-DSA—is now shipping in commercially available Chromebooks, marking a major milestone after seven years of development.
The PQCA CBOM Kit is a suite of five interconnected tools for generating, managing, and evaluating cryptographic bills of materials (CBOMs)—structured inventories of cryptographic assets in software systems—to help organisations assess their readiness for post-quantum cryptography.
PQC-LEO is an open-source automated benchmarking framework for researchers that measures the computational and TLS 1.3 performance of post-quantum cryptographic algorithms using OpenSSL 3.5.0 and the Open Quantum Safe libraries across x86 and ARM Linux systems.
Privacy
In a landmark March 2026 vote, the European Parliament narrowly passed an amendment requiring that any scanning of private communications be limited to individuals judicially suspected of child sexual abuse, rejecting untargeted mass surveillance and piling pressure on EU governments ahead of trilogue negotiations.
Instagram's end-to-end encrypted messaging—which ensures that only the communicating parties can read messages or hear calls, not Meta—will no longer be supported after May 8, 2026, with affected users advised to download any content they wish to keep.
KU Leuven's Bart Preneel highlights an open letter signed by over 370 scientists calling for a moratorium on age-verification technologies. He argues that while protecting minors is a legitimate goal, mandatory online age checks risk creating a permanent identification infrastructure with serious privacy, exclusion, and surveillance implications, and they lack sufficient evidence of effectiveness.
An unsecured MongoDB database belonging to IDMerit—a know-your-customer (KYC) identity-verification provider—exposed approximately one billion records of highly sensitive personal data including national IDs, dates of birth, and phone numbers across twenty-six countries, with the US alone accounting for over 203 million leaked records.
KU Leuven's COSIC team has published research revealing critical vulnerabilities in Microsoft's PhotoDNA CSAM-detection algorithm, including hash reversal, detection evasion, false positives, and engineered collisions—all achievable in seconds on a standard laptop, raising serious concerns about proposals to deploy such fragile systems for large-scale client-side scanning.
A California court ruled in Camplisson v. Adidas that website-tracking pixels (TikTok Pixel, Microsoft Bing) can constitute pen registers under California's CIPA, exposing businesses to $5,000 fines per violation in statutory damages and raising the bar for valid user consent.
X.509 and PKI
Ryan Hurst launched WebPKI Observatory, a new effort to provide visibility into the most widely deployed public PKI. He published a blog post announcing the launch.
RFC 9336 defines a new general-purpose X.509 Extended Key Usage identifier (id-kp-documentSigning) specifically for certificates used to digitally sign human-readable documents, separating this use case from the previously misappropriated id-kp-emailProtection and id-kp-codeSigning identifiers to reduce cross-protocol attack risks.
Chrome is removing support for SCTs delivered via OCSP responses starting in Chrome 148 (May 5, 2026) as usage is negligibly low. Affected site operators must switch to embedded or TLS-delivered SCTs before that date.
The CA/Browser Forum's Ballot SMC015v2 passed unanimously, officially recognising mobile driver's licences (mDLs) as valid cryptographic proof of identity for publicly trusted S/MIME and client authentication certificates.
A new PKI is born: SSL.com has become the first publicly trusted certificate authority to issue production-ready C2PA-conformant certificates, enabling customers to embed tamper-evident content provenance and authenticity credentials into digital media as a countermeasure against deepfakes and AI-generated misinformation. Sherif Hanna, the C2PA lead at Google, posted an early signed photo to LinkedIn, captured by a Google Pixel device. Hanna also created a NotebookLM notebook if you’d like to drill in deeper.
Cryptography
RWC 2026 took place in Taipei, Taiwan, on March 9–11, 2026. The slides and video streams are now available.
Randomness strikes again. A wireless vulnerability (CVE-2024-12054) in SAE J2497/PLC4TRUCKS trailer brake controllers was disclosed, by which a weak, sixteen-bit seed-key algorithm could allow attackers to remotely unlock electronic control units (ECUs) and control brake pressure across over five hundred thousand deployed trailers.
Tim Cappalli warns that using the WebAuthn PRF extension to derive encryption keys from passkeys is dangerous for end users because deleting a passkey (something users do without thinking) can permanently and silently destroy access to encrypted backups, messages, and files.
The Connectivity Standards Alliance launched Aliro 1.0, a universal digital-access standard backed by Apple, Google, Samsung, and 220-plus companies, designed to replace physical keycards and fobs with a single interoperable credential across homes, offices, hotels, and more.
An ACM CCS 2025 paper presents an interview study that explores what motivates participants in cryptography competitions (such as NIST PQC and CAESAR). It examines how such competitions shape careers, research directions, and the broader cryptography community.
Last month, we mentioned a research paper on password manager security, focusing on Bitwarden, LastPass, and Dashlane. The paper has been updated with an appendix dedicated to 1Password.
Other News
Following rigorous evaluation by Germany's BSI, iPhones and iPads running iOS/iPadOS 26 have become the first consumer devices certified to handle classified information in NATO-restricted environments—without requiring any special software or settings.
Researchers from ETH Zürich demonstrated BGP Vortex, a new attack that exploits two widely used BGP routing policies to trigger persistent route oscillations across the internet, with twenty-one of the thirty largest networks found to be vulnerable. A coordinated attack could potentially impact 96% of all internet networks.
We may sometimes use Anthropic’s Claude to help us create short news summaries.

Copyright © 2026 Feisty Duck Ltd

86-90 Paul Street, London EC2A 4NE, United Kingdom
www.feistyduck.com / hello@feistyduck.com

You are receiving this email because you are subscribed to the Cryptography & Security Newsletter (previously Bulletproof TLS Newsletter). If you'd prefer not to receive further emails, please unsubscribe here.

 


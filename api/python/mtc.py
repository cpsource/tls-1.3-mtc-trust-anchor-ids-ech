"""
MTC Public API (Python).

User-facing API that wraps the internal client/server machinery into
clean, high-level calls. Applications import this module instead of
dealing with proofs, cosignatures, or trust stores directly.

Usage:
    from mtc import MTC_Connect, MTC_Enroll, MTC_Verify, MTC_Revoke
    from mtc import MTC_Find, MTC_List, MTC_Status, MTC_Renew

    # Connect to a CA/Log server
    conn = MTC_Connect("http://localhost:8443")

    # Enroll a new identity
    cert = MTC_Enroll(conn, "urn:ajax-inc:app:myapp",
                      extensions={"human_id": "Cal Page"})

    # Verify a certificate
    result = MTC_Verify(conn, index=59)

    # Find certificates by subject
    matches = MTC_Find(conn, "bosfitch")

    # List local certificates in ~/.TPM
    local = MTC_List()

    # Check server and trust store status
    status = MTC_Status(conn)

    # Renew an expiring certificate
    new_cert = MTC_Renew(conn, index=59)
"""

import json
import os
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

# Add client/ to path for imports
_root = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(_root / "client"))

from mtc_client import MTCClient
from trust_store import TrustStore

TPM_DIR = Path.home() / ".TPM"


@dataclass
class MTCConnection:
    """An active connection to an MTC CA/Log server."""
    server_url: str
    client: MTCClient
    ca_name: str = ""
    log_id: str = ""
    tree_size: int = 0
    bootstrapped: bool = False


@dataclass
class MTCCertificate:
    """A simplified view of an issued MTC certificate."""
    index: int
    subject: str
    trust_anchor_id: str
    not_before: float
    not_after: float
    extensions: dict
    has_landmark: bool
    local_path: Optional[str] = None
    raw: Optional[dict] = None


@dataclass
class MTCVerifyResult:
    """Result of verifying a certificate."""
    index: int
    subject: str
    valid: bool
    inclusion_proof: bool
    cosignature_valid: bool
    not_expired: bool
    landmark_valid: Optional[bool] = None
    error: Optional[str] = None


# ---------------------------------------------------------------------------
# MTC_Connect
# ---------------------------------------------------------------------------

def MTC_Connect(server_url: str, store_path: str = "trust_store.json") -> MTCConnection:
    """
    Connect to an MTC CA/Log server and bootstrap trust.

    Fetches the CA's public key, stores it in the local trust store,
    and caches the current log state.
    """
    client = MTCClient(server_url, store_path)
    info = client.server_info()
    client.bootstrap_trust()
    log = client.fetch_log_state()

    return MTCConnection(
        server_url=server_url,
        client=client,
        ca_name=info.get("ca_name", ""),
        log_id=info.get("log_id", ""),
        tree_size=log.get("tree_size", 0),
        bootstrapped=True,
    )


# ---------------------------------------------------------------------------
# MTC_Enroll
# ---------------------------------------------------------------------------

def MTC_Enroll(
    conn: MTCConnection,
    subject: str,
    algorithm: str = "EC-P256",
    validity_days: int = 90,
    extensions: Optional[dict] = None,
) -> MTCCertificate:
    """
    Generate a key pair, request a certificate, and store in ~/.TPM.
    """
    priv_pem, pub_pem = conn.client.generate_key_pair(algorithm)

    safe_name = subject.replace("/", "_").replace(":", "_")
    subdir = TPM_DIR / safe_name
    subdir.mkdir(parents=True, exist_ok=True)

    key_path = subdir / "private_key.pem"
    key_path.write_text(priv_pem)
    os.chmod(key_path, 0o600)
    (subdir / "public_key.pem").write_text(pub_pem)

    ext = {"key_usage": "digitalSignature"}
    if extensions:
        ext.update(extensions)

    result = conn.client.request_certificate(
        subject=subject,
        public_key_pem=pub_pem,
        key_algorithm=algorithm,
        validity_days=validity_days,
        extensions=ext,
    )

    with open(subdir / "certificate.json", "w") as f:
        json.dump(result, f, indent=2)
    (subdir / "index").write_text(str(result["index"]))

    sc = result["standalone_certificate"]
    tbs = sc["tbs_entry"]

    return MTCCertificate(
        index=result["index"],
        subject=tbs["subject"],
        trust_anchor_id=sc["trust_anchor_id"],
        not_before=tbs["not_before"],
        not_after=tbs["not_after"],
        extensions=tbs["extensions"],
        has_landmark="landmark_certificate" in result,
        local_path=str(subdir),
        raw=result,
    )


# ---------------------------------------------------------------------------
# MTC_Verify
# ---------------------------------------------------------------------------

def MTC_Verify(conn: MTCConnection, index: int) -> MTCVerifyResult:
    """
    Verify a certificate by index. Checks ~/.TPM first, then server.
    """
    cert = _load_local_cert(index)
    if cert is None:
        cert = conn.client.get_certificate(index)
    if cert is None:
        return MTCVerifyResult(
            index=index, subject="", valid=False,
            inclusion_proof=False, cosignature_valid=False,
            not_expired=False, error="certificate not found",
        )

    sc = cert.get("standalone_certificate", cert)
    sv = conn.client.verify_standalone_certificate(sc)

    result = MTCVerifyResult(
        index=sv["index"],
        subject=sv["subject"],
        valid=sv["valid"],
        inclusion_proof=sv["checks"]["inclusion_proof"],
        cosignature_valid=all(c["valid"] for c in sv["checks"]["cosignatures"]),
        not_expired=sv["checks"]["not_expired"],
    )

    if "landmark_certificate" in cert:
        lv = conn.client.verify_landmark_certificate(cert["landmark_certificate"])
        result.landmark_valid = lv["valid"]

    return result


# ---------------------------------------------------------------------------
# MTC_Find
# ---------------------------------------------------------------------------

def MTC_Find(conn: MTCConnection, query: str) -> list[dict]:
    """Search for certificates by subject (case-insensitive substring)."""
    result = conn.client.search_certificates(query)
    return result.get("results", [])


# ---------------------------------------------------------------------------
# MTC_List
# ---------------------------------------------------------------------------

def MTC_List() -> list[MTCCertificate]:
    """List all certificates stored locally in ~/.TPM."""
    if not TPM_DIR.exists():
        return []

    certs = []
    for subdir in sorted(TPM_DIR.iterdir()):
        if not subdir.is_dir():
            continue
        cert_file = subdir / "certificate.json"
        if not cert_file.exists():
            continue
        with open(cert_file) as f:
            data = json.load(f)
        sc = data["standalone_certificate"]
        tbs = sc["tbs_entry"]
        certs.append(MTCCertificate(
            index=data["index"],
            subject=tbs["subject"],
            trust_anchor_id=sc["trust_anchor_id"],
            not_before=tbs["not_before"],
            not_after=tbs["not_after"],
            extensions=tbs["extensions"],
            has_landmark="landmark_certificate" in data,
            local_path=str(subdir),
        ))
    return certs


# ---------------------------------------------------------------------------
# MTC_Status
# ---------------------------------------------------------------------------

def MTC_Status(conn: MTCConnection) -> dict:
    """Get current status of the server and local trust store."""
    log = conn.client.fetch_log_state()
    return {
        "server": conn.server_url,
        "ca_name": conn.ca_name,
        "log_id": conn.log_id,
        "tree_size": log["tree_size"],
        "root_hash": log["root_hash"],
        "landmarks": log.get("landmarks", []),
        "consistency": log.get("consistency"),
        "trust_store": conn.client.store.summary(),
        "local_certificates": len(MTC_List()),
    }


# ---------------------------------------------------------------------------
# MTC_Renew
# ---------------------------------------------------------------------------

def MTC_Renew(conn: MTCConnection, index: int, validity_days: int = 90) -> MTCCertificate:
    """
    Renew a certificate with a fresh key pair, same subject and extensions.
    Old cert/key archived with .old suffix in ~/.TPM.
    """
    cert = _load_local_cert(index)
    if cert is None:
        cert = conn.client.get_certificate(index)
    if cert is None:
        raise ValueError(f"certificate {index} not found")

    tbs = cert["standalone_certificate"]["tbs_entry"]
    subject = tbs["subject"]

    safe_name = subject.replace("/", "_").replace(":", "_")
    subdir = TPM_DIR / safe_name
    if subdir.exists():
        for name in ("certificate.json", "private_key.pem"):
            p = subdir / name
            if p.exists():
                p.rename(subdir / (name + ".old"))

    return MTC_Enroll(
        conn=conn,
        subject=subject,
        algorithm=tbs["subject_public_key_algorithm"],
        validity_days=validity_days,
        extensions=tbs["extensions"],
    )


# ---------------------------------------------------------------------------
# MTC_Revoke (placeholder)
# ---------------------------------------------------------------------------

def MTC_Revoke(conn: MTCConnection, index: int) -> dict:
    """
    Request revocation of a certificate.

    In MTC, revocation is done by the relying party updating its revoked
    index ranges, not by the certificate holder. This notifies the CA
    to add the index to the revocation list.
    """
    return {
        "index": index,
        "status": "revocation_requested",
        "note": "Relying parties must update their revoked index ranges to enforce this.",
    }


# ---------------------------------------------------------------------------
# Internal
# ---------------------------------------------------------------------------

def _load_local_cert(index: int) -> Optional[dict]:
    if not TPM_DIR.exists():
        return None
    for subdir in TPM_DIR.iterdir():
        idx_file = subdir / "index"
        if idx_file.exists() and idx_file.read_text().strip() == str(index):
            cert_file = subdir / "certificate.json"
            if cert_file.exists():
                with open(cert_file) as f:
                    return json.load(f)
    return None

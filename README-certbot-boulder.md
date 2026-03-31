# Certbot and Boulder

These are the two open-source projects that power Let's Encrypt, the world's largest certificate authority. Together they implement the ACME protocol (RFC 8555) for automated certificate issuance and renewal.

Clone them into this repository with:

```bash
./clone-lets-encrypt.sh
```

## Certbot (ACME Client)

- **Repository**: https://github.com/certbot/certbot
- **Language**: Python
- **Role**: Client that requests and renews certificates
- **Analogous to**: our `client/` directory

Certbot is the EFF's tool that runs on your server. It:

1. Generates a key pair
2. Contacts the ACME CA (e.g. Let's Encrypt) and requests a certificate
3. Proves domain ownership via challenges (HTTP-01, DNS-01)
4. Downloads the signed certificate
5. Installs it for your web server (Apache, Nginx, etc.)
6. Auto-renews before expiry via a systemd timer or cron job

On Ubuntu, the typical setup is:

```bash
sudo apt install certbot python3-certbot-nginx
sudo certbot --nginx -d example.com
```

After that, a systemd timer runs `certbot renew` twice daily. Certs are renewed when they're within 30 days of expiry. With 90-day certificates, this means renewal happens roughly every 60 days automatically.

## Boulder (ACME CA Server)

- **Repository**: https://github.com/letsencrypt/boulder
- **Language**: Go
- **Role**: Certificate authority that validates requests and issues certificates
- **Analogous to**: our `server/` directory

Boulder is Let's Encrypt's server-side CA. It:

1. Receives ACME requests from clients (Certbot, etc.)
2. Issues domain validation challenges
3. Verifies the challenges were completed
4. Signs the certificate with the CA's private key
5. Submits the certificate to Certificate Transparency logs
6. Returns the signed certificate to the client

Boulder handles millions of certificates and is the production CA behind Let's Encrypt.

## How they relate to this project

| Component | Let's Encrypt | This project |
|---|---|---|
| CA server | Boulder (Go) | `server/` (Python) |
| Client | Certbot (Python) | `client/` (Python) |
| Protocol | ACME (RFC 8555) | HTTP REST API |
| Certificate format | X.509 | MTC (Merkle Tree Certificates) |
| Trust model | CA signature + CT logs | Issuance log + inclusion proofs + cosignatures |
| Transparency | CT logs (separate from CA) | Integrated issuance log (Merkle tree is the CA's log) |
| Renewal | Automated via cron/systemd | Not yet implemented |

The key architectural difference: in Let's Encrypt, the CA signs each certificate individually and then submits it to separate CT logs. In MTC, the CA certifies entries by adding them to its own Merkle tree log and cosigning the tree — signatures are amortized across many certificates rather than repeated per certificate.

## What we could borrow

- **ACME protocol**: Our server could implement ACME endpoints so standard clients like Certbot could request MTC certificates
- **Automated renewal**: A systemd timer or cron job that re-enrolls before expiry, similar to `certbot renew`
- **Challenge validation**: Domain validation challenges for public-facing certificates
- **Rate limiting and abuse prevention**: Boulder's approach to preventing certificate flooding

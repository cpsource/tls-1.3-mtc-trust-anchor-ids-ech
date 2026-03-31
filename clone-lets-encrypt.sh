#!/bin/bash
# Clone Let's Encrypt / ACME reference implementations into this repository.
#
# certbot  - EFF's ACME client (Python) - requests and renews certs
# boulder  - Let's Encrypt's CA server (Go) - validates and issues certs
#
# Usage: ./clone-lets-encrypt.sh

set -e

cd "$(dirname "$0")"

if [ -d certbot ]; then
    echo "certbot/ already exists, skipping"
else
    echo "Cloning certbot (ACME client)..."
    git clone https://github.com/certbot/certbot.git
fi

if [ -d boulder ]; then
    echo "boulder/ already exists, skipping"
else
    echo "Cloning boulder (Let's Encrypt CA server)..."
    git clone https://github.com/letsencrypt/boulder.git
fi

echo "Done."
echo "  certbot/ - ACME client (analogous to our client/)"
echo "  boulder/ - ACME CA server (analogous to our server/)"

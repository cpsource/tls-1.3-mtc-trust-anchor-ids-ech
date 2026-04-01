#!/bin/bash
# Clone reference repositories used by this project.
#
# certbot  - EFF's ACME client (Python) - requests and renews certs
# boulder  - Let's Encrypt's CA server (Go) - validates and issues certs
# liboqs   - Open Quantum Safe post-quantum crypto library (C)
#
# Usage: ./clone-repositories.sh

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

if [ -d liboqs ]; then
    echo "liboqs/ already exists, skipping"
else
    echo "Cloning liboqs (post-quantum crypto library)..."
    git clone https://github.com/open-quantum-safe/liboqs.git
fi

echo ""
echo "Done."
echo "  certbot/ - ACME client (analogous to our client/)"
echo "  boulder/ - ACME CA server (analogous to our server/)"
echo "  liboqs/  - Post-quantum crypto: ML-KEM, ML-DSA, FALCON, SPHINCS+, etc."

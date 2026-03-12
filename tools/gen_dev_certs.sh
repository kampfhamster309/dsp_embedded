#!/usr/bin/env bash
# gen_dev_certs.sh – Generate DEVELOPMENT-ONLY device certificate and key.
#
# Output: components/dsp_identity/certs/device_cert.der
#         components/dsp_identity/certs/device_key.der
#
# THESE CREDENTIALS ARE FOR DEVELOPMENT AND TESTING ONLY.
# DO NOT use them in production. Each production device must be provisioned
# with a unique certificate signed by the deployment CA.
#
# Prerequisites: openssl (>= 1.1)
# Usage: bash tools/gen_dev_certs.sh [--force]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
CERT_DIR="$REPO_ROOT/components/dsp_identity/certs"

CERT_DER="$CERT_DIR/device_cert.der"
KEY_DER="$CERT_DIR/device_key.der"

FORCE=0
if [[ "${1:-}" == "--force" ]]; then
    FORCE=1
fi

if [[ -f "$CERT_DER" && -f "$KEY_DER" && $FORCE -eq 0 ]]; then
    echo "[gen_dev_certs] Certificates already exist. Use --force to regenerate."
    echo "  cert: $CERT_DER"
    echo "  key:  $KEY_DER"
    exit 0
fi

echo "[gen_dev_certs] Generating ECDSA P-256 development certificate..."

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# 1. Generate EC P-256 private key
openssl ecparam -genkey -name prime256v1 -noout -out "$TMP/dev_key.pem"

# 2. Self-signed certificate (CN=dsp-embedded-dev, 10-year validity)
openssl req -new -x509 \
    -key "$TMP/dev_key.pem" \
    -out "$TMP/dev_cert.pem" \
    -days 3650 \
    -subj "/C=DE/O=DSP-Embedded-Dev/CN=dsp-embedded-dev"

# 3. Convert to DER
openssl x509 -in "$TMP/dev_cert.pem" -outform DER -out "$CERT_DER"
openssl ec    -in "$TMP/dev_key.pem"  -outform DER -out "$KEY_DER"

CERT_SIZE=$(wc -c < "$CERT_DER")
KEY_SIZE=$(wc -c < "$KEY_DER")

echo "[gen_dev_certs] Done."
echo "  cert: $CERT_DER  (${CERT_SIZE} bytes)"
echo "  key:  $KEY_DER   (${KEY_SIZE} bytes)"
echo ""
echo "WARNING: These are DEV-ONLY credentials. Do not flash to production devices."

#!/usr/bin/env bash
# seed-test-data.sh
#
# Verifies the DSP Counterpart is healthy and logs its state.
# When the DSP Embedded node is available, runs a catalog fetch smoke test.
#
# Usage:
#   ./seed/seed-test-data.sh [PROVIDER_URL]
#
# Examples:
#   ./seed/seed-test-data.sh                          # uses default localhost:80
#   ./seed/seed-test-data.sh http://192.168.1.100:80  # real ESP32-S3 on WiFi

set -euo pipefail

COUNTERPART_URL="${COUNTERPART_URL:-http://localhost:18000}"
PROVIDER_URL="${1:-http://localhost:80}"

echo "=== DSP Embedded – Integration Test Seed ==="
echo "Counterpart : $COUNTERPART_URL"
echo "Provider    : $PROVIDER_URL"
echo ""

# ---- 1. Wait for counterpart health ----------------------------------------
echo "[1/3] Waiting for DSP counterpart to be healthy..."
for i in $(seq 1 20); do
    if curl -sf "$COUNTERPART_URL/health" > /dev/null 2>&1; then
        STATUS=$(curl -s "$COUNTERPART_URL/health" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d['status'])")
        echo "      Counterpart healthy (status=$STATUS)"
        break
    fi
    echo "      Attempt $i/20 – retrying in 2s..."
    sleep 2
    if [ "$i" -eq 20 ]; then
        echo "ERROR: Counterpart did not become healthy in time."
        exit 1
    fi
done

# ---- 2. Probe provider reachability ----------------------------------------
echo ""
echo "[2/3] Probing provider at $PROVIDER_URL ..."
if curl -sf "$PROVIDER_URL/catalog" > /dev/null 2>&1; then
    echo "      Provider reachable – fetching catalog via counterpart..."
    CATALOG=$(curl -s -X POST "$COUNTERPART_URL/api/test/catalog" \
        -H "Content-Type: application/json" \
        -d "{\"provider_url\": \"$PROVIDER_URL\"}")
    echo "      Catalog response:"
    echo "$CATALOG" | python3 -m json.tool 2>/dev/null || echo "$CATALOG"
else
    echo "      Provider not reachable at $PROVIDER_URL (not started yet)."
    echo "      This is expected during Milestone 0 scaffolding."
    echo "      Run the host-native dsp_test_runner or flash firmware before seeding."
fi

# ---- 3. Summary ------------------------------------------------------------
echo ""
echo "[3/3] Environment summary:"
curl -s "$COUNTERPART_URL/health" | python3 -m json.tool 2>/dev/null \
    || echo "  (counterpart unreachable)"

echo ""
echo "=== Seed complete. ==="
echo ""
echo "Integration test control API:"
echo "  POST $COUNTERPART_URL/api/test/catalog        – fetch provider catalog"
echo "  POST $COUNTERPART_URL/api/test/negotiate      – start contract negotiation"
echo "  POST $COUNTERPART_URL/api/test/transfer       – start data transfer"
echo "  GET  $COUNTERPART_URL/api/test/negotiations   – list tracked negotiations"
echo "  GET  $COUNTERPART_URL/api/test/transfers      – list tracked transfers"

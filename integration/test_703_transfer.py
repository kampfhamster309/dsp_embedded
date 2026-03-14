"""
DSP-703 Integration test: transfer flow (start → status → completion).

Acceptance criteria (from development_plan.md):
  - The transfer integration test drives the full start → status → completion sequence.
  - Both DSP Embedded node and EDC connector (consumer mock) reach COMPLETED state.
  - Test exits 0.

Design note:
  DSP_NEG_MAX=4 and DSP_XFER_MAX=2.  DSP-702 already consumes 2 negotiation slots
  when the full test suite runs.  DSP-703 uses the remaining 2 slots (one per module-
  scoped fixture) and both available transfer slots.  Each fixture runs its entire
  negotiate → agree → start → complete flow exactly once; individual tests check
  specific properties of the cached responses.
"""

import uuid as _uuid

import httpx
import pytest

# -------------------------------------------------------------------------
# DSP / JSON-LD constants (must match dsp_jsonld_ctx.h and dsp_build.c)
# -------------------------------------------------------------------------
JSONLD_CONTEXT_URL       = "https://w3id.org/dspace/2024/1/context.json"

TYPE_TRANSFER_START      = "dspace:TransferStartMessage"
TYPE_TRANSFER_COMPLETION = "dspace:TransferCompletionMessage"
TYPE_NEG_EVENT           = "dspace:ContractNegotiationEventMessage"

NEG_STATE_OFFERED        = "dspace:OFFERED"
NEG_STATE_AGREED         = "dspace:AGREED"

FIELD_TYPE               = "@type"
FIELD_PROCESS_ID         = "dspace:processId"


def _make_offer_body(process_id: str) -> dict:
    """Build a ContractOfferMessage that the device accepts."""
    return {
        "@context": [JSONLD_CONTEXT_URL],
        "@type": "dspace:ContractOfferMessage",
        "dspace:processId": f"urn:uuid:{process_id}",
        "dspace:offer": {
            "@type": "odrl:Offer",
            "@id": f"urn:uuid:{process_id}-offer",
            "odrl:assigner": {"@id": "urn:connector:test"},
            "odrl:target": {"@id": "sensor-data-01"},
            "odrl:permission": [
                {
                    "odrl:action": {"odrl:type": "odrl:use"},
                    "odrl:constraint": [],
                }
            ],
        },
        "dspace:callbackAddress": "http://localhost:18000/callback",
    }


def _make_transfer_body(process_id: str, asset_id: str = "sensor-data-01") -> dict:
    """Build a TransferRequestMessage that passes device validation.

    The device's dsp_msg_validate_transfer_request uses cJSON_IsString for
    @context, so @context must be a plain string (not an array).
    """
    return {
        "@context": JSONLD_CONTEXT_URL,
        "@type": "dspace:TransferRequestMessage",
        "dspace:processId": f"urn:uuid:{process_id}",
        "dspace:dataset": asset_id,
        "dspace:callbackAddress": "http://localhost:18000/callback",
    }


# =========================================================================
# Module-scoped fixtures – run each full flow once, cache for all tests
# =========================================================================

@pytest.fixture(scope="module")
def direct_xfer_flow(provider_url):
    """Full negotiate → agree → start → complete flow, driven directly.

    Uses one negotiation slot and one transfer slot on the device.
    """
    pid = str(_uuid.uuid4())
    neg_id = f"urn:uuid:{pid}"

    with httpx.Client(timeout=15.0) as client:
        # 1. Offer
        offer_resp = client.post(
            f"{provider_url}/negotiations/offers",
            json=_make_offer_body(pid),
        )
    assert offer_resp.status_code == 201, (
        f"Setup (offer) failed: {offer_resp.status_code}: {offer_resp.text[:300]}"
    )

    with httpx.Client(timeout=10.0) as client:
        # 2. Agree
        agree_resp = client.post(f"{provider_url}/negotiations/{neg_id}/agree")
    assert agree_resp.status_code == 200, (
        f"Setup (agree) failed: {agree_resp.status_code}: {agree_resp.text[:300]}"
    )

    with httpx.Client(timeout=10.0) as client:
        # 3. Start transfer
        start_resp = client.post(
            f"{provider_url}/transfers/start",
            json=_make_transfer_body(pid),
        )

    with httpx.Client(timeout=10.0) as client:
        # 4. GET state after start
        get_after_start = client.get(f"{provider_url}/transfers/{neg_id}")

    with httpx.Client(timeout=10.0) as client:
        # 5. Complete transfer
        complete_resp = client.post(f"{provider_url}/transfers/{neg_id}/complete")

    with httpx.Client(timeout=10.0) as client:
        # 6. GET state after completion
        get_after_complete = client.get(f"{provider_url}/transfers/{neg_id}")

    return {
        "pid": pid,
        "neg_id": neg_id,
        "offer_resp": offer_resp,
        "agree_resp": agree_resp,
        "start_resp": start_resp,
        "start_body": start_resp.json() if start_resp.status_code == 200 else {},
        "get_after_start": get_after_start,
        "get_after_start_body": get_after_start.json(),
        "complete_resp": complete_resp,
        "complete_body": complete_resp.json() if complete_resp.status_code == 200 else {},
        "get_after_complete": get_after_complete,
        "get_after_complete_body": get_after_complete.json(),
    }


@pytest.fixture(scope="module")
def counterpart_xfer_flow(counterpart_url, provider_url):
    """Full negotiate → agree → start → complete flow via the DSP consumer mock.

    Uses one negotiation slot and one transfer slot on the device.
    """
    # 1. Negotiate
    with httpx.Client(timeout=15.0) as client:
        neg_resp = client.post(
            f"{counterpart_url}/api/test/negotiate",
            json={"provider_url": provider_url},
        )
    assert neg_resp.status_code == 200, (
        f"Counterpart negotiate failed: {neg_resp.status_code}: {neg_resp.text[:300]}"
    )
    consumer_pid = neg_resp.json()["consumer_pid"]

    # 2. Agree
    with httpx.Client(timeout=15.0) as client:
        agree_resp = client.post(
            f"{counterpart_url}/api/test/agree",
            json={"provider_url": provider_url, "consumer_pid": consumer_pid},
        )
    assert agree_resp.status_code == 200, (
        f"Counterpart agree failed: {agree_resp.status_code}: {agree_resp.text[:300]}"
    )

    # 3. Start transfer (consumer_pid matches the AGREED negotiation)
    with httpx.Client(timeout=15.0) as client:
        start_resp = client.post(
            f"{counterpart_url}/api/test/transfer",
            json={"provider_url": provider_url, "consumer_pid": consumer_pid},
        )

    # 4. Complete transfer
    with httpx.Client(timeout=15.0) as client:
        complete_resp = client.post(
            f"{counterpart_url}/api/test/transfer-complete",
            json={"provider_url": provider_url, "consumer_pid": consumer_pid},
        )

    neg_id = f"urn:uuid:{consumer_pid}"
    with httpx.Client(timeout=10.0) as client:
        provider_get = client.get(f"{provider_url}/transfers/{neg_id}")

    return {
        "consumer_pid": consumer_pid,
        "neg_id": neg_id,
        "neg_resp": neg_resp,
        "agree_resp": agree_resp,
        "start_resp": start_resp,
        "start_body": start_resp.json() if start_resp.status_code == 200 else {},
        "complete_resp": complete_resp,
        "complete_body": complete_resp.json() if complete_resp.status_code == 200 else {},
        "provider_get": provider_get,
        "provider_get_body": provider_get.json(),
    }


# =========================================================================
# Perspective 1 – direct transfer flow
# =========================================================================

class TestTransferDirect:
    """Verify each step of the start → complete state machine directly."""

    def test_transfer_start_returns_200(self, direct_xfer_flow):
        """POST /transfers/start returns HTTP 200 OK."""
        assert direct_xfer_flow["start_resp"].status_code == 200, (
            f"Expected 200 from transfer start, got "
            f"{direct_xfer_flow['start_resp'].status_code}: "
            f"{direct_xfer_flow['start_resp'].text[:300]}"
        )

    def test_transfer_start_response_is_json(self, direct_xfer_flow):
        """POST /transfers/start response body is valid JSON."""
        assert isinstance(direct_xfer_flow["start_body"], dict)

    def test_transfer_start_type_is_transfer_start_message(self, direct_xfer_flow):
        """Start response @type equals dspace:TransferStartMessage."""
        body = direct_xfer_flow["start_body"]
        assert body.get(FIELD_TYPE) == TYPE_TRANSFER_START, (
            f"Expected @type={TYPE_TRANSFER_START!r}, got {body.get(FIELD_TYPE)!r}"
        )

    def test_transfer_start_process_id_echoed(self, direct_xfer_flow):
        """Start response echoes back the correct dspace:processId."""
        body = direct_xfer_flow["start_body"]
        pid_in_body = body.get(FIELD_PROCESS_ID, "")
        assert pid_in_body == direct_xfer_flow["neg_id"], (
            f"Expected processId {direct_xfer_flow['neg_id']!r}, got {pid_in_body!r}"
        )

    def test_get_state_after_start_returns_200(self, direct_xfer_flow):
        """GET /transfers/{id} returns 200 after start."""
        assert direct_xfer_flow["get_after_start"].status_code == 200

    def test_get_state_after_start_is_transferring(self, direct_xfer_flow):
        """GET /transfers/{id} returns TransferStartMessage (TRANSFERRING) after start."""
        body = direct_xfer_flow["get_after_start_body"]
        assert body.get(FIELD_TYPE) == TYPE_TRANSFER_START, (
            f"Expected {TYPE_TRANSFER_START!r} after start, got: {body}"
        )

    def test_transfer_complete_returns_200(self, direct_xfer_flow):
        """POST /transfers/{id}/complete returns HTTP 200 OK."""
        assert direct_xfer_flow["complete_resp"].status_code == 200, (
            f"Expected 200 from transfer complete, got "
            f"{direct_xfer_flow['complete_resp'].status_code}: "
            f"{direct_xfer_flow['complete_resp'].text[:300]}"
        )

    def test_transfer_complete_response_is_json(self, direct_xfer_flow):
        """POST /transfers/{id}/complete response body is valid JSON."""
        assert isinstance(direct_xfer_flow["complete_body"], dict)

    def test_transfer_complete_type_is_completion_message(self, direct_xfer_flow):
        """Complete response @type equals dspace:TransferCompletionMessage."""
        body = direct_xfer_flow["complete_body"]
        assert body.get(FIELD_TYPE) == TYPE_TRANSFER_COMPLETION, (
            f"Expected @type={TYPE_TRANSFER_COMPLETION!r}, got {body.get(FIELD_TYPE)!r}"
        )

    def test_transfer_complete_process_id_echoed(self, direct_xfer_flow):
        """Complete response echoes back the correct dspace:processId."""
        body = direct_xfer_flow["complete_body"]
        pid_in_body = body.get(FIELD_PROCESS_ID, "")
        assert pid_in_body == direct_xfer_flow["neg_id"], (
            f"Expected processId {direct_xfer_flow['neg_id']!r}, got {pid_in_body!r}"
        )

    def test_get_state_after_complete_returns_200(self, direct_xfer_flow):
        """GET /transfers/{id} returns 200 after completion."""
        assert direct_xfer_flow["get_after_complete"].status_code == 200

    def test_get_state_after_complete_is_completed(self, direct_xfer_flow):
        """GET /transfers/{id} returns TransferCompletionMessage after completion (DSP-703 primary AC)."""
        body = direct_xfer_flow["get_after_complete_body"]
        assert body.get(FIELD_TYPE) == TYPE_TRANSFER_COMPLETION, (
            f"DSP-703 FAIL: expected @type={TYPE_TRANSFER_COMPLETION!r} after complete, got: {body}"
        )

    def test_complete_on_unknown_id_returns_404(self, provider_url):
        """POST /transfers/{unknown}/complete returns 404 for non-existent transfer."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.post(
                f"{provider_url}/transfers/"
                "urn:uuid:00000000-0000-0000-0000-000000000000/complete"
            )
        assert resp.status_code == 404, (
            f"Expected 404 for unknown transfer, got {resp.status_code}"
        )

    def test_start_without_agreed_negotiation_returns_400(self, provider_url):
        """POST /transfers/start returns 400 when negotiation is not AGREED."""
        bad_pid = str(_uuid.uuid4())
        with httpx.Client(timeout=10.0) as client:
            resp = client.post(
                f"{provider_url}/transfers/start",
                json=_make_transfer_body(bad_pid),
            )
        assert resp.status_code == 400, (
            f"Expected 400 for non-AGREED negotiation, got {resp.status_code}"
        )


# =========================================================================
# Perspective 2 – transfer flow via DSP counterpart consumer mock
# =========================================================================

class TestTransferViaCounterpart:
    """Drive the full start → complete flow through the DSP consumer mock."""

    def test_counterpart_transfer_start_returns_200(self, counterpart_xfer_flow):
        """POST /api/test/transfer on counterpart returns 200."""
        assert counterpart_xfer_flow["start_resp"].status_code == 200, (
            f"Transfer start failed: {counterpart_xfer_flow['start_resp'].status_code}: "
            f"{counterpart_xfer_flow['start_resp'].text[:300]}"
        )

    def test_counterpart_start_reaches_provider(self, counterpart_xfer_flow):
        """Provider returns TransferStartMessage to counterpart's transfer start."""
        provider_resp = counterpart_xfer_flow["start_body"].get("provider_response", {})
        assert provider_resp.get(FIELD_TYPE) == TYPE_TRANSFER_START, (
            f"Expected provider TransferStartMessage, got: {provider_resp}"
        )

    def test_counterpart_transfer_complete_returns_200(self, counterpart_xfer_flow):
        """POST /api/test/transfer-complete on counterpart returns 200."""
        assert counterpart_xfer_flow["complete_resp"].status_code == 200, (
            f"Transfer complete failed: {counterpart_xfer_flow['complete_resp'].status_code}: "
            f"{counterpart_xfer_flow['complete_resp'].text[:300]}"
        )

    def test_provider_complete_response_is_completion_message(self, counterpart_xfer_flow):
        """Provider's complete response contains TransferCompletionMessage."""
        provider_resp = counterpart_xfer_flow["complete_body"].get("provider_response", {})
        assert provider_resp.get(FIELD_TYPE) == TYPE_TRANSFER_COMPLETION, (
            f"Expected provider TransferCompletionMessage, got: {provider_resp}"
        )

    def test_provider_state_completed_after_counterpart_flow(self, counterpart_xfer_flow):
        """GET /transfers/{id} on provider returns COMPLETED after counterpart-driven flow.

        This is the DSP-703 primary acceptance criterion from the counterpart perspective:
        the final GET returns @type = dspace:TransferCompletionMessage.
        """
        body = counterpart_xfer_flow["provider_get_body"]
        assert body.get(FIELD_TYPE) == TYPE_TRANSFER_COMPLETION, (
            f"DSP-703 FAIL: provider state after counterpart flow: "
            f"expected @type={TYPE_TRANSFER_COMPLETION!r}, got {body.get(FIELD_TYPE)!r}"
        )

    def test_counterpart_state_completed_after_flow(self, counterpart_xfer_flow):
        """Counterpart's internal state is COMPLETED after the full flow.

        Simulates the EDC connector reaching COMPLETED state without error.
        """
        complete_body = counterpart_xfer_flow["complete_body"]
        counterpart_xfer = complete_body.get("transfer") or {}
        assert counterpart_xfer.get("state") == "COMPLETED", (
            f"Counterpart transfer state not COMPLETED after flow: {counterpart_xfer}"
        )

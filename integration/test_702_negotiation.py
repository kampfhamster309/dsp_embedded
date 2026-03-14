"""
DSP-702 Integration test: negotiation flow (offer → agree).

Acceptance criteria (from development_plan.md):
  - The negotiation integration test drives the full offer → agree sequence.
  - The final GET /negotiations/{id} response from the DSP Embedded node
    contains dspace:eventType = dspace:AGREED.
  - The DSP counterpart (consumer mock) reaches its equivalent AGREED state
    without error.

Design note:
  The DSP Embedded node has a fixed-size negotiation table (CONFIG_DSP_MAX_CONCURRENT_NEGOTIATIONS,
  default 4) that is never flushed while the device is running.  To stay within
  this budget, each test class runs the full offer → agree flow exactly ONCE via a
  module-scoped fixture, then each individual test checks a specific property of
  the cached response.  Two module-scoped flows are needed:
    - one driven directly against the provider
    - one driven via the DSP counterpart mock
  That consumes 2 of the 4 available slots.
"""

import time
import uuid as _uuid

import httpx
import pytest

# -------------------------------------------------------------------------
# DSP / JSON-LD constants (must match dsp_jsonld_ctx.h)
# -------------------------------------------------------------------------
JSONLD_CONTEXT_URL       = "https://w3id.org/dspace/2024/1/context.json"
NEG_EVENT_TYPE           = "dspace:ContractNegotiationEventMessage"
NEG_STATE_OFFERED        = "dspace:OFFERED"
NEG_STATE_AGREED         = "dspace:AGREED"

FIELD_STATE              = "dspace:eventType"   # device uses eventType for state
FIELD_PROCESS_ID         = "dspace:processId"
FIELD_TYPE               = "@type"


def _make_offer_body(process_id: str) -> dict:
    """Build a minimal ContractOfferMessage accepted by the device.

    The device reads dspace:processId (not dspace:consumerPid) to identify
    the negotiation slot.
    """
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


# =========================================================================
# Module-scoped fixtures – run the flow once, cache results for all tests
# =========================================================================

@pytest.fixture(scope="module")
def direct_neg_flow(provider_url):
    """Run the complete offer → agree flow directly against the provider.

    Returns a dict of responses at each step.  Uses exactly one negotiation
    slot on the device.
    """
    pid = str(_uuid.uuid4())
    neg_id = f"urn:uuid:{pid}"

    with httpx.Client(timeout=15.0) as client:
        offer_resp = client.post(
            f"{provider_url}/negotiations/offers",
            json=_make_offer_body(pid),
        )

    assert offer_resp.status_code == 201, (
        f"Setup (offer) failed: {offer_resp.status_code}: {offer_resp.text[:300]}"
    )

    with httpx.Client(timeout=10.0) as client:
        get_after_offer = client.get(f"{provider_url}/negotiations/{neg_id}")

    with httpx.Client(timeout=10.0) as client:
        agree_resp = client.post(f"{provider_url}/negotiations/{neg_id}/agree")

    with httpx.Client(timeout=10.0) as client:
        get_after_agree = client.get(f"{provider_url}/negotiations/{neg_id}")

    return {
        "pid": pid,
        "neg_id": neg_id,
        "offer_resp": offer_resp,
        "offer_body": offer_resp.json(),
        "get_after_offer": get_after_offer,
        "get_after_offer_body": get_after_offer.json(),
        "agree_resp": agree_resp,
        "agree_body": agree_resp.json() if agree_resp.status_code == 200 else {},
        "get_after_agree": get_after_agree,
        "get_after_agree_body": get_after_agree.json(),
    }


@pytest.fixture(scope="module")
def counterpart_neg_flow(counterpart_url, provider_url):
    """Run the complete offer → agree flow via the DSP consumer mock.

    Returns a dict of intermediate and final results.  Uses exactly one
    negotiation slot on the device.
    """
    with httpx.Client(timeout=15.0) as client:
        neg_resp = client.post(
            f"{counterpart_url}/api/test/negotiate",
            json={"provider_url": provider_url},
        )

    assert neg_resp.status_code == 200, (
        f"Counterpart negotiate failed: {neg_resp.status_code}: {neg_resp.text[:300]}"
    )
    neg_body = neg_resp.json()
    consumer_pid = neg_body["consumer_pid"]

    with httpx.Client(timeout=15.0) as client:
        agree_resp = client.post(
            f"{counterpart_url}/api/test/agree",
            json={"provider_url": provider_url, "consumer_pid": consumer_pid},
        )

    neg_id = f"urn:uuid:{consumer_pid}"
    with httpx.Client(timeout=10.0) as client:
        provider_get = client.get(f"{provider_url}/negotiations/{neg_id}")

    return {
        "consumer_pid": consumer_pid,
        "neg_id": neg_id,
        "neg_resp": neg_resp,
        "neg_body": neg_body,
        "provider_offer_resp": neg_body.get("provider_response", {}),
        "agree_resp": agree_resp,
        "agree_body": agree_resp.json() if agree_resp.status_code == 200 else {},
        "provider_get": provider_get,
        "provider_get_body": provider_get.json(),
    }


# =========================================================================
# Perspective 1 – direct negotiation flow
# =========================================================================

class TestNegotiationDirect:
    """Verify each step of the offer → agree state machine directly."""

    def test_offer_returns_201(self, direct_neg_flow):
        """POST /negotiations/offers returns HTTP 201 Created."""
        assert direct_neg_flow["offer_resp"].status_code == 201

    def test_offer_response_parses_as_json(self, direct_neg_flow):
        """POST /negotiations/offers response body is valid JSON."""
        body = direct_neg_flow["offer_body"]
        assert isinstance(body, dict)

    def test_offer_response_state_is_offered(self, direct_neg_flow):
        """Offer response dspace:eventType equals dspace:OFFERED."""
        body = direct_neg_flow["offer_body"]
        assert body.get(FIELD_STATE) == NEG_STATE_OFFERED, (
            f"Expected {FIELD_STATE}={NEG_STATE_OFFERED!r}, got: {body}"
        )

    def test_offer_response_process_id_echoed(self, direct_neg_flow):
        """Offer response echoes back the correct dspace:processId."""
        body = direct_neg_flow["offer_body"]
        pid_in_body = body.get(FIELD_PROCESS_ID, "")
        assert pid_in_body == direct_neg_flow["neg_id"], (
            f"Expected processId {direct_neg_flow['neg_id']!r}, got {pid_in_body!r}"
        )

    def test_offer_response_type_is_negotiation_event(self, direct_neg_flow):
        """Offer response @type equals dspace:ContractNegotiationEventMessage."""
        body = direct_neg_flow["offer_body"]
        assert body.get(FIELD_TYPE) == NEG_EVENT_TYPE, (
            f"Expected @type={NEG_EVENT_TYPE!r}, got {body.get(FIELD_TYPE)!r}"
        )

    def test_get_state_after_offer_returns_200(self, direct_neg_flow):
        """GET /negotiations/{id} returns 200 after offer."""
        assert direct_neg_flow["get_after_offer"].status_code == 200

    def test_get_state_after_offer_is_offered(self, direct_neg_flow):
        """GET /negotiations/{id} returns dspace:OFFERED after offer."""
        body = direct_neg_flow["get_after_offer_body"]
        assert body.get(FIELD_STATE) == NEG_STATE_OFFERED, (
            f"Expected {FIELD_STATE}={NEG_STATE_OFFERED!r} after offer, got: {body}"
        )

    def test_agree_returns_200(self, direct_neg_flow):
        """POST /negotiations/{id}/agree returns HTTP 200 OK."""
        assert direct_neg_flow["agree_resp"].status_code == 200, (
            f"Expected 200 from agree, got {direct_neg_flow['agree_resp'].status_code}: "
            f"{direct_neg_flow['agree_resp'].text[:300]}"
        )

    def test_agree_response_state_is_agreed(self, direct_neg_flow):
        """Agree response dspace:eventType equals dspace:AGREED."""
        body = direct_neg_flow["agree_body"]
        assert body.get(FIELD_STATE) == NEG_STATE_AGREED, (
            f"Expected {FIELD_STATE}={NEG_STATE_AGREED!r} in agree response, got: {body}"
        )

    def test_get_state_after_agree_returns_200(self, direct_neg_flow):
        """GET /negotiations/{id} returns 200 after agree."""
        assert direct_neg_flow["get_after_agree"].status_code == 200

    def test_get_state_after_agree_is_agreed(self, direct_neg_flow):
        """GET /negotiations/{id} returns dspace:AGREED after agree (DSP-702 primary AC)."""
        body = direct_neg_flow["get_after_agree_body"]
        assert body.get(FIELD_STATE) == NEG_STATE_AGREED, (
            f"DSP-702 FAIL: expected {FIELD_STATE}={NEG_STATE_AGREED!r}, got: {body}"
        )

    def test_agree_on_unknown_id_returns_404(self, provider_url):
        """POST /negotiations/{unknown}/agree returns 404 for non-existent negotiation."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.post(
                f"{provider_url}/negotiations/"
                "urn:uuid:00000000-0000-0000-0000-000000000000/agree"
            )
        assert resp.status_code == 404, (
            f"Expected 404 for unknown negotiation, got {resp.status_code}"
        )


# =========================================================================
# Perspective 2 – negotiation flow via DSP counterpart consumer mock
# =========================================================================

class TestNegotiationViaCounterpart:
    """Drive the full offer → agree flow through the DSP consumer mock."""

    def test_counterpart_negotiate_returns_200(self, counterpart_neg_flow):
        """POST /api/test/negotiate on counterpart returns 200."""
        assert counterpart_neg_flow["neg_resp"].status_code == 200

    def test_counterpart_offer_reaches_provider(self, counterpart_neg_flow):
        """Provider returns dspace:OFFERED to counterpart's negotiate call."""
        provider_resp = counterpart_neg_flow["provider_offer_resp"]
        assert provider_resp.get(FIELD_STATE) == NEG_STATE_OFFERED, (
            f"Expected provider OFFERED state, got: {provider_resp}"
        )

    def test_counterpart_agree_returns_200(self, counterpart_neg_flow):
        """POST /api/test/agree on counterpart returns 200."""
        assert counterpart_neg_flow["agree_resp"].status_code == 200, (
            f"Agree step failed: {counterpart_neg_flow['agree_resp'].status_code}: "
            f"{counterpart_neg_flow['agree_resp'].text[:300]}"
        )

    def test_provider_agree_response_is_agreed(self, counterpart_neg_flow):
        """Provider's agree response contains dspace:AGREED."""
        provider_resp = counterpart_neg_flow["agree_body"].get("provider_response", {})
        assert provider_resp.get(FIELD_STATE) == NEG_STATE_AGREED, (
            f"Expected provider AGREED in agree response, got: {provider_resp}"
        )

    def test_provider_state_agreed_after_counterpart_flow(self, counterpart_neg_flow):
        """GET /negotiations/{id} on provider returns AGREED after counterpart-driven flow.

        This is the DSP-702 primary acceptance criterion from the counterpart perspective:
        the final GET contains dspace:eventType = dspace:AGREED.
        """
        body = counterpart_neg_flow["provider_get_body"]
        assert body.get(FIELD_STATE) == NEG_STATE_AGREED, (
            f"DSP-702 FAIL: provider state after counterpart flow: "
            f"expected {NEG_STATE_AGREED!r}, got {body.get(FIELD_STATE)!r}"
        )

    def test_counterpart_state_agreed_after_flow(self, counterpart_neg_flow):
        """Counterpart's internal state is AGREED after the full flow.

        Simulates the EDC connector reaching AGREED state without error.
        """
        agree_body = counterpart_neg_flow["agree_body"]
        counterpart_neg = agree_body.get("negotiation") or {}
        assert counterpart_neg.get("state") == "AGREED", (
            f"Counterpart state not AGREED after flow: {counterpart_neg}"
        )

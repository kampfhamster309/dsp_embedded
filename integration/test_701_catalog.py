"""
DSP-701 Integration test: catalog fetch.

Acceptance criteria (from development_plan.md):
  - Completes end-to-end in under 30 seconds on a developer machine.
  - The provider's response to GET /catalog parses as valid JSON-LD with
    the correct @type = dcat:Catalog.

Two test perspectives are covered:
  1. Direct – the test client hits GET /catalog on the DSP Embedded node
     directly and validates the JSON-LD structure.
  2. Via counterpart – the test drives the DSP consumer mock (dsp-counterpart)
     to fetch the catalog, simulating the full consumer-side flow.
"""

import time

import httpx
import pytest

# -------------------------------------------------------------------------
# DSP / JSON-LD constants (must match dsp_jsonld_ctx.h)
# -------------------------------------------------------------------------
JSONLD_CONTEXT_URL = "https://w3id.org/dspace/2024/1/context.json"
CATALOG_TYPE       = "dcat:Catalog"
DATASET_TYPE       = "dcat:Dataset"


# =========================================================================
# Perspective 1 – direct catalog fetch
# =========================================================================

class TestCatalogDirect:
    """Hit GET /catalog on the provider node directly."""

    def test_returns_http_200(self, provider_url):
        """GET /catalog returns HTTP 200 OK."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.get(f"{provider_url}/catalog")
        assert resp.status_code == 200, (
            f"Expected 200, got {resp.status_code}: {resp.text[:200]}"
        )

    def test_content_type_is_json(self, provider_url):
        """GET /catalog responds with application/json content-type."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.get(f"{provider_url}/catalog")
        ct = resp.headers.get("content-type", "")
        assert "application/json" in ct, f"Unexpected content-type: {ct}"

    def test_response_parses_as_json(self, provider_url):
        """GET /catalog response body is valid JSON."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.get(f"{provider_url}/catalog")
        # Must not raise
        body = resp.json()
        assert isinstance(body, dict), "Expected a JSON object at the top level"

    def test_jsonld_context_present(self, provider_url):
        """Catalog response contains the DSP JSON-LD @context URL."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.get(f"{provider_url}/catalog")
        body = resp.json()
        context = body.get("@context", [])
        if isinstance(context, str):
            context = [context]
        assert any(JSONLD_CONTEXT_URL in str(c) for c in context), (
            f"@context does not include {JSONLD_CONTEXT_URL!r}. Got: {context}"
        )

    def test_type_is_dcat_catalog(self, provider_url):
        """Catalog @type equals dcat:Catalog (DSP-701 primary AC)."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.get(f"{provider_url}/catalog")
        body = resp.json()
        assert body.get("@type") == CATALOG_TYPE, (
            f"Expected @type={CATALOG_TYPE!r}, got {body.get('@type')!r}"
        )

    def test_dataset_array_non_empty(self, provider_url):
        """Catalog contains at least one dcat:dataset entry."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.get(f"{provider_url}/catalog")
        body = resp.json()
        datasets = body.get("dcat:dataset", [])
        if isinstance(datasets, dict):
            datasets = [datasets]
        assert len(datasets) >= 1, (
            f"Expected at least one dcat:dataset entry, got: {datasets}"
        )

    def test_dataset_has_type(self, provider_url):
        """Each dcat:dataset entry has @type = dcat:Dataset."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.get(f"{provider_url}/catalog")
        body = resp.json()
        datasets = body.get("dcat:dataset", [])
        if isinstance(datasets, dict):
            datasets = [datasets]
        for ds in datasets:
            assert ds.get("@type") == DATASET_TYPE, (
                f"Dataset entry missing @type=dcat:Dataset: {ds}"
            )

    def test_catalog_has_title(self, provider_url):
        """Catalog contains a non-empty dct:title."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.get(f"{provider_url}/catalog")
        body = resp.json()
        title = body.get("dct:title", "")
        assert title, f"dct:title is missing or empty in catalog: {body}"

    def test_dataset_has_id(self, provider_url):
        """Each dcat:dataset entry has a non-empty @id IRI."""
        with httpx.Client(timeout=10.0) as client:
            resp = client.get(f"{provider_url}/catalog")
        body = resp.json()
        datasets = body.get("dcat:dataset", [])
        if isinstance(datasets, dict):
            datasets = [datasets]
        for ds in datasets:
            assert ds.get("@id"), f"Dataset entry has no @id: {ds}"

    def test_completes_within_timeout(self, provider_url):
        """GET /catalog round-trip completes within 5 seconds."""
        start = time.monotonic()
        with httpx.Client(timeout=10.0) as client:
            resp = client.get(f"{provider_url}/catalog")
        elapsed = time.monotonic() - start
        assert resp.status_code == 200
        assert elapsed < 5.0, (
            f"GET /catalog took {elapsed:.2f} s — exceeds 5 s budget"
        )


# =========================================================================
# Perspective 2 – catalog fetch via DSP counterpart consumer mock
# =========================================================================

class TestCatalogViaCounterpart:
    """Drive the DSP consumer mock to fetch the catalog (full consumer path)."""

    def test_counterpart_health(self, counterpart_url):
        """DSP counterpart /health returns status=UP."""
        with httpx.Client(timeout=5.0) as client:
            resp = client.get(f"{counterpart_url}/health")
        assert resp.status_code == 200
        body = resp.json()
        assert body.get("status") == "UP", f"Unexpected health body: {body}"

    def test_catalog_fetch_via_counterpart(self, counterpart_url, provider_url):
        """Counterpart POST /api/test/catalog proxies GET /catalog correctly."""
        with httpx.Client(timeout=15.0) as client:
            resp = client.post(
                f"{counterpart_url}/api/test/catalog",
                json={"provider_url": provider_url},
            )
        assert resp.status_code == 200, (
            f"Counterpart returned {resp.status_code}: {resp.text[:300]}"
        )
        body = resp.json()
        assert body.get("@type") == CATALOG_TYPE, (
            f"Counterpart catalog fetch: expected @type={CATALOG_TYPE!r}, "
            f"got {body.get('@type')!r}"
        )

    def test_catalog_context_via_counterpart(self, counterpart_url, provider_url):
        """Counterpart-fetched catalog contains the correct JSON-LD @context."""
        with httpx.Client(timeout=15.0) as client:
            resp = client.post(
                f"{counterpart_url}/api/test/catalog",
                json={"provider_url": provider_url},
            )
        body = resp.json()
        context = body.get("@context", [])
        if isinstance(context, str):
            context = [context]
        assert any(JSONLD_CONTEXT_URL in str(c) for c in context), (
            f"Counterpart catalog @context missing {JSONLD_CONTEXT_URL!r}"
        )

    def test_counterpart_end_to_end_within_timeout(self, counterpart_url, provider_url):
        """Full counterpart → provider → counterpart round-trip < 10 seconds."""
        start = time.monotonic()
        with httpx.Client(timeout=15.0) as client:
            resp = client.post(
                f"{counterpart_url}/api/test/catalog",
                json={"provider_url": provider_url},
            )
        elapsed = time.monotonic() - start
        assert resp.status_code == 200
        assert elapsed < 10.0, (
            f"Counterpart catalog round-trip took {elapsed:.2f} s — "
            f"exceeds 10 s budget"
        )

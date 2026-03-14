"""
Pytest fixtures for DSP Embedded integration tests (Milestone 7).

Provider URL resolution order (highest priority first):
  1. --provider-url CLI option
  2. PROVIDER_DSP_URL environment variable
  3. Default: http://localhost:80

Counterpart URL resolution order:
  1. --counterpart-url CLI option
  2. COUNTERPART_DSP_URL environment variable
  3. Default: http://localhost:18000

The counterpart (docker/dsp-counterpart) must be running before tests start.
Start it with:  cd docker && docker compose up -d
"""

import os
import time
import uuid

import httpx
import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--provider-url",
        default=None,
        help="Base URL of the DSP Embedded node under test "
             "(e.g. http://192.168.1.42). Overrides PROVIDER_DSP_URL env var.",
    )
    parser.addoption(
        "--counterpart-url",
        default=None,
        help="Base URL of the DSP counterpart mock "
             "(default: http://localhost:18000). Overrides COUNTERPART_DSP_URL.",
    )


@pytest.fixture(scope="session")
def provider_url(request) -> str:
    url = (
        request.config.getoption("--provider-url")
        or os.environ.get("PROVIDER_DSP_URL")
        or "http://localhost:80"
    )
    return url.rstrip("/")


@pytest.fixture(scope="session")
def counterpart_url(request) -> str:
    url = (
        request.config.getoption("--counterpart-url")
        or os.environ.get("COUNTERPART_DSP_URL")
        or "http://localhost:18000"
    )
    return url.rstrip("/")


@pytest.fixture(scope="session", autouse=True)
def require_counterpart(counterpart_url):
    """Assert the DSP counterpart is reachable before running any test."""
    deadline = time.monotonic() + 10.0
    last_exc = None
    while time.monotonic() < deadline:
        try:
            with httpx.Client(timeout=2.0) as client:
                resp = client.get(f"{counterpart_url}/health")
                if resp.status_code == 200:
                    return
        except Exception as exc:
            last_exc = exc
        time.sleep(0.5)
    pytest.fail(
        f"DSP counterpart not reachable at {counterpart_url}/health after 10 s. "
        f"Start it with: cd docker && docker compose up -d  "
        f"(last error: {last_exc})"
    )


@pytest.fixture
def unique_pid() -> str:
    """A fresh UUID string (without urn:uuid: prefix) for each test."""
    return str(uuid.uuid4())


@pytest.fixture(scope="session", autouse=True)
def require_provider(provider_url):
    """Assert the DSP Embedded provider is reachable before running any test."""
    deadline = time.monotonic() + 10.0
    last_exc = None
    while time.monotonic() < deadline:
        try:
            with httpx.Client(timeout=2.0) as client:
                resp = client.get(f"{provider_url}/catalog")
                if resp.status_code == 200:
                    return
        except Exception as exc:
            last_exc = exc
        time.sleep(0.5)
    pytest.fail(
        f"DSP Embedded provider not reachable at {provider_url}/catalog after 10 s. "
        f"Ensure the node is running and WiFi-connected.  "
        f"(last error: {last_exc})"
    )

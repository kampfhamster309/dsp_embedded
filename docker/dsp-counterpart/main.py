"""
DSP Counterpart – minimal DSP consumer mock for integration testing.

Implements the counterpart (consumer) side of the Dataspace Protocol
endpoints used by the DSP Embedded integration test suite (Milestone 7).

DSP spec: https://docs.internationaldataspaces.org/ids-knowledgebase/v/dataspace-protocol
"""

import uuid
import logging
from contextlib import asynccontextmanager
from datetime import datetime, timezone
from typing import Any

from fastapi import FastAPI, Request, HTTPException, status
from fastapi.responses import JSONResponse

logging.basicConfig(level=logging.INFO, format="%(levelname)s %(name)s: %(message)s")
log = logging.getLogger("dsp-counterpart")

# ---------------------------------------------------------------------------
# In-memory state
# ---------------------------------------------------------------------------
negotiations: dict[str, dict] = {}
transfers: dict[str, dict] = {}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def dsp_context() -> list[str]:
    return [
        "https://w3id.org/dspace/2024/1/context.json",
        "https://w3id.org/tractusx/policy/v1.0.0",
    ]


def make_offer_body(asset_id: str, policy_id: str, provider_id: str) -> dict:
    """Build a DSP-compliant ContractOffer message to send to the provider."""
    return {
        "@context": dsp_context(),
        "@type": "dspace:ContractOfferMessage",
        "dspace:providerPid": f"urn:uuid:{uuid.uuid4()}",
        "dspace:offer": {
            "@type": "odrl:Offer",
            "@id": f"urn:uuid:{policy_id}",
            "odrl:assigner": {"@id": provider_id},
            "odrl:target": {"@id": asset_id},
            "odrl:permission": [
                {
                    "odrl:action": {"odrl:type": "odrl:use"},
                    "odrl:constraint": [],
                }
            ],
        },
        "dspace:callbackAddress": "http://dsp-counterpart:8000/callback",
    }


# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------

@asynccontextmanager
async def lifespan(app: FastAPI):
    log.info("DSP Counterpart started – listening on :8000")
    log.info("Management API: GET /health, POST /api/test/catalog, POST /api/test/negotiate")
    yield
    log.info("DSP Counterpart shutting down")


app = FastAPI(
    title="DSP Counterpart",
    description="Minimal DSP consumer mock for DSP Embedded integration testing",
    version="0.1.0",
    lifespan=lifespan,
)


# ---------------------------------------------------------------------------
# Health
# ---------------------------------------------------------------------------

@app.get("/health")
async def health():
    return {
        "status": "UP",
        "timestamp": now_iso(),
        "negotiations": len(negotiations),
        "transfers": len(transfers),
    }


# ---------------------------------------------------------------------------
# DSP Callback endpoints
# These receive callbacks from the provider (DSP Embedded node) during
# negotiation and transfer flows.
# ---------------------------------------------------------------------------

@app.post("/callback/negotiations/{consumer_pid}/offers")
async def callback_negotiation_offer(consumer_pid: str, request: Request):
    body = await request.json()
    log.info("CALLBACK negotiation offer received for %s", consumer_pid)
    if consumer_pid in negotiations:
        negotiations[consumer_pid]["state"] = "OFFERED"
        negotiations[consumer_pid]["provider_pid"] = body.get("dspace:providerPid")
    return JSONResponse(status_code=status.HTTP_200_OK, content={"status": "received"})


@app.post("/callback/negotiations/{consumer_pid}/agreement")
async def callback_negotiation_agreement(consumer_pid: str, request: Request):
    body = await request.json()
    log.info("CALLBACK agreement received for %s", consumer_pid)
    if consumer_pid in negotiations:
        negotiations[consumer_pid]["state"] = "AGREED"
        negotiations[consumer_pid]["agreement"] = body
    return JSONResponse(status_code=status.HTTP_200_OK, content={"status": "received"})


@app.post("/callback/negotiations/{consumer_pid}/termination")
async def callback_negotiation_termination(consumer_pid: str, request: Request):
    log.info("CALLBACK termination received for %s", consumer_pid)
    if consumer_pid in negotiations:
        negotiations[consumer_pid]["state"] = "TERMINATED"
    return JSONResponse(status_code=status.HTTP_200_OK, content={"status": "received"})


@app.post("/callback/transfers/{consumer_pid}/start")
async def callback_transfer_start(consumer_pid: str, request: Request):
    body = await request.json()
    log.info("CALLBACK transfer start received for %s", consumer_pid)
    if consumer_pid in transfers:
        transfers[consumer_pid]["state"] = "STARTED"
        transfers[consumer_pid]["provider_pid"] = body.get("dspace:providerPid")
    return JSONResponse(status_code=status.HTTP_200_OK, content={"status": "received"})


@app.post("/callback/transfers/{consumer_pid}/completion")
async def callback_transfer_completion(consumer_pid: str, request: Request):
    log.info("CALLBACK transfer complete for %s", consumer_pid)
    if consumer_pid in transfers:
        transfers[consumer_pid]["state"] = "COMPLETED"
    return JSONResponse(status_code=status.HTTP_200_OK, content={"status": "received"})


# ---------------------------------------------------------------------------
# Test control API
# These endpoints are used by the integration test scripts to drive the
# consumer (this mock) against the provider (DSP Embedded node).
# ---------------------------------------------------------------------------

import httpx

PROVIDER_DSP_URL = "http://dsp-embedded:80"


@app.post("/api/test/catalog")
async def test_fetch_catalog(request: Request):
    """
    Fetch the catalog from the DSP Embedded provider.
    Body: {"provider_url": "http://..."}  (optional override)
    """
    body: dict[str, Any] = {}
    try:
        body = await request.json()
    except Exception:
        pass

    provider_url = body.get("provider_url", PROVIDER_DSP_URL)
    target = f"{provider_url}/catalog"

    log.info("Fetching catalog from %s", target)
    async with httpx.AsyncClient(timeout=10.0) as client:
        try:
            resp = await client.get(target)
            resp.raise_for_status()
            log.info("Catalog fetched: %d bytes", len(resp.content))
            return resp.json()
        except httpx.HTTPStatusError as exc:
            raise HTTPException(status_code=exc.response.status_code, detail=str(exc))
        except Exception as exc:
            raise HTTPException(status_code=502, detail=f"Provider unreachable: {exc}")


@app.post("/api/test/negotiate")
async def test_start_negotiation(request: Request):
    """
    Initiate a contract negotiation with the DSP Embedded provider.
    Body: {
        "provider_url": "http://...",   (optional)
        "asset_id": "sensor-data-01",
        "policy_id": "...",
        "provider_id": "urn:connector:dsp-embedded"
    }
    """
    body: dict[str, Any] = {}
    try:
        body = await request.json()
    except Exception:
        pass

    provider_url = body.get("provider_url", PROVIDER_DSP_URL)
    asset_id = body.get("asset_id", "sensor-data-01")
    policy_id = body.get("policy_id", str(uuid.uuid4()))
    provider_id = body.get("provider_id", "urn:connector:dsp-embedded")

    consumer_pid = str(uuid.uuid4())
    offer_body = make_offer_body(asset_id, policy_id, provider_id)
    offer_body["dspace:consumerPid"] = f"urn:uuid:{consumer_pid}"

    negotiations[consumer_pid] = {
        "consumer_pid": consumer_pid,
        "state": "REQUESTING",
        "started_at": now_iso(),
    }

    target = f"{provider_url}/negotiations/offers"
    log.info("Sending contract offer to %s (consumer_pid=%s)", target, consumer_pid)

    async with httpx.AsyncClient(timeout=10.0) as client:
        try:
            resp = await client.post(target, json=offer_body)
            resp.raise_for_status()
            negotiations[consumer_pid]["state"] = "OFFERED"
            result = resp.json()
            negotiations[consumer_pid]["provider_pid"] = result.get("dspace:providerPid")
            log.info("Offer accepted, provider_pid=%s", negotiations[consumer_pid].get("provider_pid"))
            return {
                "consumer_pid": consumer_pid,
                "negotiation": negotiations[consumer_pid],
                "provider_response": result,
            }
        except httpx.HTTPStatusError as exc:
            negotiations[consumer_pid]["state"] = "FAILED"
            raise HTTPException(status_code=exc.response.status_code, detail=str(exc))
        except Exception as exc:
            negotiations[consumer_pid]["state"] = "FAILED"
            raise HTTPException(status_code=502, detail=f"Provider unreachable: {exc}")


@app.post("/api/test/transfer")
async def test_start_transfer(request: Request):
    """
    Start a data transfer with the DSP Embedded provider.
    Body: {
        "provider_url": "http://...",   (optional)
        "agreement_id": "...",
        "asset_id": "sensor-data-01"
    }
    """
    body: dict[str, Any] = {}
    try:
        body = await request.json()
    except Exception:
        pass

    provider_url = body.get("provider_url", PROVIDER_DSP_URL)
    agreement_id = body.get("agreement_id", str(uuid.uuid4()))
    asset_id = body.get("asset_id", "sensor-data-01")
    consumer_pid = str(uuid.uuid4())

    transfer_body = {
        "@context": dsp_context(),
        "@type": "dspace:TransferRequestMessage",
        "dspace:consumerPid": f"urn:uuid:{consumer_pid}",
        "dspace:agreementId": agreement_id,
        "dct:format": "HttpData-PULL",
        "dspace:dataAddress": {
            "@type": "dspace:DataAddress",
            "dspace:endpointType": "https://w3id.org/idsa/v4.1/HTTP",
        },
        "dspace:callbackAddress": "http://dsp-counterpart:8000/callback",
    }

    transfers[consumer_pid] = {
        "consumer_pid": consumer_pid,
        "asset_id": asset_id,
        "state": "REQUESTING",
        "started_at": now_iso(),
    }

    target = f"{provider_url}/transfers/start"
    log.info("Sending transfer request to %s (consumer_pid=%s)", target, consumer_pid)

    async with httpx.AsyncClient(timeout=10.0) as client:
        try:
            resp = await client.post(target, json=transfer_body)
            resp.raise_for_status()
            transfers[consumer_pid]["state"] = "STARTED"
            result = resp.json()
            log.info("Transfer started: %s", result)
            return {
                "consumer_pid": consumer_pid,
                "transfer": transfers[consumer_pid],
                "provider_response": result,
            }
        except httpx.HTTPStatusError as exc:
            transfers[consumer_pid]["state"] = "FAILED"
            raise HTTPException(status_code=exc.response.status_code, detail=str(exc))
        except Exception as exc:
            transfers[consumer_pid]["state"] = "FAILED"
            raise HTTPException(status_code=502, detail=f"Provider unreachable: {exc}")


@app.get("/api/test/negotiations")
async def get_negotiations():
    return negotiations


@app.get("/api/test/transfers")
async def get_transfers():
    return transfers

/**
 * @file dsp_jsonld_ctx.h
 * @brief Compile-time DSP JSON-LD namespace prefixes and message type mappings.
 *
 * All JSON-LD contexts, namespace prefixes, message type strings, common field
 * names, and negotiation/transfer state strings required by the Dataspace
 * Protocol specification (v2024/1) are defined here as C string literals.
 *
 * Design rationale (see also DEV-001 in doc/deviation_log.md):
 *   A full JSON-LD runtime (jsonld-java, pyLD, …) would consume several
 *   hundred KB of RAM and require network access to dereference remote
 *   contexts — both incompatible with the ESP32-S3 memory budget.  Instead,
 *   the node pre-compiles every context URL and type mapping it will ever
 *   emit or receive into this single header.  Counter-parties must use the
 *   standard DSP namespaces; custom JSON-LD extensions are not supported
 *   (see DEV-001).
 *
 * Usage:
 *   #include "dsp_jsonld_ctx.h"
 *
 *   // Set mandatory fields on an outgoing message object:
 *   dsp_json_add_string(obj, DSP_JSONLD_FIELD_CONTEXT, DSP_JSONLD_CONTEXT_URL);
 *   dsp_json_add_string(obj, DSP_JSONLD_FIELD_TYPE,    DSP_JSONLD_TYPE_CATALOG_REQUEST);
 *
 * This header is always compiled and has no platform-specific code.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * JSON-LD context URL
 *
 * The single @context value expected/emitted in all DSP messages.
 * ------------------------------------------------------------------------- */

/** Primary DSP @context URL (Dataspace Protocol v2024/1). */
#define DSP_JSONLD_CONTEXT_URL  "https://w3id.org/dspace/2024/1/context.json"

/* -------------------------------------------------------------------------
 * Namespace prefix strings
 *
 * Short-form prefix strings (including the trailing colon) matching the
 * expansions defined in DSP_JSONLD_CONTEXT_URL.
 * ------------------------------------------------------------------------- */

/** Dataspace Protocol core namespace prefix. */
#define DSP_JSONLD_PREFIX_DSPACE  "dspace:"

/** DCAT (Data Catalog Vocabulary) namespace prefix. */
#define DSP_JSONLD_PREFIX_DCAT    "dcat:"

/** ODRL (Open Digital Rights Language) namespace prefix. */
#define DSP_JSONLD_PREFIX_ODRL    "odrl:"

/** Dublin Core Terms namespace prefix. */
#define DSP_JSONLD_PREFIX_DCT     "dct:"

/* -------------------------------------------------------------------------
 * Mandatory JSON-LD field names
 * ------------------------------------------------------------------------- */

/** JSON-LD @context field. */
#define DSP_JSONLD_FIELD_CONTEXT    "@context"

/** JSON-LD @type field. */
#define DSP_JSONLD_FIELD_TYPE       "@type"

/** JSON-LD @id field. */
#define DSP_JSONLD_FIELD_ID         "@id"

/* -------------------------------------------------------------------------
 * DSP common field names
 * ------------------------------------------------------------------------- */

/** Process identifier field (negotiation or transfer). */
#define DSP_JSONLD_FIELD_PROCESS_ID    "dspace:processId"

/** Consumer participant identifier. */
#define DSP_JSONLD_FIELD_CONSUMER_ID   "dspace:consumerId"

/** Provider participant identifier. */
#define DSP_JSONLD_FIELD_PROVIDER_ID   "dspace:providerId"

/** Current state of a negotiation or transfer process. */
#define DSP_JSONLD_FIELD_STATE         "dspace:state"

/** Error code field used in DSP error responses. */
#define DSP_JSONLD_FIELD_CODE          "dspace:code"

/** Human-readable reason field used in error/termination messages. */
#define DSP_JSONLD_FIELD_REASON        "dspace:reason"

/** Contract offer embedded in a negotiation message. */
#define DSP_JSONLD_FIELD_OFFER         "dspace:offer"

/** Contract agreement embedded in a negotiation message. */
#define DSP_JSONLD_FIELD_AGREEMENT     "dspace:agreement"

/** Dataset identifier field used in transfer requests. */
#define DSP_JSONLD_FIELD_DATASET       "dspace:dataset"

/** Data address field used in transfer start messages. */
#define DSP_JSONLD_FIELD_DATA_ADDRESS  "dspace:dataAddress"

/* -------------------------------------------------------------------------
 * Catalog message types
 * ------------------------------------------------------------------------- */

/** @type for an incoming catalog request from the consumer. */
#define DSP_JSONLD_TYPE_CATALOG_REQUEST  "dspace:CatalogRequestMessage"

/** @type for the catalog response returned by the provider. */
#define DSP_JSONLD_TYPE_CATALOG          "dcat:Catalog"

/* -------------------------------------------------------------------------
 * Contract negotiation message types
 * ------------------------------------------------------------------------- */

/** @type for a consumer-originated contract request. */
#define DSP_JSONLD_TYPE_CONTRACT_REQUEST  "dspace:ContractRequestMessage"

/** @type for a provider-originated contract offer. */
#define DSP_JSONLD_TYPE_CONTRACT_OFFER    "dspace:ContractOfferMessage"

/** @type for a provider-originated contract agreement. */
#define DSP_JSONLD_TYPE_CONTRACT_AGREEMENT  "dspace:ContractAgreementMessage"

/** @type for a consumer-originated agreement verification. */
#define DSP_JSONLD_TYPE_AGREEMENT_VERIFICATION \
    "dspace:ContractAgreementVerificationMessage"

/** @type for a negotiation event message (state change notification). */
#define DSP_JSONLD_TYPE_NEGOTIATION_EVENT  "dspace:ContractNegotiationEventMessage"

/** @type for a negotiation termination message. */
#define DSP_JSONLD_TYPE_NEGOTIATION_TERMINATION \
    "dspace:ContractNegotiationTerminationMessage"

/** @type for a negotiation process resource. */
#define DSP_JSONLD_TYPE_NEGOTIATION  "dspace:ContractNegotiation"

/* -------------------------------------------------------------------------
 * Transfer process message types
 * ------------------------------------------------------------------------- */

/** @type for a consumer-originated transfer request. */
#define DSP_JSONLD_TYPE_TRANSFER_REQUEST  "dspace:TransferRequestMessage"

/** @type for a provider-originated transfer start message. */
#define DSP_JSONLD_TYPE_TRANSFER_START    "dspace:TransferStartMessage"

/** @type for a transfer completion message. */
#define DSP_JSONLD_TYPE_TRANSFER_COMPLETION  "dspace:TransferCompletionMessage"

/** @type for a transfer termination message. */
#define DSP_JSONLD_TYPE_TRANSFER_TERMINATION  "dspace:TransferTerminationMessage"

/** @type for a transfer suspension message. */
#define DSP_JSONLD_TYPE_TRANSFER_SUSPENSION  "dspace:TransferSuspensionMessage"

/** @type for a transfer process resource. */
#define DSP_JSONLD_TYPE_TRANSFER_PROCESS  "dspace:TransferProcess"

/* -------------------------------------------------------------------------
 * Error type
 * ------------------------------------------------------------------------- */

/** @type for a DSP error response body. */
#define DSP_JSONLD_TYPE_ERROR  "dspace:Error"

/* -------------------------------------------------------------------------
 * Negotiation state values
 *
 * These are the string values of the dspace:state field in negotiation
 * process resources and event messages.
 * ------------------------------------------------------------------------- */

#define DSP_JSONLD_NEG_STATE_REQUESTED   "dspace:REQUESTED"
#define DSP_JSONLD_NEG_STATE_OFFERED     "dspace:OFFERED"
#define DSP_JSONLD_NEG_STATE_AGREED      "dspace:AGREED"
#define DSP_JSONLD_NEG_STATE_VERIFIED    "dspace:VERIFIED"
#define DSP_JSONLD_NEG_STATE_FINALIZED   "dspace:FINALIZED"
#define DSP_JSONLD_NEG_STATE_TERMINATED  "dspace:TERMINATED"

/* -------------------------------------------------------------------------
 * Transfer process state values
 *
 * These are the string values of the dspace:state field in transfer
 * process resources and event messages.
 * ------------------------------------------------------------------------- */

#define DSP_JSONLD_XFER_STATE_REQUESTED   "dspace:REQUESTED"
#define DSP_JSONLD_XFER_STATE_STARTED     "dspace:STARTED"
#define DSP_JSONLD_XFER_STATE_COMPLETED   "dspace:COMPLETED"
#define DSP_JSONLD_XFER_STATE_TERMINATED  "dspace:TERMINATED"
#define DSP_JSONLD_XFER_STATE_SUSPENDED   "dspace:SUSPENDED"

#ifdef __cplusplus
}
#endif

/**
 * @file dsp_build.h
 * @brief Builder functions for outgoing DSP provider messages.
 *
 * Each builder constructs a JSON object conforming to the Dataspace Protocol
 * specification (v2024/1) and serialises it into a caller-supplied buffer.
 * No dynamic memory is retained after the call; cJSON is used internally and
 * freed before returning.
 *
 * Provider-side outgoing messages covered (DEV-003):
 *   - dcat:Catalog                          (catalog response)
 *   - dspace:ContractAgreementMessage        (negotiation – agreement)
 *   - dspace:ContractNegotiationEventMessage (negotiation – state notification)
 *   - dspace:TransferStartMessage            (transfer – start)
 *   - dspace:TransferCompletionMessage       (transfer – completion)
 *   - dspace:Error                           (error response)
 *
 * All functions are always compiled and have no platform-specific code.
 *
 * Dependencies: dsp_json (cJSON wrapper), dsp_jsonld (field/type constants).
 */

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Error codes
 * ------------------------------------------------------------------------- */

typedef enum {
    DSP_BUILD_OK             =  0, /**< Message built and serialised OK.       */
    DSP_BUILD_ERR_NULL_ARG   = -1, /**< A required argument is NULL.           */
    DSP_BUILD_ERR_BUF_TOO_SMALL = -2, /**< Output buffer too small.           */
    DSP_BUILD_ERR_ALLOC      = -3, /**< Internal cJSON allocation failure.     */
} dsp_build_err_t;

/* -------------------------------------------------------------------------
 * Catalog
 * ------------------------------------------------------------------------- */

/**
 * Build a dcat:Catalog response.
 *
 * Produces:
 * @code
 * {
 *   "@context": "...",
 *   "@type":    "dcat:Catalog",
 *   "dct:title": <title>,
 *   "dcat:dataset": [{ "@type": "dcat:Dataset", "@id": <dataset_id> }]
 * }
 * @endcode
 *
 * @param out         Output buffer.
 * @param cap         Buffer capacity in bytes.
 * @param dataset_id  Non-NULL, non-empty IRI string identifying the dataset.
 * @param title       Non-NULL catalog title string.
 * @return            DSP_BUILD_OK or a negative error code.
 */
dsp_build_err_t dsp_build_catalog(char *out, size_t cap,
                                   const char *dataset_id,
                                   const char *title);

/* -------------------------------------------------------------------------
 * Contract negotiation
 * ------------------------------------------------------------------------- */

/**
 * Build a dspace:ContractAgreementMessage.
 *
 * Produces:
 * @code
 * {
 *   "@context":          "...",
 *   "@type":             "dspace:ContractAgreementMessage",
 *   "dspace:processId":  <process_id>,
 *   "dspace:agreement":  { "@type": "odrl:Agreement", "@id": <agreement_id> }
 * }
 * @endcode
 *
 * @param out           Output buffer.
 * @param cap           Buffer capacity in bytes.
 * @param process_id    Non-NULL negotiation process identifier string.
 * @param agreement_id  Non-NULL ODRL agreement IRI string.
 * @return              DSP_BUILD_OK or a negative error code.
 */
dsp_build_err_t dsp_build_agreement_msg(char *out, size_t cap,
                                         const char *process_id,
                                         const char *agreement_id);

/**
 * Build a dspace:ContractNegotiationEventMessage (state change notification).
 *
 * Produces:
 * @code
 * {
 *   "@context":           "...",
 *   "@type":              "dspace:ContractNegotiationEventMessage",
 *   "dspace:processId":   <process_id>,
 *   "dspace:eventType":   <state>
 * }
 * @endcode
 *
 * @p state should be one of the DSP_JSONLD_NEG_STATE_* constants defined in
 * dsp_jsonld_ctx.h (e.g. "dspace:FINALIZED").
 *
 * @param out         Output buffer.
 * @param cap         Buffer capacity in bytes.
 * @param process_id  Non-NULL negotiation process identifier string.
 * @param state       Non-NULL DSP state string.
 * @return            DSP_BUILD_OK or a negative error code.
 */
dsp_build_err_t dsp_build_negotiation_event(char *out, size_t cap,
                                             const char *process_id,
                                             const char *state);

/* -------------------------------------------------------------------------
 * Transfer
 * ------------------------------------------------------------------------- */

/**
 * Build a dspace:TransferStartMessage.
 *
 * Produces:
 * @code
 * {
 *   "@context":           "...",
 *   "@type":              "dspace:TransferStartMessage",
 *   "dspace:processId":   <process_id>,
 *   "dspace:dataAddress": { "dspace:endpointUrl": <endpoint_url> }
 * }
 * @endcode
 *
 * @param out           Output buffer.
 * @param cap           Buffer capacity in bytes.
 * @param process_id    Non-NULL transfer process identifier string.
 * @param endpoint_url  Non-NULL data endpoint URL string.
 * @return              DSP_BUILD_OK or a negative error code.
 */
dsp_build_err_t dsp_build_transfer_start(char *out, size_t cap,
                                          const char *process_id,
                                          const char *endpoint_url);

/**
 * Build a dspace:TransferCompletionMessage.
 *
 * Produces:
 * @code
 * {
 *   "@context":          "...",
 *   "@type":             "dspace:TransferCompletionMessage",
 *   "dspace:processId":  <process_id>
 * }
 * @endcode
 *
 * @param out         Output buffer.
 * @param cap         Buffer capacity in bytes.
 * @param process_id  Non-NULL transfer process identifier string.
 * @return            DSP_BUILD_OK or a negative error code.
 */
dsp_build_err_t dsp_build_transfer_completion(char *out, size_t cap,
                                               const char *process_id);

/* -------------------------------------------------------------------------
 * Error
 * ------------------------------------------------------------------------- */

/**
 * Build a dspace:Error response.
 *
 * Produces:
 * @code
 * {
 *   "@context":      "...",
 *   "@type":         "dspace:Error",
 *   "dspace:code":   <code>,
 *   "dspace:reason": <reason>
 * }
 * @endcode
 *
 * @param out     Output buffer.
 * @param cap     Buffer capacity in bytes.
 * @param code    Non-NULL short error code string (e.g. "400", "FORBIDDEN").
 * @param reason  Non-NULL human-readable reason string.
 * @return        DSP_BUILD_OK or a negative error code.
 */
dsp_build_err_t dsp_build_error(char *out, size_t cap,
                                 const char *code,
                                 const char *reason);

#ifdef __cplusplus
}
#endif

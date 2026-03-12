/**
 * @file dsp_msg.h
 * @brief DSP message schema validators.
 *
 * Validates incoming JSON message bodies against the mandatory field set
 * defined by the Dataspace Protocol specification (v2024/1).  Each validator
 * checks, in order:
 *
 *   1. NULL input guard.
 *   2. JSON parse validity (via cJSON).
 *   3. Presence and non-empty value of @context.
 *   4. Presence and correct string value of @type.
 *   5. Presence of all message-type-specific required fields.
 *
 * The validators do not perform semantic validation (e.g. ODRL policy
 * evaluation or agreement signature checks); they only enforce structural
 * schema conformance sufficient for the provider to decide whether a message
 * can be dispatched to the appropriate state machine.
 *
 * All functions are always compiled and have no platform-specific code;
 * they are suitable for both ESP32-S3 and host-native unit testing.
 *
 * Dependencies: dsp_json (cJSON wrapper), dsp_jsonld (field/type constants).
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Error codes
 * ------------------------------------------------------------------------- */

typedef enum {
    DSP_MSG_OK              =  0, /**< Message is structurally valid.          */
    DSP_MSG_ERR_NULL_INPUT  = -1, /**< Input JSON string pointer is NULL.      */
    DSP_MSG_ERR_PARSE       = -2, /**< JSON text could not be parsed.          */
    DSP_MSG_ERR_MISSING_CONTEXT = -3, /**< @context field absent or empty.    */
    DSP_MSG_ERR_WRONG_TYPE  = -4, /**< @type field absent or wrong value.      */
    DSP_MSG_ERR_MISSING_FIELD = -5, /**< A required message field is absent.  */
} dsp_msg_err_t;

/* -------------------------------------------------------------------------
 * Validators
 * ------------------------------------------------------------------------- */

/**
 * Validate a CatalogRequestMessage.
 *
 * Required fields: @context, @type = dspace:CatalogRequestMessage.
 *
 * @param json  NUL-terminated JSON string (may be NULL).
 * @return      DSP_MSG_OK or a negative error code.
 */
dsp_msg_err_t dsp_msg_validate_catalog_request(const char *json);

/**
 * Validate a ContractOfferMessage.
 *
 * Required fields: @context, @type = dspace:ContractOfferMessage,
 *                  dspace:processId (string), dspace:offer (object).
 *
 * @param json  NUL-terminated JSON string (may be NULL).
 * @return      DSP_MSG_OK or a negative error code.
 */
dsp_msg_err_t dsp_msg_validate_offer(const char *json);

/**
 * Validate a ContractAgreementMessage.
 *
 * Required fields: @context, @type = dspace:ContractAgreementMessage,
 *                  dspace:processId (string), dspace:agreement (object).
 *
 * @param json  NUL-terminated JSON string (may be NULL).
 * @return      DSP_MSG_OK or a negative error code.
 */
dsp_msg_err_t dsp_msg_validate_agreement(const char *json);

/**
 * Validate a TransferRequestMessage.
 *
 * Required fields: @context, @type = dspace:TransferRequestMessage,
 *                  dspace:processId (string), dspace:dataset (string).
 *
 * @param json  NUL-terminated JSON string (may be NULL).
 * @return      DSP_MSG_OK or a negative error code.
 */
dsp_msg_err_t dsp_msg_validate_transfer_request(const char *json);

#ifdef __cplusplus
}
#endif

/**
 * @file test_main.c
 * @brief Unity test runner entry point.
 *
 * Add a RUN_TEST() call here for every test function in every test_*.c file.
 * Test function declarations must be extern so the linker resolves them.
 */

#include "unity.h"

/* -------------------------------------------------------------------------
 * test_smoke.c
 * ------------------------------------------------------------------------- */
extern void test_unity_is_working(void);
extern void test_host_build_flag_is_set(void);
extern void test_esp_platform_not_set(void);
extern void test_config_catalog_request_default(void);
extern void test_config_consumer_default(void);
extern void test_config_daps_shim_default(void);
extern void test_config_negotiate_terminate_default(void);
extern void test_config_tls_session_tickets_default(void);
extern void test_config_psram_default(void);
extern void test_config_deep_sleep_default(void);
extern void test_config_max_negotiations_default(void);
extern void test_config_max_transfers_default(void);
extern void test_config_http_port_default(void);
extern void test_config_verbose_log_default(void);
extern void test_config_daps_gateway_url_default(void);

/* -------------------------------------------------------------------------
 * test_dsp_http.c
 * ------------------------------------------------------------------------- */
extern void test_http_max_routes_in_range(void);
extern void test_http_max_routes_default_value(void);
extern void test_http_method_enum_values_distinct(void);
extern void test_http_register_null_uri_returns_invalid_arg(void);
extern void test_http_register_null_handler_returns_invalid_arg(void);
extern void test_http_register_valid_handler_returns_ok(void);
extern void test_http_register_post_handler_returns_ok(void);
extern void test_http_register_all_methods(void);
extern void test_http_is_running_false_before_start(void);
extern void test_http_start_returns_fail_on_host(void);
extern void test_http_is_running_false_after_failed_start(void);
extern void test_http_stop_safe_when_not_running(void);
extern void test_http_is_running_false_after_stop(void);
extern void test_http_esp_platform_absent_in_host_build(void);
extern void test_http_host_build_flag_set(void);

/* -------------------------------------------------------------------------
 * test_dsp_identity.c
 * ------------------------------------------------------------------------- */
extern void test_identity_init_returns_ok(void);
extern void test_identity_is_provisioned_false_before_init(void);
extern void test_identity_is_provisioned_true_after_init(void);
extern void test_identity_is_provisioned_false_after_deinit(void);
extern void test_identity_double_deinit_safe(void);
extern void test_identity_double_init_safe(void);
extern void test_identity_get_cert_null_out_len_returns_null(void);
extern void test_identity_get_cert_returns_nonnull_after_init(void);
extern void test_identity_get_cert_returns_nonzero_len_after_init(void);
extern void test_identity_get_cert_returns_null_before_init(void);
extern void test_identity_get_cert_returns_null_after_deinit(void);
extern void test_identity_get_key_null_out_len_returns_null(void);
extern void test_identity_get_key_returns_nonnull_after_init(void);
extern void test_identity_get_key_returns_nonzero_len_after_init(void);
extern void test_identity_get_key_returns_null_before_init(void);
extern void test_identity_get_key_returns_null_after_deinit(void);
extern void test_identity_stub_cert_len_is_small(void);
extern void test_identity_stub_key_len_is_small(void);
extern void test_identity_stub_cert_starts_with_der_sequence_tag(void);
extern void test_identity_esp_platform_absent_in_host_build(void);

/* -------------------------------------------------------------------------
 * test_dsp_wifi.c
 * ------------------------------------------------------------------------- */
extern void test_wifi_max_retry_default(void);
extern void test_wifi_reconnect_delay_default(void);
extern void test_sm_idle_connect_goes_connecting(void);
extern void test_sm_idle_unhandled_inputs_stay_idle(void);
extern void test_sm_connecting_connected_goes_connected(void);
extern void test_sm_connecting_disconnected_increments_retry(void);
extern void test_sm_connecting_disconnected_at_max_retry_goes_failed(void);
extern void test_sm_connected_disconnected_goes_reconnecting(void);
extern void test_sm_connected_disconnected_at_max_retry_goes_failed(void);
extern void test_sm_connected_unhandled_stays_connected(void);
extern void test_sm_reconnecting_retry_goes_connecting(void);
extern void test_sm_reconnecting_connected_goes_connected(void);
extern void test_sm_reconnecting_extra_disconnect_increments_retry(void);
extern void test_sm_reconnecting_extra_disconnect_at_max_goes_failed(void);
extern void test_sm_failed_stays_failed_on_all_inputs(void);
extern void test_sm_disconnect_from_idle_stays_idle(void);
extern void test_sm_disconnect_from_connecting_goes_idle(void);
extern void test_sm_disconnect_from_connected_goes_idle(void);
extern void test_sm_disconnect_from_reconnecting_goes_idle(void);
extern void test_sm_disconnect_from_failed_goes_idle(void);
extern void test_sm_full_happy_path(void);
extern void test_sm_exhausted_retries_reach_failed(void);
extern void test_wifi_init_returns_ok_on_host(void);
extern void test_wifi_get_state_idle_after_init(void);
extern void test_wifi_connect_returns_fail_on_host(void);
extern void test_wifi_disconnect_safe_in_idle(void);
extern void test_wifi_store_null_ssid_invalid_arg(void);
extern void test_wifi_store_empty_ssid_invalid_arg(void);
extern void test_wifi_store_null_password_invalid_arg(void);
extern void test_wifi_store_valid_args_fails_on_host(void);
extern void test_wifi_set_event_cb_null_clears(void);
extern void test_wifi_deinit_safe(void);

/* -------------------------------------------------------------------------
 * test_dsp_mem.c
 * ------------------------------------------------------------------------- */
extern void test_mem_budget_total_is_130kb(void);
extern void test_mem_budget_tls_is_50kb(void);
extern void test_mem_budget_http_is_20kb(void);
extern void test_mem_budget_json_is_15kb(void);
extern void test_mem_budget_jwt_is_25kb(void);
extern void test_mem_budget_dspsm_is_20kb(void);
extern void test_mem_budget_components_fit_in_total(void);
extern void test_mem_host_sentinel_above_budget(void);
extern void test_mem_host_sentinel_is_512kb(void);
extern void test_mem_free_internal_returns_sentinel_on_host(void);
extern void test_mem_free_internal_is_nonzero(void);
extern void test_mem_free_psram_returns_zero_on_host(void);
extern void test_mem_report_valid_tag_returns_ok(void);
extern void test_mem_report_null_tag_returns_ok(void);
extern void test_mem_report_empty_tag_returns_ok(void);
extern void test_mem_esp_platform_absent_in_host_build(void);

/* -------------------------------------------------------------------------
 * test_dsp_tls_tickets.c  (session tickets ENABLED path)
 * ------------------------------------------------------------------------- */
extern void test_tickets_flag_is_enabled(void);
extern void test_tickets_enabled_init_still_fails_on_host(void);
extern void test_tickets_deinit_initialized_false_is_safe(void);
extern void test_tickets_deinit_initialized_true_clears_flag(void);
extern void test_tickets_deinit_null_is_safe(void);
extern void test_tickets_double_deinit_safe(void);

/* -------------------------------------------------------------------------
 * test_dsp_tls.c
 * ------------------------------------------------------------------------- */
extern void test_tls_ctx_sizeof_is_nonzero(void);
extern void test_tls_ctx_zero_init_initialized_is_false(void);
extern void test_tls_ctx_initialized_field_accessible(void);
extern void test_tls_init_returns_invalid_arg_for_null_ctx(void);
extern void test_tls_init_returns_fail_on_host(void);
extern void test_tls_deinit_safe_on_null(void);
extern void test_tls_deinit_clears_initialized(void);
extern void test_tls_esp_platform_absent_in_host_build(void);
extern void test_tls_host_build_flag_set(void);

/* -------------------------------------------------------------------------
 * test_dsp_jwt.c
 * ------------------------------------------------------------------------- */
extern void test_jwt_split_null_jwt_returns_invalid_arg(void);
extern void test_jwt_split_null_out_returns_invalid_arg(void);
extern void test_jwt_split_no_dots_returns_invalid_format(void);
extern void test_jwt_split_one_dot_returns_invalid_format(void);
extern void test_jwt_split_valid_returns_ok(void);
extern void test_jwt_split_header_ptr_is_start_of_jwt(void);
extern void test_jwt_split_dot_follows_header(void);
extern void test_jwt_split_payload_follows_first_dot(void);
extern void test_jwt_split_sig_follows_second_dot(void);
extern void test_jwt_b64url_decode_null_src_returns_negative(void);
extern void test_jwt_b64url_decode_null_dst_returns_negative(void);
extern void test_jwt_b64url_decode_empty_returns_zero(void);
extern void test_jwt_b64url_decode_hello_world(void);
extern void test_jwt_b64url_decode_es256_header_length(void);
extern void test_jwt_b64url_decode_es256_header_first_byte(void);
extern void test_jwt_b64url_decode_insufficient_cap_returns_negative(void);
extern void test_jwt_parse_alg_null_returns_unknown(void);
extern void test_jwt_parse_alg_es256_returns_es256(void);
extern void test_jwt_parse_alg_rs256_returns_rs256(void);
extern void test_jwt_parse_alg_unknown_returns_unknown(void);
extern void test_jwt_parse_exp_null_json_returns_invalid_arg(void);
extern void test_jwt_parse_exp_null_out_returns_invalid_arg(void);
extern void test_jwt_parse_exp_valid_returns_ok(void);
extern void test_jwt_parse_exp_valid_value_is_far_future(void);
extern void test_jwt_parse_exp_expired_value_is_small(void);
extern void test_jwt_parse_exp_no_exp_returns_no_exp(void);
extern void test_jwt_validate_null_jwt_returns_invalid_arg(void);
extern void test_jwt_validate_null_pubkey_returns_invalid_arg(void);
extern void test_jwt_validate_zero_len_pubkey_returns_invalid_arg(void);
extern void test_jwt_validate_malformed_returns_invalid_format(void);
extern void test_jwt_validate_wrong_alg_returns_invalid_alg(void);
extern void test_jwt_validate_expired_returns_expired(void);
extern void test_jwt_validate_no_exp_returns_no_exp(void);
extern void test_jwt_validate_valid_format_host_returns_crypto(void);
extern void test_jwt_esp_platform_absent_in_host_build(void);
extern void test_jwt_rs256_alg_enum_distinct_from_es256(void);
extern void test_jwt_rs256_validate_null_jwt_returns_invalid_arg(void);
extern void test_jwt_rs256_validate_null_pubkey_returns_invalid_arg(void);
extern void test_jwt_rs256_validate_zero_len_pubkey_returns_invalid_arg(void);
extern void test_jwt_rs256_validate_malformed_returns_invalid_format(void);
extern void test_jwt_rs256_validate_es256_alg_returns_invalid_alg(void);
extern void test_jwt_rs256_validate_expired_returns_expired(void);
extern void test_jwt_rs256_validate_no_exp_returns_no_exp(void);
extern void test_jwt_rs256_validate_valid_format_host_returns_crypto(void);

/* -------------------------------------------------------------------------
 * test_dsp_psk.c
 * ------------------------------------------------------------------------- */
extern void test_psk_init_returns_ok(void);
extern void test_psk_is_configured_false_before_any_call(void);
extern void test_psk_is_configured_false_after_init(void);
extern void test_psk_is_configured_false_after_deinit(void);
extern void test_psk_double_init_safe(void);
extern void test_psk_double_deinit_safe(void);
extern void test_psk_set_null_identity_returns_invalid_arg(void);
extern void test_psk_set_null_key_returns_invalid_arg(void);
extern void test_psk_set_zero_identity_len_returns_invalid_arg(void);
extern void test_psk_set_zero_key_len_returns_invalid_arg(void);
extern void test_psk_set_identity_too_long_returns_invalid_arg(void);
extern void test_psk_set_key_too_long_returns_invalid_arg(void);
extern void test_psk_set_key_too_short_returns_invalid_arg(void);
extern void test_psk_set_valid_returns_ok(void);
extern void test_psk_is_configured_true_after_set(void);
extern void test_psk_get_identity_null_out_len_returns_null(void);
extern void test_psk_get_key_null_out_len_returns_null(void);
extern void test_psk_get_identity_null_before_set(void);
extern void test_psk_get_key_null_before_set(void);
extern void test_psk_get_identity_nonnull_after_set(void);
extern void test_psk_get_key_nonnull_after_set(void);
extern void test_psk_get_identity_len_correct(void);
extern void test_psk_get_key_len_correct(void);
extern void test_psk_get_identity_content_correct(void);
extern void test_psk_get_key_content_correct(void);
extern void test_psk_get_null_after_deinit(void);
extern void test_psk_esp_platform_absent_in_host_build(void);

/* -------------------------------------------------------------------------
 * test_dsp_neg.c
 * ------------------------------------------------------------------------- */
extern void test_neg_sm_requested_offer_goes_offered(void);
extern void test_neg_sm_requested_agree_goes_agreed(void);
extern void test_neg_sm_requested_terminate_goes_terminated(void);
extern void test_neg_sm_requested_unhandled_stays(void);
extern void test_neg_sm_offered_agree_goes_agreed(void);
extern void test_neg_sm_offered_terminate_goes_terminated(void);
extern void test_neg_sm_offered_unhandled_stays(void);
extern void test_neg_sm_agreed_verify_goes_verified(void);
extern void test_neg_sm_agreed_terminate_goes_terminated(void);
extern void test_neg_sm_agreed_unhandled_stays(void);
extern void test_neg_sm_verified_finalize_goes_finalized(void);
extern void test_neg_sm_verified_terminate_goes_terminated(void);
extern void test_neg_sm_verified_unhandled_stays(void);
extern void test_neg_sm_finalized_absorbs_all_events(void);
extern void test_neg_sm_terminated_absorbs_all_events(void);
extern void test_neg_sm_full_happy_path(void);
extern void test_neg_init_returns_ok(void);
extern void test_neg_count_active_zero_after_init(void);
extern void test_neg_create_null_cpid_returns_neg1(void);
extern void test_neg_create_null_ppid_returns_neg1(void);
extern void test_neg_create_returns_zero_first_slot(void);
extern void test_neg_create_state_is_requested(void);
extern void test_neg_apply_offer_event_gives_offered(void);
extern void test_neg_get_consumer_pid_correct(void);
extern void test_neg_get_provider_pid_correct(void);
extern void test_neg_find_by_cpid_returns_correct_index(void);
extern void test_neg_find_by_cpid_returns_neg1_if_not_found(void);
extern void test_neg_find_by_cpid_null_returns_neg1(void);
extern void test_neg_table_overflow_returns_neg1(void);
extern void test_neg_count_active_tracks_creates(void);
extern void test_neg_apply_invalid_index_returns_initial(void);
extern void test_neg_get_state_invalid_index_returns_initial(void);
extern void test_neg_get_pids_invalid_index_returns_null(void);
extern void test_neg_register_handlers_before_init_returns_invalid_state(void);
extern void test_neg_register_handlers_after_init_returns_ok(void);
extern void test_neg_double_init_safe(void);
extern void test_neg_apply_agree_event_from_offered(void);
extern void test_neg_state_is_agreed_after_offer_then_agree(void);
extern void test_neg_find_by_cpid_after_agree_still_works(void);
extern void test_neg_terminate_flag_is_disabled_by_default(void);
extern void test_neg_apply_terminate_from_requested(void);
extern void test_neg_apply_terminate_from_offered(void);
extern void test_neg_apply_terminate_from_agreed(void);
extern void test_neg_terminated_state_is_stable(void);

/* -------------------------------------------------------------------------
 * test_dsp_xfer.c
 * ------------------------------------------------------------------------- */
extern void test_xfer_sm_initial_start_goes_transferring(void);
extern void test_xfer_sm_initial_unhandled_stays(void);
extern void test_xfer_sm_transferring_complete_goes_completed(void);
extern void test_xfer_sm_transferring_fail_goes_failed(void);
extern void test_xfer_sm_transferring_unhandled_stays(void);
extern void test_xfer_sm_completed_absorbs_all(void);
extern void test_xfer_sm_failed_absorbs_all(void);
extern void test_xfer_init_returns_ok(void);
extern void test_xfer_is_initialized_false_before_init(void);
extern void test_xfer_is_initialized_true_after_init(void);
extern void test_xfer_count_active_zero_after_init(void);
extern void test_xfer_create_null_pid_returns_neg1(void);
extern void test_xfer_create_returns_zero_first_slot(void);
extern void test_xfer_create_state_is_initial(void);
extern void test_xfer_apply_start_gives_transferring(void);
extern void test_xfer_get_state_invalid_index_returns_initial(void);
extern void test_xfer_get_process_id_correct(void);
extern void test_xfer_get_process_id_invalid_returns_null(void);
extern void test_xfer_find_by_pid_returns_index(void);
extern void test_xfer_find_by_pid_null_returns_neg1(void);
extern void test_xfer_find_by_pid_not_found_returns_neg1(void);
extern void test_xfer_table_overflow_returns_neg1(void);
extern void test_xfer_count_active_tracks_creates(void);
extern void test_xfer_double_init_safe(void);
extern void test_xfer_notify_null_by_default(void);
extern void test_xfer_notify_called_on_start(void);
extern void test_xfer_notify_not_called_on_complete(void);
extern void test_xfer_notify_not_called_when_already_transferring(void);
extern void test_xfer_register_handlers_before_init_returns_invalid_state(void);
extern void test_xfer_register_handlers_after_init_returns_ok(void);
extern void test_xfer_is_initialized_false_after_deinit(void);

/* -------------------------------------------------------------------------
 * test_dsp_catalog.c
 * ------------------------------------------------------------------------- */
extern void test_catalog_dataset_id_is_nonempty(void);
extern void test_catalog_title_is_nonempty(void);
extern void test_catalog_json_buf_size_sufficient(void);
extern void test_catalog_is_initialized_false_before_init(void);
extern void test_catalog_init_returns_ok(void);
extern void test_catalog_is_initialized_true_after_init(void);
extern void test_catalog_is_initialized_false_after_deinit(void);
extern void test_catalog_double_init_safe(void);
extern void test_catalog_double_deinit_safe(void);
extern void test_catalog_get_json_null_buf_returns_null_arg(void);
extern void test_catalog_get_json_buf_too_small_returns_error(void);
extern void test_catalog_get_json_returns_ok(void);
extern void test_catalog_get_json_context_field(void);
extern void test_catalog_get_json_type_is_catalog(void);
extern void test_catalog_get_json_title_matches_config(void);
extern void test_catalog_get_json_dataset_is_array(void);
extern void test_catalog_register_handler_before_init_returns_invalid_state(void);
extern void test_catalog_register_handler_after_init_returns_ok(void);

/* -------------------------------------------------------------------------
 * test_dsp_build.c
 * ------------------------------------------------------------------------- */
extern void test_build_error_codes_distinct(void);
extern void test_build_catalog_null_out_returns_null_arg(void);
extern void test_build_catalog_null_dataset_id_returns_null_arg(void);
extern void test_build_catalog_null_title_returns_null_arg(void);
extern void test_build_catalog_buf_too_small_returns_error(void);
extern void test_build_catalog_valid_returns_ok(void);
extern void test_build_catalog_output_fields(void);
extern void test_build_agreement_null_out_returns_null_arg(void);
extern void test_build_agreement_null_process_id_returns_null_arg(void);
extern void test_build_agreement_null_agreement_id_returns_null_arg(void);
extern void test_build_agreement_buf_too_small_returns_error(void);
extern void test_build_agreement_valid_returns_ok(void);
extern void test_build_agreement_output_fields(void);
extern void test_build_neg_event_null_out_returns_null_arg(void);
extern void test_build_neg_event_null_process_id_returns_null_arg(void);
extern void test_build_neg_event_null_state_returns_null_arg(void);
extern void test_build_neg_event_buf_too_small_returns_error(void);
extern void test_build_neg_event_valid_returns_ok(void);
extern void test_build_neg_event_output_fields(void);
extern void test_build_xfer_start_null_out_returns_null_arg(void);
extern void test_build_xfer_start_null_process_id_returns_null_arg(void);
extern void test_build_xfer_start_null_endpoint_returns_null_arg(void);
extern void test_build_xfer_start_buf_too_small_returns_error(void);
extern void test_build_xfer_start_valid_returns_ok(void);
extern void test_build_xfer_start_output_fields(void);
extern void test_build_xfer_completion_null_out_returns_null_arg(void);
extern void test_build_xfer_completion_null_process_id_returns_null_arg(void);
extern void test_build_xfer_completion_buf_too_small_returns_error(void);
extern void test_build_xfer_completion_valid_returns_ok(void);
extern void test_build_xfer_completion_output_fields(void);
extern void test_build_error_null_out_returns_null_arg(void);
extern void test_build_error_null_code_returns_null_arg(void);
extern void test_build_error_null_reason_returns_null_arg(void);
extern void test_build_error_buf_too_small_returns_error(void);
extern void test_build_error_valid_returns_ok(void);
extern void test_build_error_output_fields(void);

/* -------------------------------------------------------------------------
 * test_dsp_msg.c
 * ------------------------------------------------------------------------- */
extern void test_msg_error_codes_are_distinct(void);
extern void test_msg_cat_req_null_returns_null_input(void);
extern void test_msg_cat_req_bad_json_returns_parse_error(void);
extern void test_msg_cat_req_missing_context_returns_error(void);
extern void test_msg_cat_req_wrong_type_returns_error(void);
extern void test_msg_cat_req_valid_returns_ok(void);
extern void test_msg_cat_req_with_optional_filter_returns_ok(void);
extern void test_msg_offer_null_returns_null_input(void);
extern void test_msg_offer_bad_json_returns_parse_error(void);
extern void test_msg_offer_missing_context_returns_error(void);
extern void test_msg_offer_wrong_type_returns_error(void);
extern void test_msg_offer_missing_process_id_returns_missing_field(void);
extern void test_msg_offer_missing_offer_obj_returns_missing_field(void);
extern void test_msg_offer_offer_field_is_string_returns_missing_field(void);
extern void test_msg_offer_valid_returns_ok(void);
extern void test_msg_agreement_null_returns_null_input(void);
extern void test_msg_agreement_bad_json_returns_parse_error(void);
extern void test_msg_agreement_missing_context_returns_error(void);
extern void test_msg_agreement_wrong_type_returns_error(void);
extern void test_msg_agreement_missing_process_id_returns_missing_field(void);
extern void test_msg_agreement_missing_agreement_obj_returns_missing_field(void);
extern void test_msg_agreement_agreement_field_is_string_returns_missing_field(void);
extern void test_msg_agreement_valid_returns_ok(void);
extern void test_msg_xfer_null_returns_null_input(void);
extern void test_msg_xfer_bad_json_returns_parse_error(void);
extern void test_msg_xfer_missing_context_returns_error(void);
extern void test_msg_xfer_wrong_type_returns_error(void);
extern void test_msg_xfer_missing_process_id_returns_missing_field(void);
extern void test_msg_xfer_missing_dataset_returns_missing_field(void);
extern void test_msg_xfer_valid_returns_ok(void);
extern void test_msg_catalog_request_rejected_by_offer_validator(void);

/* -------------------------------------------------------------------------
 * test_dsp_jsonld.c
 * ------------------------------------------------------------------------- */
extern void test_jsonld_context_url_is_defined(void);
extern void test_jsonld_context_url_contains_dspace_domain(void);
extern void test_jsonld_prefix_dspace_ends_with_colon(void);
extern void test_jsonld_prefix_dcat_ends_with_colon(void);
extern void test_jsonld_prefix_odrl_ends_with_colon(void);
extern void test_jsonld_field_context_value(void);
extern void test_jsonld_field_type_value(void);
extern void test_jsonld_field_id_value(void);
extern void test_jsonld_common_fields_have_dspace_prefix(void);
extern void test_jsonld_type_catalog_request_has_dspace_prefix(void);
extern void test_jsonld_type_catalog_has_dcat_prefix(void);
extern void test_jsonld_negotiation_types_have_dspace_prefix(void);
extern void test_jsonld_negotiation_types_are_distinct(void);
extern void test_jsonld_transfer_types_have_dspace_prefix(void);
extern void test_jsonld_transfer_types_are_distinct(void);
extern void test_jsonld_type_error_has_dspace_prefix(void);
extern void test_jsonld_neg_states_have_dspace_prefix(void);
extern void test_jsonld_neg_states_are_distinct(void);
extern void test_jsonld_xfer_states_have_dspace_prefix(void);
extern void test_jsonld_xfer_states_are_distinct(void);
extern void test_jsonld_constants_work_with_dsp_json(void);

/* -------------------------------------------------------------------------
 * test_dsp_json.c
 * ------------------------------------------------------------------------- */
extern void test_json_cjson_version_is_1_7(void);
extern void test_json_parse_valid_returns_non_null(void);
extern void test_json_parse_invalid_returns_null(void);
extern void test_json_parse_null_returns_null(void);
extern void test_json_delete_null_is_safe(void);
extern void test_json_get_string_existing_key(void);
extern void test_json_get_string_missing_key_returns_false(void);
extern void test_json_get_string_null_obj_returns_false(void);
extern void test_json_get_string_null_key_returns_false(void);
extern void test_json_get_string_buf_too_small_returns_false(void);
extern void test_json_get_string_exact_fit(void);
extern void test_json_get_type_returns_correct_value(void);
extern void test_json_get_context_returns_correct_value(void);
extern void test_json_has_mandatory_fields_both_present(void);
extern void test_json_has_mandatory_fields_missing_type(void);
extern void test_json_has_mandatory_fields_missing_context(void);
extern void test_json_has_mandatory_fields_null_returns_false(void);
extern void test_json_new_object_returns_non_null(void);
extern void test_json_add_string_roundtrip(void);
extern void test_json_add_string_null_args_return_false(void);
extern void test_json_print_to_buffer(void);
extern void test_json_print_buffer_too_small_returns_false(void);
extern void test_json_print_alloc_and_free(void);
extern void test_json_print_null_returns_false(void);

/* -------------------------------------------------------------------------
 * test_dsp_daps.c
 * ------------------------------------------------------------------------- */
extern void test_daps_init_returns_ok(void);
extern void test_daps_is_initialized_false_before_init(void);
extern void test_daps_is_initialized_true_after_init(void);
extern void test_daps_is_initialized_false_after_deinit(void);
extern void test_daps_double_init_safe(void);
extern void test_daps_double_deinit_safe(void);
extern void test_daps_request_null_buf_returns_invalid_arg(void);
extern void test_daps_request_zero_cap_returns_invalid_arg(void);
extern void test_daps_request_null_out_len_returns_invalid_arg(void);
extern void test_daps_request_before_init_returns_not_init(void);
extern void test_daps_shim_disabled_by_default(void);
extern void test_daps_gateway_url_empty_by_default(void);
extern void test_daps_request_shim_disabled_returns_disabled(void);
extern void test_daps_max_token_len_nonzero(void);
extern void test_daps_max_token_len_fits_typical_dat(void);
extern void test_daps_error_codes_are_distinct(void);
extern void test_daps_esp_platform_absent_in_host_build(void);

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();

    /* smoke */
    RUN_TEST(test_unity_is_working);
    RUN_TEST(test_host_build_flag_is_set);
    RUN_TEST(test_esp_platform_not_set);

    /* dsp_config defaults */
    RUN_TEST(test_config_catalog_request_default);
    RUN_TEST(test_config_consumer_default);
    RUN_TEST(test_config_daps_shim_default);
    RUN_TEST(test_config_negotiate_terminate_default);
    RUN_TEST(test_config_tls_session_tickets_default);
    RUN_TEST(test_config_psram_default);
    RUN_TEST(test_config_deep_sleep_default);
    RUN_TEST(test_config_max_negotiations_default);
    RUN_TEST(test_config_max_transfers_default);
    RUN_TEST(test_config_http_port_default);
    RUN_TEST(test_config_verbose_log_default);
    RUN_TEST(test_config_daps_gateway_url_default);

    /* dsp_http */
    RUN_TEST(test_http_max_routes_in_range);
    RUN_TEST(test_http_max_routes_default_value);
    RUN_TEST(test_http_method_enum_values_distinct);
    RUN_TEST(test_http_register_null_uri_returns_invalid_arg);
    RUN_TEST(test_http_register_null_handler_returns_invalid_arg);
    RUN_TEST(test_http_register_valid_handler_returns_ok);
    RUN_TEST(test_http_register_post_handler_returns_ok);
    RUN_TEST(test_http_register_all_methods);
    RUN_TEST(test_http_is_running_false_before_start);
    RUN_TEST(test_http_start_returns_fail_on_host);
    RUN_TEST(test_http_is_running_false_after_failed_start);
    RUN_TEST(test_http_stop_safe_when_not_running);
    RUN_TEST(test_http_is_running_false_after_stop);
    RUN_TEST(test_http_esp_platform_absent_in_host_build);
    RUN_TEST(test_http_host_build_flag_set);

    /* dsp_identity */
    RUN_TEST(test_identity_init_returns_ok);
    RUN_TEST(test_identity_is_provisioned_false_before_init);
    RUN_TEST(test_identity_is_provisioned_true_after_init);
    RUN_TEST(test_identity_is_provisioned_false_after_deinit);
    RUN_TEST(test_identity_double_deinit_safe);
    RUN_TEST(test_identity_double_init_safe);
    RUN_TEST(test_identity_get_cert_null_out_len_returns_null);
    RUN_TEST(test_identity_get_cert_returns_nonnull_after_init);
    RUN_TEST(test_identity_get_cert_returns_nonzero_len_after_init);
    RUN_TEST(test_identity_get_cert_returns_null_before_init);
    RUN_TEST(test_identity_get_cert_returns_null_after_deinit);
    RUN_TEST(test_identity_get_key_null_out_len_returns_null);
    RUN_TEST(test_identity_get_key_returns_nonnull_after_init);
    RUN_TEST(test_identity_get_key_returns_nonzero_len_after_init);
    RUN_TEST(test_identity_get_key_returns_null_before_init);
    RUN_TEST(test_identity_get_key_returns_null_after_deinit);
    RUN_TEST(test_identity_stub_cert_len_is_small);
    RUN_TEST(test_identity_stub_key_len_is_small);
    RUN_TEST(test_identity_stub_cert_starts_with_der_sequence_tag);
    RUN_TEST(test_identity_esp_platform_absent_in_host_build);

    /* dsp_wifi */
    RUN_TEST(test_wifi_max_retry_default);
    RUN_TEST(test_wifi_reconnect_delay_default);
    RUN_TEST(test_sm_idle_connect_goes_connecting);
    RUN_TEST(test_sm_idle_unhandled_inputs_stay_idle);
    RUN_TEST(test_sm_connecting_connected_goes_connected);
    RUN_TEST(test_sm_connecting_disconnected_increments_retry);
    RUN_TEST(test_sm_connecting_disconnected_at_max_retry_goes_failed);
    RUN_TEST(test_sm_connected_disconnected_goes_reconnecting);
    RUN_TEST(test_sm_connected_disconnected_at_max_retry_goes_failed);
    RUN_TEST(test_sm_connected_unhandled_stays_connected);
    RUN_TEST(test_sm_reconnecting_retry_goes_connecting);
    RUN_TEST(test_sm_reconnecting_connected_goes_connected);
    RUN_TEST(test_sm_reconnecting_extra_disconnect_increments_retry);
    RUN_TEST(test_sm_reconnecting_extra_disconnect_at_max_goes_failed);
    RUN_TEST(test_sm_failed_stays_failed_on_all_inputs);
    RUN_TEST(test_sm_disconnect_from_idle_stays_idle);
    RUN_TEST(test_sm_disconnect_from_connecting_goes_idle);
    RUN_TEST(test_sm_disconnect_from_connected_goes_idle);
    RUN_TEST(test_sm_disconnect_from_reconnecting_goes_idle);
    RUN_TEST(test_sm_disconnect_from_failed_goes_idle);
    RUN_TEST(test_sm_full_happy_path);
    RUN_TEST(test_sm_exhausted_retries_reach_failed);
    RUN_TEST(test_wifi_init_returns_ok_on_host);
    RUN_TEST(test_wifi_get_state_idle_after_init);
    RUN_TEST(test_wifi_connect_returns_fail_on_host);
    RUN_TEST(test_wifi_disconnect_safe_in_idle);
    RUN_TEST(test_wifi_store_null_ssid_invalid_arg);
    RUN_TEST(test_wifi_store_empty_ssid_invalid_arg);
    RUN_TEST(test_wifi_store_null_password_invalid_arg);
    RUN_TEST(test_wifi_store_valid_args_fails_on_host);
    RUN_TEST(test_wifi_set_event_cb_null_clears);
    RUN_TEST(test_wifi_deinit_safe);

    /* dsp_mem */
    RUN_TEST(test_mem_budget_total_is_130kb);
    RUN_TEST(test_mem_budget_tls_is_50kb);
    RUN_TEST(test_mem_budget_http_is_20kb);
    RUN_TEST(test_mem_budget_json_is_15kb);
    RUN_TEST(test_mem_budget_jwt_is_25kb);
    RUN_TEST(test_mem_budget_dspsm_is_20kb);
    RUN_TEST(test_mem_budget_components_fit_in_total);
    RUN_TEST(test_mem_host_sentinel_above_budget);
    RUN_TEST(test_mem_host_sentinel_is_512kb);
    RUN_TEST(test_mem_free_internal_returns_sentinel_on_host);
    RUN_TEST(test_mem_free_internal_is_nonzero);
    RUN_TEST(test_mem_free_psram_returns_zero_on_host);
    RUN_TEST(test_mem_report_valid_tag_returns_ok);
    RUN_TEST(test_mem_report_null_tag_returns_ok);
    RUN_TEST(test_mem_report_empty_tag_returns_ok);
    RUN_TEST(test_mem_esp_platform_absent_in_host_build);

    /* dsp_tls session tickets – enabled path */
    RUN_TEST(test_tickets_flag_is_enabled);
    RUN_TEST(test_tickets_enabled_init_still_fails_on_host);
    RUN_TEST(test_tickets_deinit_initialized_false_is_safe);
    RUN_TEST(test_tickets_deinit_initialized_true_clears_flag);
    RUN_TEST(test_tickets_deinit_null_is_safe);
    RUN_TEST(test_tickets_double_deinit_safe);

    /* dsp_tls */
    RUN_TEST(test_tls_ctx_sizeof_is_nonzero);
    RUN_TEST(test_tls_ctx_zero_init_initialized_is_false);
    RUN_TEST(test_tls_ctx_initialized_field_accessible);
    RUN_TEST(test_tls_init_returns_invalid_arg_for_null_ctx);
    RUN_TEST(test_tls_init_returns_fail_on_host);
    RUN_TEST(test_tls_deinit_safe_on_null);
    RUN_TEST(test_tls_deinit_clears_initialized);
    RUN_TEST(test_tls_esp_platform_absent_in_host_build);
    RUN_TEST(test_tls_host_build_flag_set);

    /* dsp_jwt */
    RUN_TEST(test_jwt_split_null_jwt_returns_invalid_arg);
    RUN_TEST(test_jwt_split_null_out_returns_invalid_arg);
    RUN_TEST(test_jwt_split_no_dots_returns_invalid_format);
    RUN_TEST(test_jwt_split_one_dot_returns_invalid_format);
    RUN_TEST(test_jwt_split_valid_returns_ok);
    RUN_TEST(test_jwt_split_header_ptr_is_start_of_jwt);
    RUN_TEST(test_jwt_split_dot_follows_header);
    RUN_TEST(test_jwt_split_payload_follows_first_dot);
    RUN_TEST(test_jwt_split_sig_follows_second_dot);
    RUN_TEST(test_jwt_b64url_decode_null_src_returns_negative);
    RUN_TEST(test_jwt_b64url_decode_null_dst_returns_negative);
    RUN_TEST(test_jwt_b64url_decode_empty_returns_zero);
    RUN_TEST(test_jwt_b64url_decode_hello_world);
    RUN_TEST(test_jwt_b64url_decode_es256_header_length);
    RUN_TEST(test_jwt_b64url_decode_es256_header_first_byte);
    RUN_TEST(test_jwt_b64url_decode_insufficient_cap_returns_negative);
    RUN_TEST(test_jwt_parse_alg_null_returns_unknown);
    RUN_TEST(test_jwt_parse_alg_es256_returns_es256);
    RUN_TEST(test_jwt_parse_alg_rs256_returns_rs256);
    RUN_TEST(test_jwt_parse_alg_unknown_returns_unknown);
    RUN_TEST(test_jwt_parse_exp_null_json_returns_invalid_arg);
    RUN_TEST(test_jwt_parse_exp_null_out_returns_invalid_arg);
    RUN_TEST(test_jwt_parse_exp_valid_returns_ok);
    RUN_TEST(test_jwt_parse_exp_valid_value_is_far_future);
    RUN_TEST(test_jwt_parse_exp_expired_value_is_small);
    RUN_TEST(test_jwt_parse_exp_no_exp_returns_no_exp);
    RUN_TEST(test_jwt_validate_null_jwt_returns_invalid_arg);
    RUN_TEST(test_jwt_validate_null_pubkey_returns_invalid_arg);
    RUN_TEST(test_jwt_validate_zero_len_pubkey_returns_invalid_arg);
    RUN_TEST(test_jwt_validate_malformed_returns_invalid_format);
    RUN_TEST(test_jwt_validate_wrong_alg_returns_invalid_alg);
    RUN_TEST(test_jwt_validate_expired_returns_expired);
    RUN_TEST(test_jwt_validate_no_exp_returns_no_exp);
    RUN_TEST(test_jwt_validate_valid_format_host_returns_crypto);
    RUN_TEST(test_jwt_esp_platform_absent_in_host_build);

    /* dsp_jwt RS256 (DSP-203) */
    RUN_TEST(test_jwt_rs256_alg_enum_distinct_from_es256);
    RUN_TEST(test_jwt_rs256_validate_null_jwt_returns_invalid_arg);
    RUN_TEST(test_jwt_rs256_validate_null_pubkey_returns_invalid_arg);
    RUN_TEST(test_jwt_rs256_validate_zero_len_pubkey_returns_invalid_arg);
    RUN_TEST(test_jwt_rs256_validate_malformed_returns_invalid_format);
    RUN_TEST(test_jwt_rs256_validate_es256_alg_returns_invalid_alg);
    RUN_TEST(test_jwt_rs256_validate_expired_returns_expired);
    RUN_TEST(test_jwt_rs256_validate_no_exp_returns_no_exp);
    RUN_TEST(test_jwt_rs256_validate_valid_format_host_returns_crypto);

    /* dsp_psk (DSP-204) */
    RUN_TEST(test_psk_init_returns_ok);
    RUN_TEST(test_psk_is_configured_false_before_any_call);
    RUN_TEST(test_psk_is_configured_false_after_init);
    RUN_TEST(test_psk_is_configured_false_after_deinit);
    RUN_TEST(test_psk_double_init_safe);
    RUN_TEST(test_psk_double_deinit_safe);
    RUN_TEST(test_psk_set_null_identity_returns_invalid_arg);
    RUN_TEST(test_psk_set_null_key_returns_invalid_arg);
    RUN_TEST(test_psk_set_zero_identity_len_returns_invalid_arg);
    RUN_TEST(test_psk_set_zero_key_len_returns_invalid_arg);
    RUN_TEST(test_psk_set_identity_too_long_returns_invalid_arg);
    RUN_TEST(test_psk_set_key_too_long_returns_invalid_arg);
    RUN_TEST(test_psk_set_key_too_short_returns_invalid_arg);
    RUN_TEST(test_psk_set_valid_returns_ok);
    RUN_TEST(test_psk_is_configured_true_after_set);
    RUN_TEST(test_psk_get_identity_null_out_len_returns_null);
    RUN_TEST(test_psk_get_key_null_out_len_returns_null);
    RUN_TEST(test_psk_get_identity_null_before_set);
    RUN_TEST(test_psk_get_key_null_before_set);
    RUN_TEST(test_psk_get_identity_nonnull_after_set);
    RUN_TEST(test_psk_get_key_nonnull_after_set);
    RUN_TEST(test_psk_get_identity_len_correct);
    RUN_TEST(test_psk_get_key_len_correct);
    RUN_TEST(test_psk_get_identity_content_correct);
    RUN_TEST(test_psk_get_key_content_correct);
    RUN_TEST(test_psk_get_null_after_deinit);
    RUN_TEST(test_psk_esp_platform_absent_in_host_build);

    /* dsp_neg SM + slot table (DSP-402) */
    RUN_TEST(test_neg_sm_requested_offer_goes_offered);
    RUN_TEST(test_neg_sm_requested_agree_goes_agreed);
    RUN_TEST(test_neg_sm_requested_terminate_goes_terminated);
    RUN_TEST(test_neg_sm_requested_unhandled_stays);
    RUN_TEST(test_neg_sm_offered_agree_goes_agreed);
    RUN_TEST(test_neg_sm_offered_terminate_goes_terminated);
    RUN_TEST(test_neg_sm_offered_unhandled_stays);
    RUN_TEST(test_neg_sm_agreed_verify_goes_verified);
    RUN_TEST(test_neg_sm_agreed_terminate_goes_terminated);
    RUN_TEST(test_neg_sm_agreed_unhandled_stays);
    RUN_TEST(test_neg_sm_verified_finalize_goes_finalized);
    RUN_TEST(test_neg_sm_verified_terminate_goes_terminated);
    RUN_TEST(test_neg_sm_verified_unhandled_stays);
    RUN_TEST(test_neg_sm_finalized_absorbs_all_events);
    RUN_TEST(test_neg_sm_terminated_absorbs_all_events);
    RUN_TEST(test_neg_sm_full_happy_path);
    RUN_TEST(test_neg_init_returns_ok);
    RUN_TEST(test_neg_count_active_zero_after_init);
    RUN_TEST(test_neg_create_null_cpid_returns_neg1);
    RUN_TEST(test_neg_create_null_ppid_returns_neg1);
    RUN_TEST(test_neg_create_returns_zero_first_slot);
    RUN_TEST(test_neg_create_state_is_requested);
    RUN_TEST(test_neg_apply_offer_event_gives_offered);
    RUN_TEST(test_neg_get_consumer_pid_correct);
    RUN_TEST(test_neg_get_provider_pid_correct);
    RUN_TEST(test_neg_find_by_cpid_returns_correct_index);
    RUN_TEST(test_neg_find_by_cpid_returns_neg1_if_not_found);
    RUN_TEST(test_neg_find_by_cpid_null_returns_neg1);
    RUN_TEST(test_neg_table_overflow_returns_neg1);
    RUN_TEST(test_neg_count_active_tracks_creates);
    RUN_TEST(test_neg_apply_invalid_index_returns_initial);
    RUN_TEST(test_neg_get_state_invalid_index_returns_initial);
    RUN_TEST(test_neg_get_pids_invalid_index_returns_null);
    RUN_TEST(test_neg_register_handlers_before_init_returns_invalid_state);
    RUN_TEST(test_neg_register_handlers_after_init_returns_ok);
    RUN_TEST(test_neg_double_init_safe);

    /* dsp_neg agree path (DSP-403) */
    RUN_TEST(test_neg_apply_agree_event_from_offered);
    RUN_TEST(test_neg_state_is_agreed_after_offer_then_agree);
    RUN_TEST(test_neg_find_by_cpid_after_agree_still_works);

    /* dsp_neg termination – disabled flag + slot-level (DSP-404) */
    RUN_TEST(test_neg_terminate_flag_is_disabled_by_default);
    RUN_TEST(test_neg_apply_terminate_from_requested);
    RUN_TEST(test_neg_apply_terminate_from_offered);
    RUN_TEST(test_neg_apply_terminate_from_agreed);
    RUN_TEST(test_neg_terminated_state_is_stable);

    /* dsp_xfer SM + slot table + notify (DSP-405) */
    RUN_TEST(test_xfer_sm_initial_start_goes_transferring);
    RUN_TEST(test_xfer_sm_initial_unhandled_stays);
    RUN_TEST(test_xfer_sm_transferring_complete_goes_completed);
    RUN_TEST(test_xfer_sm_transferring_fail_goes_failed);
    RUN_TEST(test_xfer_sm_transferring_unhandled_stays);
    RUN_TEST(test_xfer_sm_completed_absorbs_all);
    RUN_TEST(test_xfer_sm_failed_absorbs_all);
    RUN_TEST(test_xfer_init_returns_ok);
    RUN_TEST(test_xfer_is_initialized_false_before_init);
    RUN_TEST(test_xfer_is_initialized_true_after_init);
    RUN_TEST(test_xfer_count_active_zero_after_init);
    RUN_TEST(test_xfer_create_null_pid_returns_neg1);
    RUN_TEST(test_xfer_create_returns_zero_first_slot);
    RUN_TEST(test_xfer_create_state_is_initial);
    RUN_TEST(test_xfer_apply_start_gives_transferring);
    RUN_TEST(test_xfer_get_state_invalid_index_returns_initial);
    RUN_TEST(test_xfer_get_process_id_correct);
    RUN_TEST(test_xfer_get_process_id_invalid_returns_null);
    RUN_TEST(test_xfer_find_by_pid_returns_index);
    RUN_TEST(test_xfer_find_by_pid_null_returns_neg1);
    RUN_TEST(test_xfer_find_by_pid_not_found_returns_neg1);
    RUN_TEST(test_xfer_table_overflow_returns_neg1);
    RUN_TEST(test_xfer_count_active_tracks_creates);
    RUN_TEST(test_xfer_double_init_safe);
    RUN_TEST(test_xfer_notify_null_by_default);
    RUN_TEST(test_xfer_notify_called_on_start);
    RUN_TEST(test_xfer_notify_not_called_on_complete);
    RUN_TEST(test_xfer_notify_not_called_when_already_transferring);
    RUN_TEST(test_xfer_register_handlers_before_init_returns_invalid_state);
    RUN_TEST(test_xfer_register_handlers_after_init_returns_ok);
    RUN_TEST(test_xfer_is_initialized_false_after_deinit);

    /* dsp_catalog (DSP-401) */
    RUN_TEST(test_catalog_dataset_id_is_nonempty);
    RUN_TEST(test_catalog_title_is_nonempty);
    RUN_TEST(test_catalog_json_buf_size_sufficient);
    RUN_TEST(test_catalog_is_initialized_false_before_init);
    RUN_TEST(test_catalog_init_returns_ok);
    RUN_TEST(test_catalog_is_initialized_true_after_init);
    RUN_TEST(test_catalog_is_initialized_false_after_deinit);
    RUN_TEST(test_catalog_double_init_safe);
    RUN_TEST(test_catalog_double_deinit_safe);
    RUN_TEST(test_catalog_get_json_null_buf_returns_null_arg);
    RUN_TEST(test_catalog_get_json_buf_too_small_returns_error);
    RUN_TEST(test_catalog_get_json_returns_ok);
    RUN_TEST(test_catalog_get_json_context_field);
    RUN_TEST(test_catalog_get_json_type_is_catalog);
    RUN_TEST(test_catalog_get_json_title_matches_config);
    RUN_TEST(test_catalog_get_json_dataset_is_array);
    RUN_TEST(test_catalog_register_handler_before_init_returns_invalid_state);
    RUN_TEST(test_catalog_register_handler_after_init_returns_ok);

    /* dsp_build message builders (DSP-304) */
    RUN_TEST(test_build_error_codes_distinct);
    RUN_TEST(test_build_catalog_null_out_returns_null_arg);
    RUN_TEST(test_build_catalog_null_dataset_id_returns_null_arg);
    RUN_TEST(test_build_catalog_null_title_returns_null_arg);
    RUN_TEST(test_build_catalog_buf_too_small_returns_error);
    RUN_TEST(test_build_catalog_valid_returns_ok);
    RUN_TEST(test_build_catalog_output_fields);
    RUN_TEST(test_build_agreement_null_out_returns_null_arg);
    RUN_TEST(test_build_agreement_null_process_id_returns_null_arg);
    RUN_TEST(test_build_agreement_null_agreement_id_returns_null_arg);
    RUN_TEST(test_build_agreement_buf_too_small_returns_error);
    RUN_TEST(test_build_agreement_valid_returns_ok);
    RUN_TEST(test_build_agreement_output_fields);
    RUN_TEST(test_build_neg_event_null_out_returns_null_arg);
    RUN_TEST(test_build_neg_event_null_process_id_returns_null_arg);
    RUN_TEST(test_build_neg_event_null_state_returns_null_arg);
    RUN_TEST(test_build_neg_event_buf_too_small_returns_error);
    RUN_TEST(test_build_neg_event_valid_returns_ok);
    RUN_TEST(test_build_neg_event_output_fields);
    RUN_TEST(test_build_xfer_start_null_out_returns_null_arg);
    RUN_TEST(test_build_xfer_start_null_process_id_returns_null_arg);
    RUN_TEST(test_build_xfer_start_null_endpoint_returns_null_arg);
    RUN_TEST(test_build_xfer_start_buf_too_small_returns_error);
    RUN_TEST(test_build_xfer_start_valid_returns_ok);
    RUN_TEST(test_build_xfer_start_output_fields);
    RUN_TEST(test_build_xfer_completion_null_out_returns_null_arg);
    RUN_TEST(test_build_xfer_completion_null_process_id_returns_null_arg);
    RUN_TEST(test_build_xfer_completion_buf_too_small_returns_error);
    RUN_TEST(test_build_xfer_completion_valid_returns_ok);
    RUN_TEST(test_build_xfer_completion_output_fields);
    RUN_TEST(test_build_error_null_out_returns_null_arg);
    RUN_TEST(test_build_error_null_code_returns_null_arg);
    RUN_TEST(test_build_error_null_reason_returns_null_arg);
    RUN_TEST(test_build_error_buf_too_small_returns_error);
    RUN_TEST(test_build_error_valid_returns_ok);
    RUN_TEST(test_build_error_output_fields);

    /* dsp_msg validators (DSP-303) */
    RUN_TEST(test_msg_error_codes_are_distinct);
    RUN_TEST(test_msg_cat_req_null_returns_null_input);
    RUN_TEST(test_msg_cat_req_bad_json_returns_parse_error);
    RUN_TEST(test_msg_cat_req_missing_context_returns_error);
    RUN_TEST(test_msg_cat_req_wrong_type_returns_error);
    RUN_TEST(test_msg_cat_req_valid_returns_ok);
    RUN_TEST(test_msg_cat_req_with_optional_filter_returns_ok);
    RUN_TEST(test_msg_offer_null_returns_null_input);
    RUN_TEST(test_msg_offer_bad_json_returns_parse_error);
    RUN_TEST(test_msg_offer_missing_context_returns_error);
    RUN_TEST(test_msg_offer_wrong_type_returns_error);
    RUN_TEST(test_msg_offer_missing_process_id_returns_missing_field);
    RUN_TEST(test_msg_offer_missing_offer_obj_returns_missing_field);
    RUN_TEST(test_msg_offer_offer_field_is_string_returns_missing_field);
    RUN_TEST(test_msg_offer_valid_returns_ok);
    RUN_TEST(test_msg_agreement_null_returns_null_input);
    RUN_TEST(test_msg_agreement_bad_json_returns_parse_error);
    RUN_TEST(test_msg_agreement_missing_context_returns_error);
    RUN_TEST(test_msg_agreement_wrong_type_returns_error);
    RUN_TEST(test_msg_agreement_missing_process_id_returns_missing_field);
    RUN_TEST(test_msg_agreement_missing_agreement_obj_returns_missing_field);
    RUN_TEST(test_msg_agreement_agreement_field_is_string_returns_missing_field);
    RUN_TEST(test_msg_agreement_valid_returns_ok);
    RUN_TEST(test_msg_xfer_null_returns_null_input);
    RUN_TEST(test_msg_xfer_bad_json_returns_parse_error);
    RUN_TEST(test_msg_xfer_missing_context_returns_error);
    RUN_TEST(test_msg_xfer_wrong_type_returns_error);
    RUN_TEST(test_msg_xfer_missing_process_id_returns_missing_field);
    RUN_TEST(test_msg_xfer_missing_dataset_returns_missing_field);
    RUN_TEST(test_msg_xfer_valid_returns_ok);
    RUN_TEST(test_msg_catalog_request_rejected_by_offer_validator);

    /* dsp_jsonld_ctx (DSP-302) */
    RUN_TEST(test_jsonld_context_url_is_defined);
    RUN_TEST(test_jsonld_context_url_contains_dspace_domain);
    RUN_TEST(test_jsonld_prefix_dspace_ends_with_colon);
    RUN_TEST(test_jsonld_prefix_dcat_ends_with_colon);
    RUN_TEST(test_jsonld_prefix_odrl_ends_with_colon);
    RUN_TEST(test_jsonld_field_context_value);
    RUN_TEST(test_jsonld_field_type_value);
    RUN_TEST(test_jsonld_field_id_value);
    RUN_TEST(test_jsonld_common_fields_have_dspace_prefix);
    RUN_TEST(test_jsonld_type_catalog_request_has_dspace_prefix);
    RUN_TEST(test_jsonld_type_catalog_has_dcat_prefix);
    RUN_TEST(test_jsonld_negotiation_types_have_dspace_prefix);
    RUN_TEST(test_jsonld_negotiation_types_are_distinct);
    RUN_TEST(test_jsonld_transfer_types_have_dspace_prefix);
    RUN_TEST(test_jsonld_transfer_types_are_distinct);
    RUN_TEST(test_jsonld_type_error_has_dspace_prefix);
    RUN_TEST(test_jsonld_neg_states_have_dspace_prefix);
    RUN_TEST(test_jsonld_neg_states_are_distinct);
    RUN_TEST(test_jsonld_xfer_states_have_dspace_prefix);
    RUN_TEST(test_jsonld_xfer_states_are_distinct);
    RUN_TEST(test_jsonld_constants_work_with_dsp_json);

    /* dsp_json (DSP-301) */
    RUN_TEST(test_json_cjson_version_is_1_7);
    RUN_TEST(test_json_parse_valid_returns_non_null);
    RUN_TEST(test_json_parse_invalid_returns_null);
    RUN_TEST(test_json_parse_null_returns_null);
    RUN_TEST(test_json_delete_null_is_safe);
    RUN_TEST(test_json_get_string_existing_key);
    RUN_TEST(test_json_get_string_missing_key_returns_false);
    RUN_TEST(test_json_get_string_null_obj_returns_false);
    RUN_TEST(test_json_get_string_null_key_returns_false);
    RUN_TEST(test_json_get_string_buf_too_small_returns_false);
    RUN_TEST(test_json_get_string_exact_fit);
    RUN_TEST(test_json_get_type_returns_correct_value);
    RUN_TEST(test_json_get_context_returns_correct_value);
    RUN_TEST(test_json_has_mandatory_fields_both_present);
    RUN_TEST(test_json_has_mandatory_fields_missing_type);
    RUN_TEST(test_json_has_mandatory_fields_missing_context);
    RUN_TEST(test_json_has_mandatory_fields_null_returns_false);
    RUN_TEST(test_json_new_object_returns_non_null);
    RUN_TEST(test_json_add_string_roundtrip);
    RUN_TEST(test_json_add_string_null_args_return_false);
    RUN_TEST(test_json_print_to_buffer);
    RUN_TEST(test_json_print_buffer_too_small_returns_false);
    RUN_TEST(test_json_print_alloc_and_free);
    RUN_TEST(test_json_print_null_returns_false);

    /* dsp_daps (DSP-205) */
    RUN_TEST(test_daps_init_returns_ok);
    RUN_TEST(test_daps_is_initialized_false_before_init);
    RUN_TEST(test_daps_is_initialized_true_after_init);
    RUN_TEST(test_daps_is_initialized_false_after_deinit);
    RUN_TEST(test_daps_double_init_safe);
    RUN_TEST(test_daps_double_deinit_safe);
    RUN_TEST(test_daps_request_null_buf_returns_invalid_arg);
    RUN_TEST(test_daps_request_zero_cap_returns_invalid_arg);
    RUN_TEST(test_daps_request_null_out_len_returns_invalid_arg);
    RUN_TEST(test_daps_request_before_init_returns_not_init);
    RUN_TEST(test_daps_shim_disabled_by_default);
    RUN_TEST(test_daps_gateway_url_empty_by_default);
    RUN_TEST(test_daps_request_shim_disabled_returns_disabled);
    RUN_TEST(test_daps_max_token_len_nonzero);
    RUN_TEST(test_daps_max_token_len_fits_typical_dat);
    RUN_TEST(test_daps_error_codes_are_distinct);
    RUN_TEST(test_daps_esp_platform_absent_in_host_build);

    return UNITY_END();
}

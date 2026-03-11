/**
 * @file mbedtls_config_dsp.h
 * @brief DSP Embedded mbedTLS configuration reference.
 *
 * This header is NOT a live mbedTLS config override (ESP-IDF does not support
 * MBEDTLS_CONFIG_FILE injection via a component header in this version).
 * It is the authoritative written record of every mbedTLS Kconfig choice made
 * in sdkconfig.defaults for this project.
 *
 * When any Kconfig option is changed, this file MUST be updated to match.
 *
 * Target: ESP32-S3, ≤50 KB heap at peak TLS handshake (including I/O buffers).
 *
 * =========================================================================
 * ENABLED features
 * =========================================================================
 *
 *  TLS protocol
 *  ------------
 *  MBEDTLS_TLS_SERVER_ONLY      – provider-only role; no outgoing TLS
 *  MBEDTLS_SSL_PROTO_TLS1_2     – TLS 1.2 for backward-compatible counterparts
 *  MBEDTLS_SSL_PROTO_TLS1_3     – TLS 1.3 for modern counterparts
 *  MBEDTLS_SSL_ALPN             – ALPN extension, required for HTTP/1.1 on TLS
 *  MBEDTLS_SERVER_SSL_SESSION_TICKETS – server-side session resumption
 *
 *  Key exchange (cipher suites)
 *  ----------------------------
 *  MBEDTLS_KEY_EXCHANGE_ECDHE_RSA   – ECDHE forward secrecy with RSA cert
 *  MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA – ECDHE forward secrecy with ECDSA cert
 *
 *  Symmetric encryption
 *  --------------------
 *  MBEDTLS_AES_C     – AES (via hardware accelerator on ESP32-S3)
 *  MBEDTLS_GCM_C     – AES-GCM AEAD mode (TLS 1.2 GCM suites + TLS 1.3)
 *  MBEDTLS_HARDWARE_AES  – route AES through ESP32-S3 hardware engine
 *  MBEDTLS_HARDWARE_GCM  – GCM hardware acceleration
 *
 *  Hash / MAC
 *  ----------
 *  MBEDTLS_SHA256_C  – SHA-256 (via hardware accelerator on ESP32-S3)
 *  MBEDTLS_SHA1_C    – SHA-1 retained for X.509 signature verification
 *  MBEDTLS_HARDWARE_SHA – route SHA through ESP32-S3 hardware engine
 *  MBEDTLS_HKDF_C    – HKDF key derivation required by TLS 1.3
 *
 *  Asymmetric / ECC
 *  ----------------
 *  MBEDTLS_RSA_C              – RSA for certificate operations
 *  MBEDTLS_HARDWARE_MPI       – bignum via ESP32-S3 hardware (speeds RSA)
 *  MBEDTLS_ECP_C              – elliptic curve base
 *  MBEDTLS_ECDH_C             – ECDH key exchange
 *  MBEDTLS_ECDSA_C            – ECDSA signatures (JWT ES256, TLS client auth)
 *  MBEDTLS_ECDSA_DETERMINISTIC – RFC 6979 deterministic ECDSA
 *  MBEDTLS_HARDWARE_ECC       – ECC hardware acceleration on ESP32-S3
 *  MBEDTLS_HARDWARE_ECDSA_SIGN    – hardware ECDSA signing
 *  MBEDTLS_HARDWARE_ECDSA_VERIFY  – hardware ECDSA verification
 *
 *  Elliptic curves
 *  ---------------
 *  MBEDTLS_ECP_DP_SECP256R1_ENABLED  – P-256: JWT ES256, TLS ECDHE, ECDSA cert
 *  MBEDTLS_ECP_DP_SECP384R1_ENABLED  – P-384: RS384 cert option
 *  MBEDTLS_ECP_DP_CURVE25519_ENABLED – X25519: TLS 1.3 key exchange
 *
 *  X.509 / PEM
 *  -----------
 *  MBEDTLS_PEM_PARSE_C  – parse PEM-encoded certificates and keys
 *
 *  Memory optimisation
 *  -------------------
 *  MBEDTLS_ASYMMETRIC_CONTENT_LEN  – separate in/out buffer sizes
 *  MBEDTLS_SSL_IN_CONTENT_LEN=4096  – receive buffer (default 16 KB → 4 KB)
 *  MBEDTLS_SSL_OUT_CONTENT_LEN=4096 – send buffer    (default 16 KB → 4 KB)
 *  MBEDTLS_DYNAMIC_BUFFER          – allocate I/O buffers only during handshake
 *  MBEDTLS_DYNAMIC_FREE_CONFIG_DATA – free config struct after handshake
 *  MBEDTLS_DYNAMIC_FREE_CA_CERT    – free CA cert after chain verification
 *  MBEDTLS_SSL_KEEP_PEER_CERTIFICATE=n – do not retain peer cert after handshake
 *
 * =========================================================================
 * DISABLED features (RAM/flash savings)
 * =========================================================================
 *
 *  TLS versions
 *  ------------
 *  MBEDTLS_SSL_PROTO_DTLS=n    – no DTLS (HTTP/1.1 over TCP only)
 *
 *  Key exchange (disabled)
 *  -----------------------
 *  MBEDTLS_KEY_EXCHANGE_RSA=n        – static RSA: no forward secrecy
 *  MBEDTLS_KEY_EXCHANGE_DHE_RSA=n    – DHE-RSA: slower than ECDHE, no HW accel
 *  MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA=n – static ECDH: no forward secrecy
 *  MBEDTLS_KEY_EXCHANGE_ECDH_RSA=n   – static ECDH: no forward secrecy
 *  MBEDTLS_PSK_MODES=n               – PSK ciphersuites (added in DSP-204)
 *
 *  Symmetric ciphers (not needed)
 *  ------------------------------
 *  MBEDTLS_CAMELLIA_C=n
 *  MBEDTLS_DES_C=n      – insecure, deprecated
 *  MBEDTLS_BLOWFISH_C=n
 *  MBEDTLS_XTEA_C=n
 *  MBEDTLS_CCM_C=n      – AES-CCM; AES-GCM is preferred
 *  MBEDTLS_CHACHAPOLY_C=n – ChaCha20-Poly1305; no hardware accel on S3
 *
 *  Hash (not needed)
 *  -----------------
 *  MBEDTLS_SHA512_C=n   – not used by TLS 1.2/1.3 suites we enable
 *  MBEDTLS_SHA3_C=n
 *  MBEDTLS_RIPEMD160_C=n
 *  MBEDTLS_CMAC_C=n
 *
 *  ECC curves (not needed)
 *  -----------------------
 *  MBEDTLS_ECP_DP_SECP192R1_ENABLED=n
 *  MBEDTLS_ECP_DP_SECP224R1_ENABLED=n
 *  MBEDTLS_ECP_DP_SECP521R1_ENABLED=n – too large for embedded
 *  MBEDTLS_ECP_DP_SECP192K1_ENABLED=n
 *  MBEDTLS_ECP_DP_SECP224K1_ENABLED=n
 *  MBEDTLS_ECP_DP_SECP256K1_ENABLED=n – Koblitz/Bitcoin, not standard TLS
 *  MBEDTLS_ECP_DP_BP256R1_ENABLED=n   – Brainpool (not required)
 *  MBEDTLS_ECP_DP_BP384R1_ENABLED=n
 *  MBEDTLS_ECP_DP_BP512R1_ENABLED=n
 *
 *  Misc (not needed)
 *  -----------------
 *  MBEDTLS_CERTIFICATE_BUNDLE=n – we use pre-provisioned certs, not a CA bundle
 *  MBEDTLS_DHM_C=n              – DH (using ECDHE instead)
 *  MBEDTLS_ECJPAKE_C=n          – J-PAKE, not used
 *  MBEDTLS_X509_CRL_PARSE_C=n  – no CRL support
 *  MBEDTLS_X509_CSR_PARSE_C=n  – no CSR generation/parsing
 *  MBEDTLS_PEM_WRITE_C=n        – not needed (we only parse)
 *  MBEDTLS_SSL_RENEGOTIATION=n  – security risk, disable
 *  MBEDTLS_ECP_RESTARTABLE=n    – adds complexity without benefit
 *  MBEDTLS_CLIENT_SSL_SESSION_TICKETS=n – server-only role
 *  MBEDTLS_ERROR_STRINGS=n      – saves flash; decode errors offline if needed
 *  MBEDTLS_NIST_KW_C=n
 *
 * =========================================================================
 * RAM budget estimate (peak, during TLS handshake)
 * =========================================================================
 *
 *  ssl_context static alloc                 ~1.5 KB
 *  ssl_config static alloc                  ~0.5 KB
 *  IN  I/O buffer (DYNAMIC, during handshake) 4.0 KB
 *  OUT I/O buffer (DYNAMIC, during handshake) 4.0 KB
 *  RSA/ECDH scratch space                   ~8–12 KB
 *  X.509 cert parsing temp                  ~4–6 KB
 *  Handshake state machine                  ~4–8 KB
 *  ─────────────────────────────────────────────────
 *  Estimated peak (DYNAMIC_BUFFER enabled)  ~25–35 KB
 *  Target budget                            ≤50 KB  ✓
 *
 * =========================================================================
 */

#pragma once

/* This header is intentionally empty at runtime.
 * All configuration is applied via Kconfig / sdkconfig.defaults.
 * It exists solely as a documentation artefact. */

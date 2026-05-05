/*
 * Endpoint registrasi kartu (PrepaidRegistrationApi.kt di MyXL APK).
 *
 * Engsel sebelumnya cuma punya 1 endpoint registrasi (`api/v8/auth/regist/dukcapil`
 * minimal: msisdn+nik+kk+lang). MyXL native pakai full flow dengan banyak
 * endpoint yang punya semantik berbeda — file ini menambahkan semuanya.
 *
 * Reference smali: classes8/ex/k.smali (PrepaidRegistrationApi.kt) di APK 9.2.0.
 *
 * Mapping endpoint:
 *   /api/v8/infos/regist/info             – cek status reg untuk MSISDN
 *   /api/v8/auth/check-dukcapil           – validate NIK+KK+MSISDN sebelum submit
 *   /api/v8/auth/regist/dukcapil          – simple register (msisdn+nik+kk),
 *                                            untuk akun yang sudah aktif
 *   auth/regist/dukcapil                   – register dengan PUK + isAutoPair=false
 *                                            (untuk SIM yang sudah pernah aktif tapi
 *                                            butuh re-pair NIK)
 *   auth/regist/dukcapil-autopair          – register dengan PUK + isAutoPair=true
 *                                            (UNTUK SIM BARU yang belum pernah dipakai)
 *   /api/v8/infos/validate-puk             – validate PUK SIM (V8)
 *   /api/v9/infos/validate-puk             – validate PUK SIM (V9, lebih baru)
 *   /api/v8/auth/regist/request-otp        – request OTP untuk konfirmasi NIK
 *   /api/v8/auth/regist/validate-otp       – validate OTP yang dikirim ke nomor
 *   /api/v8/auth/regist/biometric-attempt  – cek apakah nomor butuh biometric
 *
 * SEMUA function balikin cJSON* yang harus di-free oleh caller.
 */
#ifndef ENGSEL_CLIENT_REGISTRATION_H
#define ENGSEL_CLIENT_REGISTRATION_H

#include "../cJSON.h"

/* === Cek & validasi (read-only) =========================================== */

/* Cek status registrasi MSISDN.
 * Response.data: { registration_status, is_nik_active, eligible_biometric,
 *                  name, display_nik, display_kk, registration_date } */
cJSON* registration_get_info(const char* base, const char* api_key,
                             const char* xdata_key, const char* api_secret,
                             const char* id_token, const char* msisdn);

/* Validate kombinasi NIK+KK+MSISDN sebelum submit register. Endpoint ini
 * dipakai App MyXL untuk pre-check di form registrasi. */
cJSON* registration_check_dukcapil(const char* base, const char* api_key,
                                   const char* xdata_key, const char* api_secret,
                                   const char* id_token,
                                   const char* msisdn,
                                   const char* nik, const char* kk);

/* Validate PUK SIM. V8 dan V9 endpoint — V8 untuk akun lama, V9 dipakai
 * MyXL 9.2.0 keatas. Coba V9 dulu, fallback V8. */
cJSON* registration_validate_puk_v8(const char* base, const char* api_key,
                                    const char* xdata_key, const char* api_secret,
                                    const char* id_token,
                                    const char* msisdn, const char* puk);

cJSON* registration_validate_puk_v9(const char* base, const char* api_key,
                                    const char* xdata_key, const char* api_secret,
                                    const char* id_token,
                                    const char* msisdn, const char* puk);

/* Cek apakah nomor butuh biometric verification (KTP + selfie + liveness).
 * Engsel TIDAK bisa lakukan biometric flow di CLI — ini cuma untuk inform user
 * bahwa mereka harus pakai App MyXL official. */
cJSON* registration_biometric_attempt(const char* base, const char* api_key,
                                      const char* xdata_key, const char* api_secret,
                                      const char* id_token, const char* msisdn);

/* === Submit (write operations) ============================================ */

/* Simple register untuk SIM yang sudah aktif, hanya butuh re-pair NIK+KK.
 * Endpoint: /api/v8/auth/regist/dukcapil. */
cJSON* registration_dukcapil_simple(const char* base, const char* api_key,
                                    const char* xdata_key, const char* api_secret,
                                    const char* id_token,
                                    const char* msisdn,
                                    const char* nik, const char* kk);

/* Register dengan PUK untuk SIM yang sudah pernah pair tapi butuh ganti owner.
 * isAutoPair=false. Endpoint: auth/regist/dukcapil (relatif). */
cJSON* registration_dukcapil_with_puk(const char* base, const char* api_key,
                                      const char* xdata_key, const char* api_secret,
                                      const char* id_token,
                                      const char* msisdn,
                                      const char* nik, const char* kk,
                                      const char* puk);

/* Register SIM BARU dengan PUK + autopair. isAutoPair=true.
 * Endpoint: auth/regist/dukcapil-autopair (relatif). */
cJSON* registration_dukcapil_autopair(const char* base, const char* api_key,
                                      const char* xdata_key, const char* api_secret,
                                      const char* id_token,
                                      const char* msisdn,
                                      const char* nik, const char* kk,
                                      const char* puk);

/* === OTP flow ============================================================= */

/* Request OTP ke MSISDN. Server kirim SMS dengan kode 6 digit. */
cJSON* registration_request_otp(const char* base, const char* api_key,
                                const char* xdata_key, const char* api_secret,
                                const char* id_token,
                                const char* msisdn);

/* Validate OTP yang user terima via SMS. Setelah ini, server kasih session
 * token untuk lanjut ke step register. */
cJSON* registration_validate_otp(const char* base, const char* api_key,
                                 const char* xdata_key, const char* api_secret,
                                 const char* id_token,
                                 const char* msisdn, const char* otp);

/* === Error mapping ======================================================== */

/* Map error code dari response register ke pesan Indonesia user-friendly.
 * Sumber: strings.xml MyXL (values-in/strings.xml di APK).
 * Return string statis, JANGAN di-free. */
const char* registration_error_message(const char* code);

#endif

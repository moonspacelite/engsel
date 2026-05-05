/*
 * Implementasi endpoint registrasi (PrepaidRegistrationApi.kt).
 * Lihat include/client/registration.h untuk dokumentasi per-function.
 *
 * --- POST-MAINTENANCE API CHANGE ---
 * Setelah maintenance XL (~Apr 2026), banyak endpoint registrasi mulai
 * mengembalikan code 111 REQUEST_BODY_MALFORMED untuk payload format lama
 * (flat JSON dengan field langsung). Server sekarang ekspektasi payload
 * dibungkus dalam envelope `{"data": {...}}`.
 *
 * Endpoint yang TER-KONFIRMASI BEKERJA dengan envelope wrapper:
 *   - api/v8/infos/regist/info       (200 SUCCESS)
 *   - api/v9/infos/validate-puk      (135 INVALID_REQUEST → format OK,
 *                                       PUK fake = expected)
 *
 * Endpoint yang MASIH 111 walau pakai wrapper (butuh investigasi lanjut,
 * mungkin butuh field signature/device-id tambahan):
 *   - api/v8/auth/regist/dukcapil
 *   - api/v8/auth/regist/dukcapil-autopair
 *   - api/v8/auth/regist/biometric-attempt
 *   - api/v8/infos/validate-puk (V8 — pakai V9 sebagai default)
 *
 * Endpoint yang return non-standard response (perlu probe ulang):
 *   - api/v8/auth/regist/request-otp
 *   - api/v8/auth/regist/validate-otp
 */
#include <stdlib.h>
#include <string.h>

#include "../include/cJSON.h"
#include "../include/client/registration.h"
#include "../include/client/engsel.h"

/* Bungkus payload dalam envelope `{"data": <payload>}`.
 * Ownership of `inner` is transferred to the wrapper — caller hanya perlu
 * Delete the wrapper. */
static cJSON* wrap_data(cJSON* inner) {
    cJSON* w = cJSON_CreateObject();
    cJSON_AddItemToObject(w, "data", inner);
    return w;
}

/* === Cek & validasi ======================================================= */

cJSON* registration_get_info(const char* base, const char* api_key,
                             const char* xdata_key, const char* api_secret,
                             const char* id_token, const char* msisdn) {
    /* Confirmed WORKING dengan wrapper post-maintenance. */
    cJSON* inner = cJSON_CreateObject();
    cJSON_AddStringToObject(inner, "msisdn", msisdn ? msisdn : "");
    cJSON* p = wrap_data(inner);
    cJSON* r = send_api_request(base, api_key, xdata_key, api_secret,
                                "api/v8/infos/regist/info", p,
                                id_token ? id_token : "", "POST", NULL);
    cJSON_Delete(p);
    return r;
}

cJSON* registration_check_dukcapil(const char* base, const char* api_key,
                                   const char* xdata_key, const char* api_secret,
                                   const char* id_token,
                                   const char* msisdn,
                                   const char* nik, const char* kk) {
    /* DTO LoginMsisdnRequestDto: msisdn + 5 with_* boolean.
     * NIK + KK belum tentu valid di endpoint ini (check-dukcapil sebenarnya
     * shared dengan flow login MSISDN). Kita pass tetap untuk back-compat. */
    cJSON* inner = cJSON_CreateObject();
    cJSON_AddStringToObject(inner, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(inner, "nik",    nik    ? nik    : "");
    cJSON_AddStringToObject(inner, "kk",     kk     ? kk     : "");
    cJSON_AddBoolToObject  (inner, "with_regist_status", 1);
    cJSON_AddBoolToObject  (inner, "with_bizon",         0);
    cJSON_AddBoolToObject  (inner, "with_family_plan",   0);
    cJSON_AddBoolToObject  (inner, "with_optimus",       0);
    cJSON_AddBoolToObject  (inner, "with_enterprise",    0);
    cJSON_AddBoolToObject  (inner, "is_enterprise",      0);
    cJSON_AddStringToObject(inner, "lang", "en");
    cJSON* p = wrap_data(inner);
    cJSON* r = send_api_request(base, api_key, xdata_key, api_secret,
                                "api/v8/auth/check-dukcapil", p,
                                id_token ? id_token : "", "POST", NULL);
    cJSON_Delete(p);
    return r;
}

static cJSON* validate_puk_impl(const char* base, const char* api_key,
                                const char* xdata_key, const char* api_secret,
                                const char* id_token,
                                const char* path,
                                const char* msisdn, const char* puk) {
    /* Confirmed WORKING (V9) dengan wrapper post-maintenance. */
    cJSON* inner = cJSON_CreateObject();
    cJSON_AddStringToObject(inner, "msisdn",        msisdn ? msisdn : "");
    cJSON_AddStringToObject(inner, "puk",           puk    ? puk    : "");
    cJSON_AddBoolToObject  (inner, "is_enterprise", 0);
    cJSON_AddBoolToObject  (inner, "is_enc",        0);
    cJSON_AddStringToObject(inner, "lang", "en");
    cJSON* p = wrap_data(inner);
    cJSON* r = send_api_request(base, api_key, xdata_key, api_secret,
                                path, p,
                                id_token ? id_token : "", "POST", NULL);
    cJSON_Delete(p);
    return r;
}

cJSON* registration_validate_puk_v8(const char* base, const char* api_key,
                                    const char* xdata_key, const char* api_secret,
                                    const char* id_token,
                                    const char* msisdn, const char* puk) {
    return validate_puk_impl(base, api_key, xdata_key, api_secret, id_token,
                             "api/v8/infos/validate-puk", msisdn, puk);
}

cJSON* registration_validate_puk_v9(const char* base, const char* api_key,
                                    const char* xdata_key, const char* api_secret,
                                    const char* id_token,
                                    const char* msisdn, const char* puk) {
    return validate_puk_impl(base, api_key, xdata_key, api_secret, id_token,
                             "api/v9/infos/validate-puk", msisdn, puk);
}

cJSON* registration_biometric_attempt(const char* base, const char* api_key,
                                      const char* xdata_key, const char* api_secret,
                                      const char* id_token, const char* msisdn) {
    cJSON* inner = cJSON_CreateObject();
    cJSON_AddStringToObject(inner, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(inner, "lang", "en");
    cJSON* p = wrap_data(inner);
    cJSON* r = send_api_request(base, api_key, xdata_key, api_secret,
                                "api/v8/auth/regist/biometric-attempt", p,
                                id_token ? id_token : "", "POST", NULL);
    cJSON_Delete(p);
    return r;
}

/* === Submit (write operations) ============================================ */

cJSON* registration_dukcapil_simple(const char* base, const char* api_key,
                                    const char* xdata_key, const char* api_secret,
                                    const char* id_token,
                                    const char* msisdn,
                                    const char* nik, const char* kk) {
    /* PENDING: post-maintenance endpoint masih return 111 walau dengan
     * wrapper. Kita tetap kirim format wrapper karena itu protokol baru,
     * tapi user akan dapat error 111 kalau backend belum siap. */
    cJSON* inner = cJSON_CreateObject();
    cJSON_AddStringToObject(inner, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(inner, "nik",    nik    ? nik    : "");
    cJSON_AddStringToObject(inner, "kk",     kk     ? kk     : "");
    cJSON_AddStringToObject(inner, "lang", "en");
    cJSON* p = wrap_data(inner);
    cJSON* r = send_api_request(base, api_key, xdata_key, api_secret,
                                "api/v8/auth/regist/dukcapil", p,
                                id_token ? id_token : "", "POST", NULL);
    cJSON_Delete(p);
    return r;
}

static cJSON* dukcapil_with_puk_impl(const char* base, const char* api_key,
                                     const char* xdata_key, const char* api_secret,
                                     const char* id_token,
                                     const char* path,
                                     const char* msisdn,
                                     const char* nik, const char* kk,
                                     const char* puk,
                                     int is_auto_pair) {
    /* DTO PrepaidRegisterRequestDto: msisdn, nik, kk, puk, is_auto_pair. */
    cJSON* inner = cJSON_CreateObject();
    cJSON_AddStringToObject(inner, "msisdn",       msisdn ? msisdn : "");
    cJSON_AddStringToObject(inner, "nik",          nik    ? nik    : "");
    cJSON_AddStringToObject(inner, "kk",           kk     ? kk     : "");
    cJSON_AddStringToObject(inner, "puk",          puk    ? puk    : "");
    cJSON_AddBoolToObject  (inner, "is_auto_pair", is_auto_pair ? 1 : 0);
    cJSON_AddStringToObject(inner, "lang", "en");
    cJSON* p = wrap_data(inner);
    cJSON* r = send_api_request(base, api_key, xdata_key, api_secret,
                                path, p,
                                id_token ? id_token : "", "POST", NULL);
    cJSON_Delete(p);
    return r;
}

cJSON* registration_dukcapil_with_puk(const char* base, const char* api_key,
                                      const char* xdata_key, const char* api_secret,
                                      const char* id_token,
                                      const char* msisdn,
                                      const char* nik, const char* kk,
                                      const char* puk) {
    /* Path relatif (tanpa leading slash, tanpa /api/v8/ prefix) — server
     * resolve ke base URL Retrofit yang berbeda dari path absolut. */
    return dukcapil_with_puk_impl(base, api_key, xdata_key, api_secret, id_token,
                                  "auth/regist/dukcapil",
                                  msisdn, nik, kk, puk, 0);
}

cJSON* registration_dukcapil_autopair(const char* base, const char* api_key,
                                      const char* xdata_key, const char* api_secret,
                                      const char* id_token,
                                      const char* msisdn,
                                      const char* nik, const char* kk,
                                      const char* puk) {
    return dukcapil_with_puk_impl(base, api_key, xdata_key, api_secret, id_token,
                                  "auth/regist/dukcapil-autopair",
                                  msisdn, nik, kk, puk, 1);
}

/* === OTP flow ============================================================= */

cJSON* registration_request_otp(const char* base, const char* api_key,
                                const char* xdata_key, const char* api_secret,
                                const char* id_token,
                                const char* msisdn) {
    cJSON* inner = cJSON_CreateObject();
    cJSON_AddStringToObject(inner, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(inner, "lang", "en");
    cJSON* p = wrap_data(inner);
    cJSON* r = send_api_request(base, api_key, xdata_key, api_secret,
                                "api/v8/auth/regist/request-otp", p,
                                id_token ? id_token : "", "POST", NULL);
    cJSON_Delete(p);
    return r;
}

cJSON* registration_validate_otp(const char* base, const char* api_key,
                                 const char* xdata_key, const char* api_secret,
                                 const char* id_token,
                                 const char* msisdn, const char* otp) {
    cJSON* inner = cJSON_CreateObject();
    cJSON_AddStringToObject(inner, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(inner, "otp",    otp    ? otp    : "");
    cJSON_AddStringToObject(inner, "lang", "en");
    cJSON* p = wrap_data(inner);
    cJSON* r = send_api_request(base, api_key, xdata_key, api_secret,
                                "api/v8/auth/regist/validate-otp", p,
                                id_token ? id_token : "", "POST", NULL);
    cJSON_Delete(p);
    return r;
}

/* === Error mapping ======================================================== */

const char* registration_error_message(const char* code) {
    if (!code || !code[0]) return "Terjadi kesalahan tidak dikenal saat registrasi.";

    /* Subset dari values-in/strings.xml MyXL (key dimulai dengan 'rgs_'). */
    if (strcmp(code, "164") == 0) {
        return "NIK Anda sudah mencapai batas maksimum registrasi nomor "
               "prabayar (3 nomor per NIK). Silakan hubungi Live Chat XL "
               "atau kunjungi XL Center terdekat.";
    }
    if (strcmp(code, "165") == 0) {
        return "Data NIK / Nomor KK Anda tidak valid atau tidak sinkron "
               "dengan database Dukcapil. Periksa kembali kombinasi NIK & "
               "KK Anda. Bisa juga coba dengan PUK (SIM baru).";
    }
    if (strcmp(code, "166") == 0) {
        return "Kode PUK Anda tidak valid. Pastikan PUK diambil dari "
               "kemasan SIM yang masih original.";
    }
    if (strcmp(code, "167") == 0) {
        return "Kombinasi MSISDN dan PUK tidak valid. PUK ini bukan untuk "
               "nomor yang Anda masukkan.";
    }
    if (strcmp(code, "168") == 0) {
        return "SIM ini sudah teraktivasi. Tidak perlu registrasi PUK ulang.";
    }
    if (strcmp(code, "169") == 0) {
        return "Sesi registrasi expired atau OTP salah. Silakan request OTP "
               "baru dan submit ulang.";
    }
    if (strcmp(code, "170") == 0) {
        return "Mohon maaf, nomor gagal terhubung. Silakan coba lagi dengan "
               "nomor XL lain. Pastikan nomor yang dimasukkan belum terdaftar "
               "sebagai anggota Kuota Bersama.";
    }
    if (strcmp(code, "171") == 0) {
        return "Nomor yang dihubungkan tidak memenuhi syarat Kuota Bersama. "
               "Silakan coba lagi dengan nomor lain.";
    }
    if (strcmp(code, "174") == 0) {
        return "Nomor yang Anda masukkan tidak terdaftar sebagai nomor XL "
               "prabayar atau sudah tidak aktif.";
    }
    if (strcmp(code, "175") == 0) {
        return "Nomor butuh registrasi via biometric (KTP + selfie + liveness). "
               "Engsel CLI tidak mendukung biometric — gunakan App MyXL.";
    }
    if (strcmp(code, "176") == 0) {
        return "Server Dukcapil sedang tidak tersedia. Silakan coba lagi "
               "beberapa saat lagi.";
    }
    if (strcmp(code, "REQUEST_BODY_MALFORMED") == 0) {
        return "Format request salah. Engsel mungkin perlu update payload "
               "(server-side rule berubah).";
    }
    if (strcmp(code, "RATE_LIMIT_EXCEEDED") == 0) {
        return "Terlalu banyak percobaan registrasi. Tunggu beberapa menit "
               "sebelum mencoba lagi.";
    }
    return "Sepertinya terjadi kesalahan. Mohon coba beberapa saat lagi.";
}

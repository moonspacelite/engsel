#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <openssl/evp.h>
#include "../include/cJSON.h"
#include "../include/client/transfer.h"
#include "../include/client/ciam.h"
#include "../include/client/engsel.h"
#include "../include/client/http_client.h"
#include "../include/service/crypto_aes.h"
#include "../include/service/crypto_helper.h"
#include "../include/util/json_util.h"

/* ---- util ---- */

static char* b64_encode_str(const char* in) {
    size_t len = strlen(in);
    size_t out_len = 4 * ((len + 2) / 3) + 1;
    char* out = malloc(out_len);
    if (!out) return NULL;
    int n = EVP_EncodeBlock((unsigned char*)out, (const unsigned char*)in, (int)len);
    out[n] = 0;
    return out;
}

/* Port dari `generate_ax_fingerprint`/`generate_ax_device_id` di ciam.c.
 * Dideklarasi di ciam.c sebagai static; kita duplikasi minimal yang dibutuhkan
 * untuk panggilan CIAM extra (authorization-token/generate).
 *
 * Strategi paling aman: panggil http_get/post langsung dari sini, pakai
 * header Ax-* dari file cache fingerprint (/etc/engsel/ax.fp). */

/* ---- get stage_token ---- */

char* ciam_generate_share_balance_token(const char* base_ciam_url,
                                        const char* ua,
                                        const char* access_token,
                                        const char* pin_6_digits,
                                        const char* receiver_msisdn) {
    if (!base_ciam_url || !access_token || !pin_6_digits || !receiver_msisdn)
        return NULL;

    char url[512];
    snprintf(url, sizeof(url), "%s/ciam/auth/authorization-token/generate", base_ciam_url);

    char* fp = ciam_helper_load_fingerprint();
    if (!fp) { fprintf(stderr, "[transfer] gagal load ax fingerprint\n"); return NULL; }
    char* dev_id = ciam_helper_device_id();
    char* req_at = ciam_helper_timestamp_header();
    char req_id[37]; ciam_helper_uuid_v4(req_id);

    char auth_hdr[4096];
    snprintf(auth_hdr, sizeof(auth_hdr), "Authorization: Bearer %s", access_token);
    char ua_hdr[512];
    snprintf(ua_hdr, sizeof(ua_hdr), "User-Agent: %s",
             ua && *ua ? ua
             : "myXL / 8.9.0(1202); com.android.vending; (samsung; SM-N935F; SDK 33; Android 13)");
    char fp_hdr[1024];
    snprintf(fp_hdr, sizeof(fp_hdr), "Ax-Fingerprint: %s", fp);
    char dev_hdr[256];
    snprintf(dev_hdr, sizeof(dev_hdr), "Ax-Device-Id: %s", dev_id ? dev_id : "");
    char at_hdr[128];
    snprintf(at_hdr, sizeof(at_hdr), "Ax-Request-At: %s", req_at ? req_at : "");
    char ri_hdr[128];
    snprintf(ri_hdr, sizeof(ri_hdr), "Ax-Request-Id: %s", req_id);

    const char* headers[] = {
        "Content-Type: application/json",
        "Ax-Request-Device: samsung",
        "Ax-Request-Device-Model: SM-N935F",
        "Ax-Substype: PREPAID",
        auth_hdr, ua_hdr, fp_hdr, dev_hdr, at_hdr, ri_hdr
    };

    char* pin_b64 = b64_encode_str(pin_6_digits);
    cJSON* body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "pin", pin_b64 ? pin_b64 : "");
    cJSON_AddStringToObject(body, "transaction_type", "SHARE_BALANCE");
    cJSON_AddStringToObject(body, "receiver_msisdn", receiver_msisdn);
    char* body_str = cJSON_PrintUnformatted(body);

    struct HttpResponse* resp = http_post(url, headers,
                                          sizeof(headers)/sizeof(headers[0]),
                                          body_str);

    char* auth_code = NULL;
    if (resp && resp->body) {
        cJSON* j = cJSON_Parse(resp->body);
        if (j) {
            const char* status = json_get_str(j, "status", "");
            if (strcmp(status, "Success") == 0) {
                cJSON* data = cJSON_GetObjectItem(j, "data");
                if (data) {
                    const char* ac = json_get_str(data, "authorization_code", "");
                    if (ac && ac[0]) auth_code = strdup(ac);
                }
            } else {
                fprintf(stderr, "[transfer] get_auth_code gagal, status=%s\n",
                        status[0] ? status : resp->body);
            }
            cJSON_Delete(j);
        } else {
            fprintf(stderr, "[transfer] get_auth_code JSON parse gagal: %s\n",
                    resp->body);
        }
    }

    free(pin_b64);
    free(body_str);
    cJSON_Delete(body);
    free(fp); free(dev_id); free(req_at);
    free_http_response(resp);
    return auth_code;
}

/* ---- balance allotment ---- */

cJSON* balance_allotment(const char* base_api_url,
                         const char* api_key,
                         const char* xdata_key,
                         const char* x_api_base_secret,
                         const char* id_token,
                         const char* access_token,
                         const char* stage_token,
                         const char* receiver_msisdn,
                         int amount) {
    if (!base_api_url || !api_key || !xdata_key || !x_api_base_secret || !id_token ||
        !access_token || !stage_token || !receiver_msisdn) return NULL;

    const char* path = "sharings/api/v8/balance/allotment";

    /* Payload */
    cJSON* payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "access_token", access_token);
    cJSON_AddStringToObject(payload, "receiver", receiver_msisdn);
    cJSON_AddNumberToObject(payload, "amount", amount);
    cJSON_AddStringToObject(payload, "stage_token", stage_token);
    cJSON_AddStringToObject(payload, "lang", "en");
    cJSON_AddBoolToObject(payload, "is_enterprise", 0);

    /* Build signature — pakai waktu sekarang, harus sama dengan ts yang di-pass
     * ke send_api_request. send_api_request meng-generate ts-nya sendiri jadi
     * signature kustom kita akan MISMATCH waktunya. Untuk itu kita bypass
     * send_api_request dan kirim langsung pakai libcurl. */

    struct timeval tv; gettimeofday(&tv, NULL);
    long long xtime = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    long sig_time_sec = (long)(xtime / 1000);

    char* x_sig = make_x_signature_balance_allotment(x_api_base_secret,
                                                     access_token,
                                                     sig_time_sec,
                                                     receiver_msisdn,
                                                     amount,
                                                     path);
    if (!x_sig) { cJSON_Delete(payload); return NULL; }

    char* plain_body = cJSON_PrintUnformatted(payload);

    char* xdata = encrypt_xdata(plain_body, xtime, xdata_key);

    cJSON* wrap = cJSON_CreateObject();
    cJSON_AddStringToObject(wrap, "xdata", xdata ? xdata : "");
    cJSON_AddNumberToObject(wrap, "xtime", (double)xtime);
    char* wrap_str = cJSON_PrintUnformatted(wrap);

    char sig_time_str[32]; snprintf(sig_time_str, sizeof(sig_time_str), "%ld", sig_time_sec);
    char req_id[37]; ciam_helper_uuid_v4(req_id);
    char* req_at = ciam_helper_timestamp_header();

    char url[512];
    snprintf(url, sizeof(url), "%s/%s", base_api_url, path);

    char auth_hdr[4096];
    snprintf(auth_hdr, sizeof(auth_hdr), "Authorization: Bearer %s", id_token);
    char api_hdr[128];
    snprintf(api_hdr, sizeof(api_hdr), "x-api-key: %s", api_key);
    char st_hdr[64];
    snprintf(st_hdr, sizeof(st_hdr), "x-signature-time: %s", sig_time_str);
    char sg_hdr[512];
    snprintf(sg_hdr, sizeof(sg_hdr), "x-signature: %s", x_sig);
    char ri_hdr[128];
    snprintf(ri_hdr, sizeof(ri_hdr), "x-request-id: %s", req_id);
    char ra_hdr[128];
    snprintf(ra_hdr, sizeof(ra_hdr), "x-request-at: %s", req_at ? req_at : "");

    const char* headers[] = {
        "Content-Type: application/json; charset=utf-8",
        "User-Agent: myXL / 8.9.0(1202); com.android.vending; (samsung; SM-N935F; SDK 33; Android 13)",
        "x-hv: v3",
        "x-version-app: 8.9.0",
        auth_hdr, api_hdr, st_hdr, sg_hdr, ri_hdr, ra_hdr
    };

    struct HttpResponse* resp = http_post(url, headers,
                                          sizeof(headers)/sizeof(headers[0]),
                                          wrap_str);

    cJSON* result = NULL;
    if (resp && resp->body) {
        cJSON* j = cJSON_Parse(resp->body);
        if (j) {
            cJSON* rx = cJSON_GetObjectItem(j, "xdata");
            cJSON* rt = cJSON_GetObjectItem(j, "xtime");
            if (rx && cJSON_IsString(rx) && rt) {
                char* dec = decrypt_xdata(rx->valuestring,
                                          (long long)rt->valuedouble,
                                          xdata_key);
                if (dec) { result = cJSON_Parse(dec); free(dec); }
            } else {
                result = cJSON_Duplicate(j, 1);
            }
            cJSON_Delete(j);
        } else {
            fprintf(stderr, "[transfer] balance_allotment parse fail: %s\n", resp->body);
        }
    }

    free(x_sig); free(plain_body); free(xdata); free(wrap_str); free(req_at);
    cJSON_Delete(payload); cJSON_Delete(wrap);
    free_http_response(resp);
    return result;
}

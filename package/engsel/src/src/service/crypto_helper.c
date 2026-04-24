#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include "../include/service/crypto_helper.h"

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/* Format printf-style ke heap string, caller free(). Kembalikan NULL jika error. */
static char *xvasprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0) return NULL;

    char *buf = malloc((size_t)n + 1);
    if (!buf) return NULL;

    va_start(ap, fmt);
    vsnprintf(buf, (size_t)n + 1, fmt, ap);
    va_end(ap);
    return buf;
}

static char *hmac_hex(const EVP_MD *md, const char *key, const char *msg) {
    if (!key || !msg) return NULL;
    unsigned int dlen = EVP_MAX_MD_SIZE;
    unsigned char digest[EVP_MAX_MD_SIZE];
    if (HMAC(md, (unsigned char *)key, (int)strlen(key),
             (unsigned char *)msg, strlen(msg), digest, &dlen) == NULL) return NULL;

    char *hex = malloc(dlen * 2 + 1);
    if (!hex) return NULL;
    static const char H[] = "0123456789abcdef";
    for (unsigned int i = 0; i < dlen; i++) {
        hex[i * 2]     = H[(digest[i] >> 4) & 0xF];
        hex[i * 2 + 1] = H[digest[i] & 0xF];
    }
    hex[dlen * 2] = '\0';
    return hex;
}

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

char *make_x_signature(const char *x_api_base_secret,
                       const char *id_token,
                       const char *method,
                       const char *path,
                       long sig_time_sec) {
    if (!x_api_base_secret || !id_token || !method || !path) return NULL;
    char *key = xvasprintf("%s;%s;%s;%s;%ld",
                           x_api_base_secret, id_token, method, path, sig_time_sec);
    char *msg = xvasprintf("%s;%ld;", id_token, sig_time_sec);
    if (!key || !msg) { free(key); free(msg); return NULL; }
    char *sig = hmac_hex(EVP_sha512(), key, msg);
    free(key); free(msg);
    return sig;
}

char *make_x_signature_payment(const char *secret,
                               const char *access_token,
                               long sig_time_sec,
                               const char *package_code,
                               const char *token_payment,
                               const char *payment_method,
                               const char *payment_for,
                               const char *path) {
    if (!secret || !access_token || !package_code || !token_payment ||
        !payment_method || !payment_for || !path) return NULL;

    char *key = xvasprintf("%s;%ld#ae-hei_9Tee6he+Ik3Gais5=;POST;%s;%ld",
                           secret, sig_time_sec, path, sig_time_sec);
    char *msg = xvasprintf("%s;%s;%ld;%s;%s;%s;",
                           access_token, token_payment, sig_time_sec,
                           payment_for, payment_method, package_code);
    if (!key || !msg) { free(key); free(msg); return NULL; }
    char *sig = hmac_hex(EVP_sha512(), key, msg);
    free(key); free(msg);
    return sig;
}

char *make_x_signature_bounty(const char *secret,
                              const char *access_token,
                              long sig_time_sec,
                              const char *package_code,
                              const char *token_payment) {
    if (!secret || !access_token || !package_code || !token_payment) return NULL;
    const char *path = "api/v8/personalization/bounties-exchange";
    char *key = xvasprintf("%s;%s;%ld#ae-hei_9Tee6he+Ik3Gais5=;POST;%s;%ld",
                           secret, access_token, sig_time_sec, path, sig_time_sec);
    char *msg = xvasprintf("%s;%s;%ld;%s;",
                           access_token, token_payment, sig_time_sec, package_code);
    if (!key || !msg) { free(key); free(msg); return NULL; }
    char *sig = hmac_hex(EVP_sha512(), key, msg);
    free(key); free(msg);
    return sig;
}

char *make_x_signature_bounty_allotment(const char *secret,
                                        long sig_time_sec,
                                        const char *package_code,
                                        const char *token_confirmation,
                                        const char *path,
                                        const char *destination_msisdn) {
    if (!secret || !package_code || !token_confirmation || !path || !destination_msisdn)
        return NULL;

    char *key = xvasprintf("%s;%ld#ae-hei_9Tee6he+Ik3Gais5=;%s;POST;%s;%ld",
                           secret, sig_time_sec, destination_msisdn, path, sig_time_sec);
    char *msg = xvasprintf("%s;%ld;%s;%s;",
                           token_confirmation, sig_time_sec, destination_msisdn, package_code);
    if (!key || !msg) { free(key); free(msg); return NULL; }
    char *sig = hmac_hex(EVP_sha512(), key, msg);
    free(key); free(msg);
    return sig;
}

char *make_x_signature_balance_allotment(const char *secret,
                                         const char *access_token,
                                         long sig_time_sec,
                                         const char *receiver_msisdn,
                                         int amount,
                                         const char *path) {
    if (!secret || !access_token || !receiver_msisdn || !path) return NULL;

    char *key = xvasprintf("%s;%ld#ae-hei_9Tee6he+Ik3Gais5=;%s;POST;%s;%ld",
                           secret, sig_time_sec, receiver_msisdn, path, sig_time_sec);
    char *msg = xvasprintf("%s;%ld;%s;%d;",
                           access_token, sig_time_sec, receiver_msisdn, amount);
    if (!key || !msg) { free(key); free(msg); return NULL; }
    char *sig = hmac_hex(EVP_sha512(), key, msg);
    free(key); free(msg);
    return sig;
}

char *make_x_signature_loyalty(const char *secret,
                               long sig_time_sec,
                               const char *package_code,
                               const char *token_confirmation,
                               const char *path) {
    if (!secret || !package_code || !token_confirmation || !path) return NULL;
    char *key = xvasprintf("%s;%ld#ae-hei_9Tee6he+Ik3Gais5=;POST;%s;%ld",
                           secret, sig_time_sec, path, sig_time_sec);
    char *msg = xvasprintf("%s;%ld;%s;",
                           token_confirmation, sig_time_sec, package_code);
    if (!key || !msg) { free(key); free(msg); return NULL; }
    char *sig = hmac_hex(EVP_sha512(), key, msg);
    free(key); free(msg);
    return sig;
}

char *make_x_signature_basic(const char *secret,
                             const char *method,
                             const char *path,
                             long sig_time_sec) {
    if (!secret || !method || !path) return NULL;
    char *key = xvasprintf("%s;%s;%s;%ld", secret, method, path, sig_time_sec);
    char *msg = xvasprintf("%ld;en;", sig_time_sec);
    if (!key || !msg) { free(key); free(msg); return NULL; }
    char *sig = hmac_hex(EVP_sha512(), key, msg);
    free(key); free(msg);
    return sig;
}

char *make_ax_api_signature(const char *ax_api_sig_key,
                            const char *ts_for_sign,
                            const char *contact,
                            const char *code,
                            const char *contact_type) {
    if (!ax_api_sig_key || !ts_for_sign || !contact || !code || !contact_type) return NULL;

    char *preimage = xvasprintf("%spassword%s%s%sopenid",
                                ts_for_sign, contact_type, contact, code);
    if (!preimage) return NULL;

    unsigned int dlen = 32;
    unsigned char digest[EVP_MAX_MD_SIZE];
    if (HMAC(EVP_sha256(), (unsigned char *)ax_api_sig_key, (int)strlen(ax_api_sig_key),
             (unsigned char *)preimage, strlen(preimage), digest, &dlen) == NULL) {
        free(preimage);
        return NULL;
    }
    free(preimage);

    /* Standard base64 (non-urlsafe). */
    size_t b64_len = 4 * ((dlen + 2) / 3);
    char *b64 = malloc(b64_len + 1);
    if (!b64) return NULL;
    EVP_EncodeBlock((unsigned char *)b64, digest, (int)dlen);
    return b64;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include "../include/service/crypto_aes.h"

/* ---------------------------------------------------------------------------
 * Utilitas PRNG
 * ------------------------------------------------------------------------- */
static int csprng_bytes(unsigned char *buf, size_t len) {
    if (RAND_bytes(buf, (int)len) == 1) return 0;
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return -1;
    size_t n = fread(buf, 1, len, f);
    fclose(f);
    return (n == len) ? 0 : -1;
}

static void bytes_to_hex(const unsigned char *in, size_t len, char *out) {
    static const char H[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[i * 2]     = H[(in[i] >> 4) & 0xF];
        out[i * 2 + 1] = H[in[i] & 0xF];
    }
    out[len * 2] = '\0';
}

/* Derive IV dari 16 karakter pertama hexdigest SHA-256(xtime_ms). */
static void derive_iv(long long xtime_ms, unsigned char *iv) {
    char xtime_str[32];
    int n = snprintf(xtime_str, sizeof(xtime_str), "%lld", xtime_ms);
    if (n < 0 || n >= (int)sizeof(xtime_str)) n = (int)sizeof(xtime_str) - 1;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)xtime_str, (size_t)n, hash);

    char hex_hash[SHA256_DIGEST_LENGTH * 2 + 1];
    bytes_to_hex(hash, SHA256_DIGEST_LENGTH, hex_hash);
    memcpy(iv, hex_hash, 16);
}

/* ---------------------------------------------------------------------------
 * Base64 helper
 * ------------------------------------------------------------------------- */
static char *b64_url_from_bytes(const unsigned char *buf, size_t length) {
    size_t b64_len = 4 * ((length + 2) / 3);
    char *b64_text = malloc(b64_len + 1);
    if (!b64_text) return NULL;
    EVP_EncodeBlock((unsigned char *)b64_text, buf, (int)length);
    for (char *p = b64_text; *p; p++) {
        if (*p == '+')      *p = '-';
        else if (*p == '/') *p = '_';
    }
    return b64_text;
}

static char *b64_std_from_bytes(const unsigned char *buf, size_t length) {
    size_t b64_len = 4 * ((length + 2) / 3);
    char *out = malloc(b64_len + 1);
    if (!out) return NULL;
    EVP_EncodeBlock((unsigned char *)out, buf, (int)length);
    return out;
}

/* Dekode urlsafe/standard base64 (menerima +/, -_, dengan/ tanpa padding). */
static unsigned char *b64_decode_flex(const char *b64_text, size_t *out_len) {
    if (!b64_text) return NULL;
    size_t len = strlen(b64_text);
    size_t padded_len = len + ((4 - (len % 4)) % 4);

    char *padded = malloc(padded_len + 1);
    if (!padded) return NULL;
    memcpy(padded, b64_text, len);
    for (size_t i = len; i < padded_len; i++) padded[i] = '=';
    padded[padded_len] = '\0';

    for (char *p = padded; *p; p++) {
        if (*p == '-')      *p = '+';
        else if (*p == '_') *p = '/';
    }

    unsigned char *out = malloc(padded_len ? padded_len : 1);
    if (!out) { free(padded); return NULL; }

    int decoded = EVP_DecodeBlock(out, (unsigned char *)padded, (int)padded_len);
    int pad_count = 0;
    if (padded_len > 0 && padded[padded_len - 1] == '=') pad_count++;
    if (padded_len > 1 && padded[padded_len - 2] == '=') pad_count++;
    free(padded);

    if (decoded < 0) { free(out); return NULL; }
    *out_len = (size_t)(decoded - pad_count);
    return out;
}

/* ---------------------------------------------------------------------------
 * xdata
 * ------------------------------------------------------------------------- */
char *encrypt_xdata(const char *plaintext, long long xtime_ms, const char *xdata_key) {
    if (!plaintext || !xdata_key) return NULL;
    unsigned char iv[16];
    derive_iv(xtime_ms, iv);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
                           (unsigned char *)xdata_key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    size_t pt_len = strlen(plaintext);
    unsigned char *ct = malloc(pt_len + 32);
    if (!ct) { EVP_CIPHER_CTX_free(ctx); return NULL; }

    int len = 0, ct_len = 0;
    if (EVP_EncryptUpdate(ctx, ct, &len, (unsigned char *)plaintext, (int)pt_len) != 1) {
        free(ct); EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    ct_len = len;
    if (EVP_EncryptFinal_ex(ctx, ct + len, &len) != 1) {
        free(ct); EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    ct_len += len;
    EVP_CIPHER_CTX_free(ctx);

    char *b64 = b64_url_from_bytes(ct, (size_t)ct_len);
    free(ct);
    return b64;
}

char *decrypt_xdata(const char *xdata, long long xtime_ms, const char *xdata_key) {
    if (!xdata || !xdata_key) return NULL;
    unsigned char iv[16];
    derive_iv(xtime_ms, iv);

    size_t ct_len = 0;
    unsigned char *ct = b64_decode_flex(xdata, &ct_len);
    if (!ct || ct_len == 0) { free(ct); return NULL; }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { free(ct); return NULL; }
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
                           (unsigned char *)xdata_key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx); free(ct); return NULL;
    }

    unsigned char *pt = malloc(ct_len + 32);
    if (!pt) { EVP_CIPHER_CTX_free(ctx); free(ct); return NULL; }

    int len = 0, pt_len = 0;
    if (EVP_DecryptUpdate(ctx, pt, &len, ct, (int)ct_len) != 1) {
        free(pt); free(ct); EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    pt_len = len;
    if (EVP_DecryptFinal_ex(ctx, pt + len, &len) != 1) {
        free(pt); free(ct); EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    pt_len += len;
    EVP_CIPHER_CTX_free(ctx);
    free(ct);

    pt[pt_len] = '\0';
    return (char *)pt;
}

/* ---------------------------------------------------------------------------
 * build_encrypted_field
 *
 * Mengenkripsi blok PKCS7 kosong (16 byte 0x10) menggunakan kunci ASCII dan
 * IV = 16 karakter hex acak (8 byte random dikonversi hex).
 * Output: urlsafe_b64(ciphertext) + iv_hex16  (sesuai Python versi
 * urlsafe_b64=True yang dipakai di settlement).
 * ------------------------------------------------------------------------- */
char *build_encrypted_field(const char *enc_field_key) {
    if (!enc_field_key) return NULL;
    size_t key_len = strlen(enc_field_key);
    if (key_len == 0) return NULL;

    unsigned char iv_bytes[8];
    if (csprng_bytes(iv_bytes, sizeof(iv_bytes)) != 0) return NULL;
    char iv_hex[17];
    bytes_to_hex(iv_bytes, 8, iv_hex);

    unsigned char pt[16];
    for (int i = 0; i < 16; i++) pt[i] = 16; /* PKCS7 pad untuk plaintext kosong */

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return NULL;

    const EVP_CIPHER *cipher = (key_len >= 32) ? EVP_aes_256_cbc()
                             : (key_len >= 24) ? EVP_aes_192_cbc()
                                               : EVP_aes_128_cbc();

    if (EVP_EncryptInit_ex(ctx, cipher, NULL,
                           (unsigned char *)enc_field_key,
                           (unsigned char *)iv_hex) != 1) {
        EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    unsigned char ct[32]; int len1 = 0, len2 = 0;
    if (EVP_EncryptUpdate(ctx, ct, &len1, pt, 16) != 1) {
        EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    if (EVP_EncryptFinal_ex(ctx, ct + len1, &len2) != 1) {
        EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    EVP_CIPHER_CTX_free(ctx);

    char *b64 = b64_url_from_bytes(ct, (size_t)(len1 + len2));
    if (!b64) return NULL;
    size_t b64_len = strlen(b64);
    char *result = malloc(b64_len + 16 + 1);
    if (!result) { free(b64); return NULL; }
    memcpy(result, b64, b64_len);
    memcpy(result + b64_len, iv_hex, 16);
    result[b64_len + 16] = '\0';
    free(b64);
    return result;
}

/* ---------------------------------------------------------------------------
 * encrypt_circle_msisdn / decrypt_circle_msisdn
 *
 * Format Python (crypto_helper.py):
 *   output = urlsafe_b64( AES-CBC-PKCS7(msisdn) ) + iv_ascii16
 * dimana iv_ascii16 = hex(random 8 bytes).
 * ------------------------------------------------------------------------- */
char *encrypt_circle_msisdn(const char *msisdn, const char *enc_field_key) {
    if (!msisdn || !enc_field_key) return NULL;
    size_t key_len = strlen(enc_field_key);
    if (key_len == 0) return NULL;

    unsigned char iv_bytes[8];
    if (csprng_bytes(iv_bytes, sizeof(iv_bytes)) != 0) return NULL;
    char iv_hex[17];
    bytes_to_hex(iv_bytes, 8, iv_hex);

    size_t msisdn_len = strlen(msisdn);
    size_t pad = 16 - (msisdn_len % 16);
    size_t padded_len = msisdn_len + pad;
    unsigned char *padded = malloc(padded_len);
    if (!padded) return NULL;
    memcpy(padded, msisdn, msisdn_len);
    for (size_t i = msisdn_len; i < padded_len; i++) padded[i] = (unsigned char)pad;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { free(padded); return NULL; }

    const EVP_CIPHER *cipher = (key_len >= 32) ? EVP_aes_256_cbc()
                             : (key_len >= 24) ? EVP_aes_192_cbc()
                                               : EVP_aes_128_cbc();

    if (EVP_EncryptInit_ex(ctx, cipher, NULL,
                           (unsigned char *)enc_field_key,
                           (unsigned char *)iv_hex) != 1) {
        EVP_CIPHER_CTX_free(ctx); free(padded); return NULL;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    unsigned char *ct = malloc(padded_len + 32);
    if (!ct) { EVP_CIPHER_CTX_free(ctx); free(padded); return NULL; }

    int len1 = 0, len2 = 0;
    if (EVP_EncryptUpdate(ctx, ct, &len1, padded, (int)padded_len) != 1) {
        free(ct); free(padded); EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    if (EVP_EncryptFinal_ex(ctx, ct + len1, &len2) != 1) {
        free(ct); free(padded); EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    EVP_CIPHER_CTX_free(ctx);
    free(padded);

    char *b64 = b64_url_from_bytes(ct, (size_t)(len1 + len2));
    free(ct);
    if (!b64) return NULL;

    size_t b64_len = strlen(b64);
    char *result = malloc(b64_len + 16 + 1);
    if (!result) { free(b64); return NULL; }
    memcpy(result, b64, b64_len);
    memcpy(result + b64_len, iv_hex, 16);
    result[b64_len + 16] = '\0';
    free(b64);
    return result;
}

char *decrypt_circle_msisdn(const char *encrypted_b64, const char *enc_field_key) {
    if (!encrypted_b64 || !enc_field_key) return NULL;
    size_t total = strlen(encrypted_b64);
    if (total < 17) return NULL;

    const char *iv_ascii = encrypted_b64 + (total - 16);
    size_t b64_part_len = total - 16;

    char *b64_part = malloc(b64_part_len + 1);
    if (!b64_part) return NULL;
    memcpy(b64_part, encrypted_b64, b64_part_len);
    b64_part[b64_part_len] = '\0';

    size_t ct_len = 0;
    unsigned char *ct = b64_decode_flex(b64_part, &ct_len);
    free(b64_part);
    if (!ct || ct_len == 0 || (ct_len % 16) != 0) { free(ct); return NULL; }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { free(ct); return NULL; }

    size_t key_len = strlen(enc_field_key);
    const EVP_CIPHER *cipher = (key_len >= 32) ? EVP_aes_256_cbc()
                             : (key_len >= 24) ? EVP_aes_192_cbc()
                                               : EVP_aes_128_cbc();

    if (EVP_DecryptInit_ex(ctx, cipher, NULL,
                           (unsigned char *)enc_field_key,
                           (unsigned char *)iv_ascii) != 1) {
        EVP_CIPHER_CTX_free(ctx); free(ct); return NULL;
    }
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    unsigned char *pt = malloc(ct_len + 1);
    if (!pt) { EVP_CIPHER_CTX_free(ctx); free(ct); return NULL; }

    int len1 = 0, len2 = 0;
    if (EVP_DecryptUpdate(ctx, pt, &len1, ct, (int)ct_len) != 1) {
        free(pt); free(ct); EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    if (EVP_DecryptFinal_ex(ctx, pt + len1, &len2) != 1) {
        free(pt); free(ct); EVP_CIPHER_CTX_free(ctx); return NULL;
    }
    EVP_CIPHER_CTX_free(ctx);
    free(ct);

    size_t total_len = (size_t)(len1 + len2);
    if (total_len == 0) { free(pt); return NULL; }
    unsigned char pad = pt[total_len - 1];
    if (pad == 0 || pad > 16 || pad > total_len) { free(pt); return NULL; }
    for (size_t i = total_len - pad; i < total_len; i++) {
        if (pt[i] != pad) { free(pt); return NULL; }
    }
    size_t plain_len = total_len - pad;
    pt[plain_len] = '\0';
    return (char *)pt;
}

/* Supress unused-function warning when this helper isn't used externally. */
__attribute__((unused)) static char *b64_std_from_bytes_unused(const unsigned char *b, size_t n) {
    return b64_std_from_bytes(b, n);
}

#ifndef CRYPTO_AES_H
#define CRYPTO_AES_H

#include <stddef.h>

/* Enkripsi body JSON dengan AES-256-CBC, IV = first-16-hex(SHA256(xtime_ms)).
 * Output: urlsafe base64 (no padding-trim, caller free()). */
char* encrypt_xdata(const char* plaintext, long long xtime_ms, const char* xdata_key);

/* Dekripsi urlsafe-base64 xdata -> JSON plaintext (caller free()). */
char* decrypt_xdata(const char* xdata, long long xtime_ms, const char* xdata_key);

/* Field terenkripsi untuk settlement (encrypted_payment_token,
 * encrypted_authentication_id). Pakai IV acak kriptografis. Caller free(). */
char* build_encrypted_field(const char* enc_field_key);

/* Enkripsi MSISDN untuk fitur Circle/Family-Plan.
 *   Output = urlsafe_b64( AES-CBC(plaintext) ) + iv_hex16
 * Format cocok dengan Python encrypt_circle_msisdn(). */
char* encrypt_circle_msisdn(const char* msisdn, const char* enc_field_key);

/* Dekripsi kembali MSISDN terenkripsi. Kembalikan NULL jika gagal. */
char* decrypt_circle_msisdn(const char* encrypted_b64, const char* enc_field_key);

#endif

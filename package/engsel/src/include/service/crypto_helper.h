#ifndef CRYPTO_HELPER_H
#define CRYPTO_HELPER_H

/* x-signature standar untuk request API umum.
 *   key = "{SECRET};{id_token};{method};{path};{sig_time_sec}"
 *   msg = "{id_token};{sig_time_sec};"
 *   out = hex( HMAC-SHA512(key, msg) ). Caller free(). */
char* make_x_signature(const char* x_api_base_secret,
                       const char* id_token,
                       const char* method,
                       const char* path,
                       long sig_time_sec);

/* x-signature settlement purchase (BALANCE / E-Wallet / QRIS / DANA / DompetKu).
 *   key = "{SECRET};{sig_time_sec}#ae-hei_9Tee6he+Ik3Gais5=;POST;{path};{sig_time_sec}"
 *   msg = "{access_token};{token_payment};{sig_time_sec};{payment_for};{payment_method};{package_code};" */
char* make_x_signature_payment(const char* secret,
                               const char* access_token,
                               long sig_time_sec,
                               const char* package_code,
                               const char* token_payment,
                               const char* payment_method,
                               const char* payment_for,
                               const char* path);

/* bounties-exchange signature. */
char* make_x_signature_bounty(const char* secret,
                              const char* access_token,
                              long sig_time_sec,
                              const char* package_code,
                              const char* token_payment);

/* bounty allotment dengan destination MSISDN. */
char* make_x_signature_bounty_allotment(const char* secret,
                                        long sig_time_sec,
                                        const char* package_code,
                                        const char* token_confirmation,
                                        const char* path,
                                        const char* destination_msisdn);

/* balance allotment (transfer pulsa) signature.
 *   key = "{SECRET};{sig_time_sec}#ae-hei_9Tee6he+Ik3Gais5=;{msisdn};POST;{path};{sig_time_sec}"
 *   msg = "{access_token};{sig_time_sec};{msisdn};{amount};"
 *
 * Formula eksperimen — polanya mengikuti bounty_allotment tapi substitusi
 * {token_confirmation}->{access_token}, {package_code}->{amount}. Kalau XL
 * server reject dengan "signature not match", formula ini perlu disesuaikan. */
char* make_x_signature_balance_allotment(const char* secret,
                                         const char* access_token,
                                         long sig_time_sec,
                                         const char* receiver_msisdn,
                                         int amount,
                                         const char* path);

/* loyalty settlement signature. */
char* make_x_signature_loyalty(const char* secret,
                               long sig_time_sec,
                               const char* package_code,
                               const char* token_confirmation,
                               const char* path);

/* x-signature untuk request ringan (lang=en, dll).
 *   key = "{SECRET};{method};{path};{sig_time_sec}"
 *   msg = "{sig_time_sec};en;" */
char* make_x_signature_basic(const char* secret,
                             const char* method,
                             const char* path,
                             long sig_time_sec);

/* Ax-Api-Signature untuk /auth/otp endpoints.
 *   key = ax_api_sig_key (ASCII)
 *   msg = "{ts}password{contact_type}{contact}{code}openid"
 *   out = base64(HMAC-SHA256). Caller free(). */
char* make_ax_api_signature(const char* ax_api_sig_key,
                            const char* ts_for_sign,
                            const char* contact,
                            const char* code,
                            const char* contact_type);

#endif

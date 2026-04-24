#ifndef TRANSFER_H
#define TRANSFER_H

#include "../cJSON.h"

/* ciam_generate_share_balance_token
 *
 * Minta stage_token untuk transaksi SHARE_BALANCE (transfer pulsa).
 * Endpoint: {BASE_CIAM_URL}/ciam/auth/authorization-token/generate
 * Body: { pin: b64(pin), transaction_type: "SHARE_BALANCE", receiver_msisdn }
 *
 * Return: strdup authorization_code on success. NULL on failure (caller should
 * check stderr for cause). Caller free().
 */
char* ciam_generate_share_balance_token(const char* base_ciam_url,
                                        const char* ua,
                                        const char* access_token,
                                        const char* pin_6_digits,
                                        const char* receiver_msisdn);

/* balance_allotment
 *
 * Transfer pulsa ke receiver_msisdn. Memakai x-signature khusus untuk
 * sharings/api/v8/balance/allotment.
 *
 * signature (saat ini eksperimen — format diturunkan dari pola bounty_allotment):
 *   key = "{X_API_BASE_SECRET};{ts}#ae-hei_9Tee6he+Ik3Gais5=;{msisdn};POST;{path};{ts}"
 *   msg = "{access_token};{ts};{msisdn};{amount};"
 *
 * Return: cJSON response dari server (decrypted xdata). Caller cJSON_Delete.
 */
cJSON* balance_allotment(const char* base_api_url,
                         const char* api_key,
                         const char* xdata_key,
                         const char* x_api_base_secret,
                         const char* id_token,
                         const char* access_token,
                         const char* stage_token,
                         const char* receiver_msisdn,
                         int amount);

#endif

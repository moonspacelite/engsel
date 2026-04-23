#ifndef ENGSEL_CLIENT_REDEEM_H
#define ENGSEL_CLIENT_REDEEM_H

#include "../cJSON.h"

/* Settle bounty exchange (endpoint: api/v8/personalization/bounties-exchange)
   Menggunakan signature variant bounty. Parameter 'sec' = X_API_BASE_SECRET. */
cJSON* redeem_settlement_bounty(const char* base, const char* api_key, const char* xdata_key,
                                const char* sec, const char* enc_key,
                                const char* id_token, const char* access_token,
                                const char* token_confirmation, long ts_to_sign,
                                const char* payment_target, int price,
                                const char* item_name);

/* Settle loyalty tiering exchange (endpoint: gamification/api/v8/loyalties/tiering/exchange)
   Menggunakan signature variant loyalty. */
cJSON* redeem_settlement_loyalty(const char* base, const char* api_key, const char* xdata_key,
                                 const char* sec, const char* id_token,
                                 const char* token_confirmation, long ts_to_sign,
                                 const char* payment_target, int points);

/* Bounty allotment ke MSISDN tujuan (endpoint: gamification/api/v8/loyalties/tiering/bounties-allotment) */
cJSON* redeem_bounty_allotment(const char* base, const char* api_key, const char* xdata_key,
                               const char* sec, const char* id_token,
                               long ts_to_sign, const char* destination_msisdn,
                               const char* item_name, const char* item_code,
                               const char* token_confirmation);

#endif

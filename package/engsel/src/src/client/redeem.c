#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/client/redeem.h"
#include "../include/client/engsel.h"
#include "../include/service/crypto_aes.h"
#include "../include/service/crypto_helper.h"

cJSON* redeem_settlement_bounty(const char* base, const char* api_key, const char* xdata_key,
                                const char* sec, const char* enc_key,
                                const char* id_token, const char* access_token,
                                const char* token_confirmation, long ts_to_sign,
                                const char* payment_target, int price,
                                const char* item_name) {
    const char* path = "api/v8/personalization/bounties-exchange";
    char* enc_tok = build_encrypted_field(enc_key);
    char* enc_auth = build_encrypted_field(enc_key);

    cJSON* p = cJSON_CreateObject();
    cJSON_AddNumberToObject(p, "total_discount", 0);
    cJSON_AddBoolToObject(p, "is_enterprise", 0);
    cJSON_AddStringToObject(p, "payment_token", "");
    cJSON_AddStringToObject(p, "token_payment", "");
    cJSON_AddStringToObject(p, "activated_autobuy_code", "");
    cJSON_AddStringToObject(p, "cc_payment_type", "");
    cJSON_AddBoolToObject(p, "is_myxl_wallet", 0);
    cJSON_AddStringToObject(p, "pin", "");
    cJSON_AddStringToObject(p, "ewallet_promo_id", "");
    cJSON_AddItemToObject(p, "members", cJSON_CreateArray());
    cJSON_AddNumberToObject(p, "total_fee", 0);
    cJSON_AddStringToObject(p, "fingerprint", "");
    cJSON* th = cJSON_AddObjectToObject(p, "autobuy_threshold_setting");
    cJSON_AddStringToObject(th, "label", "");
    cJSON_AddStringToObject(th, "type", "");
    cJSON_AddNumberToObject(th, "value", 0);
    cJSON_AddBoolToObject(p, "is_use_point", 0);
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON_AddStringToObject(p, "payment_method", "BALANCE");
    cJSON_AddNumberToObject(p, "timestamp", (double)ts_to_sign);
    cJSON_AddNumberToObject(p, "points_gained", 0);
    cJSON_AddBoolToObject(p, "can_trigger_rating", 0);
    cJSON_AddItemToObject(p, "akrab_members", cJSON_CreateArray());
    cJSON_AddStringToObject(p, "akrab_parent_alias", "");
    cJSON_AddStringToObject(p, "referral_unique_code", "");
    cJSON_AddStringToObject(p, "coupon", "");
    cJSON_AddStringToObject(p, "payment_for", "REDEEM_VOUCHER");
    cJSON_AddBoolToObject(p, "with_upsell", 0);
    cJSON_AddStringToObject(p, "topup_number", "");
    cJSON_AddStringToObject(p, "stage_token", "");
    cJSON_AddStringToObject(p, "authentication_id", "");
    cJSON_AddStringToObject(p, "encrypted_payment_token", enc_tok ? enc_tok : "");
    cJSON_AddStringToObject(p, "token", "");
    cJSON_AddStringToObject(p, "token_confirmation", token_confirmation ? token_confirmation : "");
    cJSON_AddStringToObject(p, "access_token", access_token ? access_token : "");
    cJSON_AddStringToObject(p, "wallet_number", "");
    cJSON_AddStringToObject(p, "encrypted_authentication_id", enc_auth ? enc_auth : "");

    cJSON* add_data = cJSON_AddObjectToObject(p, "additional_data");
    cJSON_AddNumberToObject(add_data, "original_price", 0);
    cJSON_AddBoolToObject(add_data, "is_spend_limit_temporary", 0);
    cJSON_AddStringToObject(add_data, "migration_type", "");
    cJSON_AddStringToObject(add_data, "akrab_m2m_group_id", "");
    cJSON_AddNumberToObject(add_data, "spend_limit_amount", 0);
    cJSON_AddBoolToObject(add_data, "is_spend_limit", 0);
    cJSON_AddStringToObject(add_data, "mission_id", "");
    cJSON_AddNumberToObject(add_data, "tax", 0);
    cJSON_AddStringToObject(add_data, "benefit_type", "");
    cJSON_AddNumberToObject(add_data, "quota_bonus", 0);
    cJSON_AddStringToObject(add_data, "cashtag", "");
    cJSON_AddBoolToObject(add_data, "is_family_plan", 0);
    cJSON_AddItemToObject(add_data, "combo_details", cJSON_CreateArray());
    cJSON_AddBoolToObject(add_data, "is_switch_plan", 0);
    cJSON_AddNumberToObject(add_data, "discount_recurring", 0);
    cJSON_AddBoolToObject(add_data, "is_akrab_m2m", 0);
    cJSON_AddStringToObject(add_data, "balance_type", "");
    cJSON_AddBoolToObject(add_data, "has_bonus", 0);
    cJSON_AddNumberToObject(add_data, "discount_promo", 0);

    cJSON_AddNumberToObject(p, "total_amount", 0);
    cJSON_AddBoolToObject(p, "is_using_autobuy", 0);
    cJSON* items = cJSON_AddArrayToObject(p, "items");
    cJSON* item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "item_code", payment_target ? payment_target : "");
    cJSON_AddStringToObject(item, "product_type", "");
    cJSON_AddNumberToObject(item, "item_price", price);
    cJSON_AddStringToObject(item, "item_name", item_name ? item_name : "");
    cJSON_AddNumberToObject(item, "tax", 0);
    cJSON_AddItemToArray(items, item);

    char* x_sig = make_x_signature_bounty(sec, access_token, ts_to_sign,
                                          payment_target, token_confirmation);
    cJSON* res = send_api_request(base, api_key, xdata_key, sec, path, p,
                                  id_token, "POST", x_sig);
    free(enc_tok);
    free(enc_auth);
    free(x_sig);
    cJSON_Delete(p);
    return res;
}

cJSON* redeem_settlement_loyalty(const char* base, const char* api_key, const char* xdata_key,
                                 const char* sec, const char* id_token,
                                 const char* token_confirmation, long ts_to_sign,
                                 const char* payment_target, int points) {
    const char* path = "gamification/api/v8/loyalties/tiering/exchange";
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "item_code", payment_target ? payment_target : "");
    cJSON_AddNumberToObject(p, "amount", 0);
    cJSON_AddStringToObject(p, "partner", "");
    cJSON_AddBoolToObject(p, "is_enterprise", 0);
    cJSON_AddStringToObject(p, "item_name", "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON_AddNumberToObject(p, "points", points);
    cJSON_AddNumberToObject(p, "timestamp", (double)ts_to_sign);
    cJSON_AddStringToObject(p, "token_confirmation", token_confirmation ? token_confirmation : "");

    char* x_sig = make_x_signature_loyalty(sec, ts_to_sign, payment_target, token_confirmation, path);
    cJSON* res = send_api_request(base, api_key, xdata_key, sec, path, p,
                                  id_token, "POST", x_sig);
    free(x_sig);
    cJSON_Delete(p);
    return res;
}

cJSON* redeem_bounty_allotment(const char* base, const char* api_key, const char* xdata_key,
                               const char* sec, const char* id_token,
                               long ts_to_sign, const char* destination_msisdn,
                               const char* item_name, const char* item_code,
                               const char* token_confirmation) {
    const char* path = "gamification/api/v8/loyalties/tiering/bounties-allotment";
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "destination_msisdn", destination_msisdn ? destination_msisdn : "");
    cJSON_AddStringToObject(p, "item_code", item_code ? item_code : "");
    cJSON_AddBoolToObject(p, "is_enterprise", 0);
    cJSON_AddStringToObject(p, "item_name", item_name ? item_name : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON_AddNumberToObject(p, "timestamp", (double)time(NULL));
    cJSON_AddStringToObject(p, "token_confirmation", token_confirmation ? token_confirmation : "");

    char* x_sig = make_x_signature_bounty_allotment(sec, ts_to_sign, item_code,
                                                    token_confirmation, path,
                                                    destination_msisdn);
    cJSON* res = send_api_request(base, api_key, xdata_key, sec, path, p,
                                  id_token, "POST", x_sig);
    free(x_sig);
    cJSON_Delete(p);
    return res;
}

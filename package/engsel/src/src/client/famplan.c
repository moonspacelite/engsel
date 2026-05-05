#include <stdlib.h>
#include "../include/client/famplan.h"
#include "../include/client/engsel.h"

static cJSON* post(const char* base, const char* key, const char* xdata, const char* sec,
                   const char* path, cJSON* payload, const char* id_token) {
    return send_api_request(base, key, xdata, sec, path, payload, id_token, "POST", NULL);
}

cJSON* famplan_get_member_info(const char* base, const char* api_key, const char* xdata_key,
                               const char* sec, const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddNumberToObject(p, "group_id", 0);
    cJSON_AddBoolToObject(p, "is_enterprise", 0);
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "sharings/api/v8/family-plan/member-info", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* famplan_validate_msisdn(const char* base, const char* api_key, const char* xdata_key,
                               const char* sec, const char* id_token, const char* msisdn) {
    /* Post-maintenance: check-dukcapil butuh envelope `{"data": {...}}`
     * sama seperti endpoint registrasi. Sebelumnya tanpa wrapper -> 111
     * REQUEST_BODY_MALFORMED. Sinkron dengan validate_msisdn di engsel.c. */
    cJSON* inner = cJSON_CreateObject();
    cJSON_AddBoolToObject(inner, "with_bizon", 1);
    cJSON_AddBoolToObject(inner, "with_family_plan", 1);
    cJSON_AddBoolToObject(inner, "is_enterprise", 0);
    cJSON_AddBoolToObject(inner, "with_optimus", 1);
    cJSON_AddStringToObject(inner, "lang", "en");
    cJSON_AddStringToObject(inner, "msisdn", msisdn ? msisdn : "");
    cJSON_AddBoolToObject(inner, "with_regist_status", 1);
    cJSON_AddBoolToObject(inner, "with_enterprise", 1);
    cJSON* p = cJSON_CreateObject();
    cJSON_AddItemToObject(p, "data", inner);
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "api/v8/auth/check-dukcapil", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* famplan_change_member(const char* base, const char* api_key, const char* xdata_key,
                             const char* sec, const char* id_token,
                             const char* parent_alias, const char* alias, int slot_id,
                             const char* family_member_id, const char* new_msisdn) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "parent_alias", parent_alias ? parent_alias : "");
    cJSON_AddBoolToObject(p, "is_enterprise", 0);
    cJSON_AddNumberToObject(p, "slot_id", slot_id);
    cJSON_AddStringToObject(p, "alias", alias ? alias : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON_AddStringToObject(p, "msisdn", new_msisdn ? new_msisdn : "");
    cJSON_AddStringToObject(p, "family_member_id", family_member_id ? family_member_id : "");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "sharings/api/v8/family-plan/change-member", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* famplan_remove_member(const char* base, const char* api_key, const char* xdata_key,
                             const char* sec, const char* id_token,
                             const char* family_member_id) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddBoolToObject(p, "is_enterprise", 0);
    cJSON_AddStringToObject(p, "family_member_id", family_member_id ? family_member_id : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "sharings/api/v8/family-plan/remove-member", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* famplan_set_quota_limit(const char* base, const char* api_key, const char* xdata_key,
                               const char* sec, const char* id_token,
                               long long original_allocation, long long new_allocation,
                               const char* family_member_id) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddBoolToObject(p, "is_enterprise", 0);
    cJSON* arr = cJSON_AddArrayToObject(p, "member_allocations");
    cJSON* a = cJSON_CreateObject();
    cJSON_AddNumberToObject(a, "new_text_allocation", 0);
    cJSON_AddNumberToObject(a, "original_text_allocation", 0);
    cJSON_AddNumberToObject(a, "original_voice_allocation", 0);
    cJSON_AddNumberToObject(a, "original_allocation", (double)original_allocation);
    cJSON_AddNumberToObject(a, "new_voice_allocation", 0);
    cJSON_AddStringToObject(a, "message", "");
    cJSON_AddNumberToObject(a, "new_allocation", (double)new_allocation);
    cJSON_AddStringToObject(a, "family_member_id", family_member_id ? family_member_id : "");
    cJSON_AddStringToObject(a, "status", "");
    cJSON_AddItemToArray(arr, a);
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "sharings/api/v8/family-plan/allocate-quota", p, id_token);
    cJSON_Delete(p);
    return r;
}

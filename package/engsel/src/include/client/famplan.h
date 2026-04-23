#ifndef ENGSEL_CLIENT_FAMPLAN_H
#define ENGSEL_CLIENT_FAMPLAN_H

#include "../cJSON.h"

cJSON* famplan_get_member_info(const char* base, const char* api_key, const char* xdata_key,
                               const char* sec, const char* id_token);

cJSON* famplan_validate_msisdn(const char* base, const char* api_key, const char* xdata_key,
                               const char* sec, const char* id_token, const char* msisdn);

cJSON* famplan_change_member(const char* base, const char* api_key, const char* xdata_key,
                             const char* sec, const char* id_token,
                             const char* parent_alias, const char* alias, int slot_id,
                             const char* family_member_id, const char* new_msisdn);

cJSON* famplan_remove_member(const char* base, const char* api_key, const char* xdata_key,
                             const char* sec, const char* id_token,
                             const char* family_member_id);

cJSON* famplan_set_quota_limit(const char* base, const char* api_key, const char* xdata_key,
                               const char* sec, const char* id_token,
                               long long original_allocation, long long new_allocation,
                               const char* family_member_id);

#endif

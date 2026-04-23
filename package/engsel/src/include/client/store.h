#ifndef ENGSEL_CLIENT_STORE_H
#define ENGSEL_CLIENT_STORE_H

#include "../cJSON.h"

cJSON* store_get_family_list(const char* base, const char* api_key, const char* xdata_key,
                             const char* sec, const char* id_token,
                             const char* subs_type, int is_enterprise);

cJSON* store_get_packages(const char* base, const char* api_key, const char* xdata_key,
                          const char* sec, const char* id_token,
                          const char* subs_type, int is_enterprise);

cJSON* store_get_segments(const char* base, const char* api_key, const char* xdata_key,
                          const char* sec, const char* id_token, int is_enterprise);

cJSON* store_get_redeemables(const char* base, const char* api_key, const char* xdata_key,
                             const char* sec, const char* id_token, int is_enterprise);

cJSON* store_get_notifications(const char* base, const char* api_key, const char* xdata_key,
                               const char* sec, const char* id_token);

#endif

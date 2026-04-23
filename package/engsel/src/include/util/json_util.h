#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include "../cJSON.h"

/* Ambil string dari field; kembalikan default jika tidak ada atau bukan string. */
const char* json_get_str(cJSON* obj, const char* key, const char* fallback);

/* Ambil integer dari field (valueint); kembalikan default jika tidak ada/numeric. */
int json_get_int(cJSON* obj, const char* key, int fallback);

/* Ambil double (valuedouble) untuk timestamps, prices, dll. */
double json_get_double(cJSON* obj, const char* key, double fallback);

/* true jika obj[key] bernilai "SUCCESS" (case-sensitive, string type). */
int json_status_is_success(cJSON* obj);

/* Nested path e.g. "data.profile.subscriber_id". */
const char* json_get_nested_str(cJSON* obj, const char* dotted_path, const char* fallback);

#endif

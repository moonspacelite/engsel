#include <string.h>
#include "../include/util/json_util.h"

const char *json_get_str(cJSON *obj, const char *key, const char *fallback) {
    if (!obj || !key) return fallback;
    cJSON *n = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!n || !cJSON_IsString(n) || !n->valuestring) return fallback;
    return n->valuestring;
}

int json_get_int(cJSON *obj, const char *key, int fallback) {
    if (!obj || !key) return fallback;
    cJSON *n = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!n) return fallback;
    if (cJSON_IsNumber(n)) return n->valueint;
    if (cJSON_IsBool(n))   return cJSON_IsTrue(n) ? 1 : 0;
    return fallback;
}

double json_get_double(cJSON *obj, const char *key, double fallback) {
    if (!obj || !key) return fallback;
    cJSON *n = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!n || !cJSON_IsNumber(n)) return fallback;
    return n->valuedouble;
}

int json_status_is_success(cJSON *obj) {
    const char *s = json_get_str(obj, "status", NULL);
    return (s && strcmp(s, "SUCCESS") == 0);
}

const char *json_get_nested_str(cJSON *obj, const char *dotted_path, const char *fallback) {
    if (!obj || !dotted_path) return fallback;
    cJSON *cur = obj;
    char buf[256];
    size_t path_len = strlen(dotted_path);
    if (path_len >= sizeof(buf)) return fallback;
    memcpy(buf, dotted_path, path_len + 1);

    char *saveptr = NULL;
    char *tok = strtok_r(buf, ".", &saveptr);
    while (tok && cur) {
        cur = cJSON_GetObjectItemCaseSensitive(cur, tok);
        tok = strtok_r(NULL, ".", &saveptr);
        if (!cur) return fallback;
    }
    if (cur && cJSON_IsString(cur) && cur->valuestring) return cur->valuestring;
    return fallback;
}

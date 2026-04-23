#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include "../include/client/http_client.h"

static CURL        *shared_curl      = NULL;
static int          g_init_done      = 0;
static const char  *g_ca_bundle_path = NULL;

struct MemoryStruct { char *memory; size_t size; };

static size_t write_mem_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->memory = ptr;
    memcpy(mem->memory + mem->size, contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

static void detect_ca_bundle(void) {
    static const char *candidates[] = {
        "/etc/ssl/cert.pem",
        "/etc/ssl/certs/ca-certificates.crt",
        "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        if (access(candidates[i], R_OK) == 0) {
            g_ca_bundle_path = candidates[i];
            return;
        }
    }
    g_ca_bundle_path = NULL;
}

void http_client_init(void) {
    if (g_init_done) return;
    curl_global_init(CURL_GLOBAL_ALL);
    detect_ca_bundle();
    g_init_done = 1;
}

void http_client_cleanup(void) {
    if (shared_curl) {
        curl_easy_cleanup(shared_curl);
        shared_curl = NULL;
    }
    if (g_init_done) {
        curl_global_cleanup();
        g_init_done = 0;
    }
}

static CURL *get_handle(void) {
    if (!g_init_done) http_client_init();
    if (!shared_curl) shared_curl = curl_easy_init();
    else              curl_easy_reset(shared_curl);
    return shared_curl;
}

static void apply_common_opts(CURL *curl, struct MemoryStruct *chunk) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_mem_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS,      5L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,       1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

    if (g_ca_bundle_path) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, g_ca_bundle_path);
    }
}

static struct HttpResponse *run_request(const char *url,
                                        const char *headers[],
                                        int header_count,
                                        const char *method,
                                        const char *payload) {
    struct HttpResponse *response = calloc(1, sizeof(*response));
    if (!response) return NULL;

    struct MemoryStruct chunk = { malloc(1), 0 };
    if (chunk.memory) chunk.memory[0] = '\0';

    CURL *curl = get_handle();
    if (!curl) {
        free(chunk.memory);
        return response;
    }

    struct curl_slist *list = NULL;
    for (int i = 0; i < header_count; i++) {
        if (headers[i]) list = curl_slist_append(list, headers[i]);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    apply_common_opts(curl, &chunk);

    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload ? payload : "");
    } else if (strcmp(method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload ? payload : "");
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    CURLcode rc = curl_easy_perform(curl);
    if (rc == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
        response->body = chunk.memory;
    } else {
        fprintf(stderr, "[HTTP_ERR] %s %s => %s\n", method, url, curl_easy_strerror(rc));
        free(chunk.memory);
    }
    curl_slist_free_all(list);
    return response;
}

struct HttpResponse *http_post(const char *url, const char *headers[], int header_count, const char *payload) {
    return run_request(url, headers, header_count, "POST", payload);
}

struct HttpResponse *http_get(const char *url, const char *headers[], int header_count) {
    return run_request(url, headers, header_count, "GET", NULL);
}

struct HttpResponse *http_put(const char *url, const char *headers[], int header_count, const char *payload) {
    return run_request(url, headers, header_count, "PUT", payload);
}

void free_http_response(struct HttpResponse *resp) {
    if (!resp) return;
    if (resp->body) free(resp->body);
    free(resp);
}

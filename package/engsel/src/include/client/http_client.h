#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

struct HttpResponse {
    char *body;
    long  status_code;
};

/* Panggil sekali di awal main() dan http_client_cleanup() di akhir. */
void http_client_init(void);
void http_client_cleanup(void);

struct HttpResponse* http_post(const char* url, const char* headers[], int header_count, const char* payload);
struct HttpResponse* http_get (const char* url, const char* headers[], int header_count);

/* HTTP PUT dengan JSON/form body. */
struct HttpResponse* http_put (const char* url, const char* headers[], int header_count, const char* payload);

void free_http_response(struct HttpResponse* resp);

#endif

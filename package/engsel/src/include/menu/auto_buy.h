#ifndef ENGSEL_MENU_AUTO_BUY_H
#define ENGSEL_MENU_AUTO_BUY_H

#include "../cJSON.h"

/* Menu UI untuk mengelola entry Auto Buy (dipakai dari features.c).
 * Kredensial (base_api/api_key/xdata/x_api_secret/enc_field_key/id_token) sudah
 * tersedia di call-site; selain itu auto_buy resolve base_ciam/basic_auth/ua
 * via getenv() dan pakai g_tokens_arr global. */
void auto_buy_menu(const char* base_api, const char* api_key,
                   const char* xdata_key, const char* x_api_secret,
                   const char* enc_field_key, const char* id_token);

/* Worker loop yang dipanggil dari `engsel --auto-buy`.
 * Return 0 kalau selesai normal (SIGTERM), non-zero kalau error. */
int auto_buy_worker_main(void);

#endif

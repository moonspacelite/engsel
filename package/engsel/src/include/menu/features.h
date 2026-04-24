/* Forward-declared; implemented di features.c. Auto Buy menu dispatch lewat
 * show_features_menu yang sudah menerima semua kredensial dibutuhkan.  */
#ifndef ENGSEL_MENU_FEATURES_H
#define ENGSEL_MENU_FEATURES_H

/* Submenu "Fitur Lanjutan":
     - Circle info + member management
     - Family Plan info + quota/slot management
     - Store (family-list, search, segments, redeemables, notifications)
     - Redeem voucher (bounty + loyalty + bounty allotment)
   Dipanggil dari main.c setelah user login dengan token aktif. */
void show_features_menu(const char* base_api,
                        const char* api_key,
                        const char* xdata_key,
                        const char* x_api_secret,
                        const char* enc_field_key,
                        const char* id_token,
                        const char* access_token,
                        const char* my_msisdn);

#endif

/* Menu Migrasi & Kontrak (Menu 8 > 9).
 *
 * Sub-menu untuk semua flow migrasi/upgrade subscription:
 *  - PREPAID  -> PRIORITAS    (auths/prio/registration + activation)
 *  - PREPAID  -> PRIOHYBRID   (infos/pre-to-prioh/* dengan KYC upload)
 *  - PRIOHYBRID -> PRIORITAS  (auths/prio/post-hybridactivation)
 *  - Aktivasi MPD 6/12/24m kontrak (store+sharings/pre-to-postpaid/*)
 *  - Recontract / perpanjang (refinancing/recontract/*)
 *  - Trial PRIO 7-hari revertible (registinfo/brand-portability/*)
 *  - Estimasi billing migrasi (sharings/migration/*)
 *
 * Submit operations butuh konfirmasi user 2x karena permanent.
 */
#ifndef ENGSEL_MENU_MIGRATION_H
#define ENGSEL_MENU_MIGRATION_H

void migration_menu(const char* base_api,
                    const char* api_key,
                    const char* xdata_key,
                    const char* x_api_secret,
                    const char* id_token,
                    const char* access_token,
                    const char* my_msisdn);

/* Verify Active Package: cek apakah paket aktif di /balance-and-credit
 * adalah LEGIT (resolvable ke catalog server) atau DECOY (cosmetic only). */
void verify_active_package_menu(const char* base_api,
                                const char* api_key,
                                const char* xdata_key,
                                const char* x_api_secret,
                                const char* id_token);

#endif

/* Menu Migrasi & Kontrak (Menu 8 > 9).
 *
 * Implementasi 14 sub-fitur untuk semua flow migrasi/kontrak subscription.
 * Endpoint discovery dari decompile APK MyXL 9.1.0 — lihat client/migration.h.
 *
 * SAFETY:
 *   - Read-only operation: dijalankan tanpa konfirmasi tambahan
 *   - Submit operation (yang ubah akun permanent): konfirmasi 2x dengan
 *     menampilkan dampak + mengetik string konfirmasi spesifik
 *   - MPD aktivasi: konfirmasi 3x karena melibatkan billing commitment
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "../include/cJSON.h"
#include "../include/client/engsel.h"
#include "../include/client/migration.h"
#include "../include/menu/migration.h"
#include "../include/util/json_util.h"
#include "../include/util/nav.h"

extern char active_subs_type[32];

/* ---------------------------------------------------------------------------
 * Local helpers (sengaja disalin agar migration.c standalone, tidak coupled
 * ke static helpers di features.c)
 * ------------------------------------------------------------------------- */

static void mig_clear(void) { printf("\033[H\033[J"); fflush(stdout); }

static void mig_flush_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void mig_pause(void) {
    printf("\nTekan Enter untuk melanjutkan...");
    fflush(stdout);
    mig_flush_line();
}

static void mig_read_line(const char* prompt, char* buf, size_t n) {
    printf("%s", prompt); fflush(stdout);
    if (!fgets(buf, (int)n, stdin)) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\n")] = '\0';
}

static int mig_read_yn(const char* prompt) {
    char b[8]; mig_read_line(prompt, b, sizeof(b));
    return (b[0] == 'y' || b[0] == 'Y');
}

static void mig_dump(const char* label, cJSON* res) {
    printf("\n=== %s ===\n", label);
    if (!res) { printf("[-] Tidak ada response (network/decrypt error).\n"); return; }
    char* out = cJSON_Print(res);
    if (out) {
        printf("%s\n", out);
        free(out);
    }
}

/* Cek status response dari API. Print ringkasan + return status string. */
static const char* mig_status(cJSON* res) {
    if (!res) return "NETWORK_ERROR";
    cJSON* status = cJSON_GetObjectItem(res, "status");
    if (status && cJSON_IsString(status)) return status->valuestring;
    return "UNKNOWN";
}

static int mig_is_success(cJSON* res) {
    const char* s = mig_status(res);
    return (strcmp(s, "SUCCESS") == 0);
}

/* Konfirmasi 2x dengan typed string */
static int mig_confirm_typed(const char* required) {
    char b[256];
    printf("\n[!] PERINGATAN: aksi ini bersifat PERMANENT.\n");
    printf("    Ketik persis '%s' untuk konfirmasi (atau apa saja untuk batal): ", required);
    fflush(stdout);
    mig_read_line("", b, sizeof(b));
    return strcmp(b, required) == 0;
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 1: Cek Status Subscription
 * ------------------------------------------------------------------------- */

static void mig_check_status(const char* base, const char* key, const char* xdata,
                             const char* sec, const char* id_token,
                             const char* my_msisdn) {
    mig_clear();
    printf("=======================================================\n");
    printf("       1. CEK STATUS SUBSCRIPTION\n");
    printf("=======================================================\n");
    printf("MSISDN aktif    : %s\n", my_msisdn ? my_msisdn : "(unknown)");
    printf("Subscription    : %s\n", active_subs_type[0] ? active_subs_type : "UNKNOWN");
    printf("\n[*] Memanggil /api/v8/profile untuk verifikasi server-side...\n");

    cJSON* prof = get_profile(base, key, xdata, sec, id_token, NULL);
    if (prof) {
        cJSON* data = cJSON_GetObjectItem(prof, "data");
        if (data) {
            printf("  - subscription_type : %s\n", json_get_str(data, "subscription_type", "?"));
            printf("  - profile.status    : %s\n", json_get_str(data, "status", "?"));
            cJSON* contact = cJSON_GetObjectItem(data, "contact");
            if (contact) {
                printf("  - email             : %s\n", json_get_str(contact, "email", "(belum set)"));
            }
        } else {
            mig_dump("RAW PROFILE", prof);
        }
    }
    cJSON_Delete(prof);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 2: Cek Eligibility PRIOHYBRID
 * ------------------------------------------------------------------------- */

static void mig_check_priohybrid_eligibility(const char* base, const char* key,
                                             const char* xdata, const char* sec,
                                             const char* id_token) {
    mig_clear();
    printf("=======================================================\n");
    printf("       2. CEK ELIGIBILITY PRIOHYBRID\n");
    printf("=======================================================\n");
    printf("[*] POST /sharings/api/v8/packages/check-prio-hybrid ...\n");
    cJSON* res = mig_check_prio_hybrid(base, key, xdata, sec, id_token);
    printf("\nStatus: %s\n", mig_status(res));
    mig_dump("RESPONSE", res);
    cJSON_Delete(res);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 3: Migrasi PREPAID -> PRIORITAS (full flow)
 * ------------------------------------------------------------------------- */

static void mig_prepaid_to_prio(const char* base, const char* key,
                                const char* xdata, const char* sec,
                                const char* id_token, const char* my_msisdn) {
    mig_clear();
    printf("=======================================================\n");
    printf("       3. MIGRASI PREPAID -> PRIORITAS (full flow)\n");
    printf("=======================================================\n");
    printf("Flow lengkap (4 step):\n");
    printf("  a. check-dukcapil-registration (cek apakah NIK terdaftar)\n");
    printf("  b. create-dukcapil (buat record kalau belum)\n");
    printf("  c. prio/registration (register MSISDN ke PRIO)\n");
    printf("  d. prio/activation (aktivasi final)\n");
    printf("-------------------------------------------------------\n");

    if (active_subs_type[0] && strcmp(active_subs_type, "PREPAID") != 0) {
        printf("[!] Akun ini bukan PREPAID (%s). Endpoint ini DIPANGGIL APP MyXL\n",
               active_subs_type);
        printf("    saat user PREPAID daftar PRIO. Tetap lanjut?\n");
        if (!mig_read_yn("Lanjut anyway? (y/N): ")) return;
    }

    char nik[32], family_name[128], birthdate[16], iccid_last[8], reg_no[64];
    mig_read_line("NIK (16 digit, 99=batal): ", nik, sizeof(nik));
    if (strcmp(nik, "99") == 0 || strlen(nik) == 0) return;

    mig_read_line("Nama Keluarga (sesuai KTP): ", family_name, sizeof(family_name));
    mig_read_line("Tanggal Lahir (YYYY-MM-DD): ", birthdate, sizeof(birthdate));
    mig_read_line("4 Digit ICCID Terakhir: ", iccid_last, sizeof(iccid_last));
    mig_read_line("Nomor Registrasi (kode dari pack box, opsional): ",
                  reg_no, sizeof(reg_no));

    if (!mig_confirm_typed("DAFTAR PRIO PERMANENT")) {
        printf("\n[*] Dibatalkan.\n");
        mig_pause();
        return;
    }

    printf("\n[*] Step a: check-dukcapil-registration ...\n");
    cJSON* r = mig_prio_check_dukcapil_registration(base, key, xdata, sec,
                                                    id_token, nik, family_name, birthdate);
    printf("Status: %s\n", mig_status(r));
    mig_dump("Step a", r);
    int dukcapil_ok = mig_is_success(r);
    cJSON_Delete(r);

    if (!dukcapil_ok) {
        printf("\n[*] Step b: create-dukcapil ...\n");
        r = mig_prio_create_dukcapil(base, key, xdata, sec, id_token,
                                     nik, family_name, birthdate, my_msisdn);
        printf("Status: %s\n", mig_status(r));
        mig_dump("Step b", r);
        cJSON_Delete(r);
    }

    printf("\n[*] Step c: prio/registration ...\n");
    r = mig_prio_registration(base, key, xdata, sec, id_token,
                              my_msisdn, iccid_last, reg_no);
    printf("Status: %s\n", mig_status(r));
    mig_dump("Step c", r);
    int reg_ok = mig_is_success(r);
    cJSON_Delete(r);

    if (!reg_ok) {
        printf("\n[-] Registration gagal — tidak melanjutkan ke activation.\n");
        mig_pause();
        return;
    }

    printf("\n[*] Step d: prio/activation ...\n");
    r = mig_prio_activation(base, key, xdata, sec, id_token,
                            my_msisdn, iccid_last, reg_no);
    printf("Status: %s\n", mig_status(r));
    mig_dump("Step d", r);
    cJSON_Delete(r);

    printf("\n[*] Selesai. Cek /profile untuk verifikasi subscription_type.\n");
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 4: Migrasi PREPAID -> PRIOHYBRID (KYC upload)
 * ------------------------------------------------------------------------- */

static void mig_prepaid_to_priohybrid(const char* base, const char* key,
                                      const char* xdata, const char* sec,
                                      const char* id_token) {
    mig_clear();
    printf("=======================================================\n");
    printf("       4. MIGRASI PREPAID -> PRIOHYBRID (KYC upload)\n");
    printf("=======================================================\n");
    printf("Endpoint: POST /infos/api/v8/pre-to-prioh/update-migration-data\n");
    printf("\n[!] Fitur ini butuh upload base64 KTP + selfie.\n");
    printf("    Disarankan: prepare image KTP & selfie sebagai base64 string\n");
    printf("    via tools eksternal (mis. `base64 -w 0 ktp.jpg`), lalu paste.\n");
    printf("\n");

    if (!mig_read_yn("Lanjut input data KYC sekarang? (y/N): ")) return;

    char name[128], email[128], birthdate[16], nik[32];
    char address[256], birthplace[64];
    char img_path[256], selfie_path[256];

    mig_read_line("Nama Lengkap: ", name, sizeof(name));
    mig_read_line("Email: ", email, sizeof(email));
    mig_read_line("Tanggal Lahir (YYYY-MM-DD): ", birthdate, sizeof(birthdate));
    mig_read_line("NIK KTP (16 digit): ", nik, sizeof(nik));
    mig_read_line("Tempat Lahir: ", birthplace, sizeof(birthplace));
    mig_read_line("Alamat lengkap: ", address, sizeof(address));
    mig_read_line("Path file KTP (.jpg, untuk base64-encode): ",
                  img_path, sizeof(img_path));
    mig_read_line("Path file Selfie (.jpg): ", selfie_path, sizeof(selfie_path));

    /* TODO: read file + base64 encode */
    printf("\n[!] Upload base64 belum diimplementasi di fitur ini.\n");
    printf("    Saat ini hanya menyimpan placeholder kosong untuk testing endpoint.\n");
    printf("    File yang akan di-upload: %s + %s\n", img_path, selfie_path);

    if (!mig_confirm_typed("MIGRATE KE PRIOHYBRID")) {
        printf("\n[*] Dibatalkan.\n");
        mig_pause();
        return;
    }

    cJSON* r = mig_pre_to_prioh_submit(base, key, xdata, sec, id_token,
                                       name, email, birthdate, nik,
                                       "" /* card_image */,
                                       "" /* selfie */,
                                       address, birthplace, 0);
    printf("Status: %s\n", mig_status(r));
    mig_dump("RESPONSE", r);
    cJSON_Delete(r);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 5: ⭐ PRIOHYBRID -> PRIORITAS upgrade (1 step!)
 * ------------------------------------------------------------------------- */

static void mig_priohybrid_to_prioritas(const char* base, const char* key,
                                        const char* xdata, const char* sec,
                                        const char* id_token,
                                        const char* my_msisdn) {
    mig_clear();
    printf("=======================================================\n");
    printf("    5. UPGRADE PRIOHYBRID -> PRIORITAS (1 step!)\n");
    printf("=======================================================\n");
    printf("Endpoint: POST /auths/api/v8/prio/post-hybridactivation\n");
    printf("Payload : { \"msisdn\": \"%s\", \"lang\": \"en\" }\n",
           my_msisdn ? my_msisdn : "?");
    printf("\n");
    printf("[i] V2 endpoint — cuma butuh msisdn karena PRIOHYBRID sudah punya\n");
    printf("    dukcapil + KTP terdaftar di server.\n");
    printf("\n[!] DAMPAK:\n");
    printf("    - subscription_type: PRIOHYBRID -> PRIORITAS (PERMANENT)\n");
    printf("    - Setelah upgrade, akun jadi POSTPAID-mode tanpa billing dulu\n");
    printf("    - Bisa lanjut ke aktivasi MPD 6/12/24m (sub-menu 7) untuk\n");
    printf("      mendapatkan validity 2099 (auto-renewal)\n");
    printf("    - Revert hanya bisa via call CS XL\n");
    printf("\n");

    if (active_subs_type[0] && strcmp(active_subs_type, "PRIOHYBRID") != 0) {
        printf("[!] Akun ini bukan PRIOHYBRID (current: %s).\n", active_subs_type);
        printf("    Endpoint mungkin reject atau tidak ada efek.\n");
        if (!mig_read_yn("Tetap test endpoint? (y/N): ")) return;
    }

    if (!mig_confirm_typed("UPGRADE KE PRIORITAS")) {
        printf("\n[*] Dibatalkan.\n");
        mig_pause();
        return;
    }

    /* Konfirmasi kedua */
    printf("\n[!] KONFIRMASI AKHIR\n");
    printf("    MSISDN target: %s\n", my_msisdn ? my_msisdn : "?");
    if (!mig_read_yn("Eksekusi upgrade SEKARANG? (y/N): ")) {
        printf("[*] Dibatalkan.\n");
        mig_pause();
        return;
    }

    printf("\n[*] Memanggil prio/post-hybridactivation ...\n");
    cJSON* r = mig_prio_post_hybridactivation(base, key, xdata, sec,
                                              id_token, my_msisdn);
    printf("\nStatus: %s\n", mig_status(r));
    mig_dump("RESPONSE", r);

    if (mig_is_success(r)) {
        printf("\n[+] BERHASIL! Cek /profile untuk verifikasi.\n");
        printf("    Tunggu 1-5 menit, lalu logout & login ulang untuk\n");
        printf("    refresh subscription_type di token baru.\n");
    }
    cJSON_Delete(r);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 6: Browse MPD Bundle 6/12/24 Month
 * ------------------------------------------------------------------------- */

static void mig_browse_mpd_offers(const char* base, const char* key,
                                  const char* xdata, const char* sec,
                                  const char* id_token) {
    mig_clear();
    printf("=======================================================\n");
    printf("    6. BROWSE MPD BUNDLE 6/12/24 MONTH\n");
    printf("=======================================================\n");
    printf("[*] POST /store/api/v8/pre-to-postpaid/offers ...\n");
    cJSON* r = mig_pre_to_post_offers(base, key, xdata, sec, id_token);
    printf("\nStatus: %s\n", mig_status(r));

    if (mig_is_success(r)) {
        cJSON* data = cJSON_GetObjectItem(r, "data");
        cJSON* offers = data ? cJSON_GetObjectItem(data, "offers") : NULL;
        if (!offers) offers = data;
        if (offers && cJSON_IsArray(offers)) {
            int n = cJSON_GetArraySize(offers);
            printf("\nTotal offers: %d\n\n", n);
            for (int i = 0; i < n; i++) {
                cJSON* o = cJSON_GetArrayItem(offers, i);
                printf("%2d. %s\n", i + 1, json_get_str(o, "name", "?"));
                printf("    offer_id     : %s\n", json_get_str(o, "offer_id", json_get_str(o, "id", "?")));
                printf("    monthly_fee  : %d\n", json_get_int(o, "monthly_fee",
                                                                json_get_int(o, "price", 0)));
                printf("    duration     : %d month\n", json_get_int(o, "validity_months",
                                                                    json_get_int(o, "duration", 0)));
                printf("    quota        : %s\n", json_get_str(o, "quota", "?"));
                printf("\n");
            }
        } else {
            mig_dump("RAW", r);
        }
    } else {
        mig_dump("RESPONSE", r);
    }
    cJSON_Delete(r);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 7: ⭐ AKTIVASI KONTRAK MPD (validity 2099)
 * ------------------------------------------------------------------------- */

static void mig_activate_mpd_contract(const char* base, const char* key,
                                      const char* xdata, const char* sec,
                                      const char* id_token,
                                      const char* my_msisdn) {
    mig_clear();
    printf("=======================================================\n");
    printf("    7. AKTIVASI KONTRAK MPD (validity 2099)\n");
    printf("=======================================================\n");
    printf("[!] PERHATIAN: aksi ini menciptakan COMMITMENT BILLING\n");
    printf("    bulanan ke akun XL Anda. Pastikan Anda sudah:\n");
    printf("    - Browse offer dulu (sub-menu 6) untuk dapat offer_id\n");
    printf("    - Cek estimasi billing (sub-menu 12)\n");
    printf("    - Pastikan akun PRIOHYBRID atau PRIORITAS\n");
    printf("\n");

    char offer_id[128];
    mig_read_line("Offer ID (dari sub-menu 6, 99=batal): ", offer_id, sizeof(offer_id));
    if (strcmp(offer_id, "99") == 0 || strlen(offer_id) == 0) return;

    /* Step 1: check eligibility */
    printf("\n[*] Step 1/3: check-eligibility offer_id=%s ...\n", offer_id);
    cJSON* r = mig_pre_to_post_check_eligibility(base, key, xdata, sec,
                                                 id_token, offer_id);
    printf("Status: %s\n", mig_status(r));
    mig_dump("Step 1", r);
    int eligible = mig_is_success(r);
    cJSON_Delete(r);

    if (!eligible) {
        printf("\n[-] Akun tidak eligible untuk offer ini.\n");
        mig_pause();
        return;
    }

    /* Step 2: estimasi billing */
    printf("\n[*] Step 2/3: estimasi billing ...\n");
    r = mig_billing_estimation(base, key, xdata, sec, id_token, offer_id, my_msisdn);
    printf("Status: %s\n", mig_status(r));
    mig_dump("Step 2", r);
    cJSON_Delete(r);

    /* Konfirmasi 3x */
    printf("\n[!] KONFIRMASI 1/3: Anda akan mengaktivasi kontrak %s\n", offer_id);
    if (!mig_read_yn("    Lanjut? (y/N): ")) { printf("Dibatalkan.\n"); mig_pause(); return; }

    if (!mig_confirm_typed("AKTIVASI KONTRAK MPD")) {
        printf("\n[*] Dibatalkan (typo).\n");
        mig_pause();
        return;
    }

    printf("\n[!] KONFIRMASI 3/3 (FINAL):\n");
    printf("    Offer  : %s\n", offer_id);
    printf("    MSISDN : %s\n", my_msisdn ? my_msisdn : "?");
    printf("    Setelah submit, billing akan auto-debit setiap bulan.\n");
    if (!mig_read_yn("    EKSEKUSI SEKARANG? (y/N): ")) {
        printf("Dibatalkan.\n");
        mig_pause();
        return;
    }

    /* Step 3: submit */
    printf("\n[*] Step 3/3: pre-to-postpaid/submit ...\n");
    r = mig_pre_to_post_submit(base, key, xdata, sec, id_token, offer_id, my_msisdn);
    printf("\nStatus: %s\n", mig_status(r));
    mig_dump("RESPONSE", r);

    if (mig_is_success(r)) {
        printf("\n[+] AKTIVASI BERHASIL!\n");
        printf("    - Cek /balance-and-credit untuk lihat expired_at = 2099-12-31\n");
        printf("    - Auto-renewal aktif sampai Anda cancel via CS XL\n");
    }
    cJSON_Delete(r);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 8: Cek Recontract Eligibility
 * ------------------------------------------------------------------------- */

static void mig_check_recontract(const char* base, const char* key,
                                 const char* xdata, const char* sec,
                                 const char* id_token) {
    mig_clear();
    printf("=======================================================\n");
    printf("    8. CEK RECONTRACT ELIGIBILITY\n");
    printf("=======================================================\n");
    printf("[*] POST /refinancing/api/v8/recontract/check-eligibility ...\n");
    cJSON* r = mig_recontract_check_eligibility(base, key, xdata, sec, id_token);
    printf("\nStatus: %s\n", mig_status(r));
    mig_dump("RESPONSE", r);
    cJSON_Delete(r);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 10: Trial PRIO 7-Hari (revertible)
 * ------------------------------------------------------------------------- */

static void mig_trial_prio_7day(const char* base, const char* key,
                                const char* xdata, const char* sec,
                                const char* id_token) {
    mig_clear();
    printf("=======================================================\n");
    printf("    10. TRIAL PRIO 7-HARI (revertible)\n");
    printf("=======================================================\n");
    printf("Endpoint: POST /registinfo/api/v8/brand-portability/trial\n");
    printf("\n[i] Trial PRIO selama 7 hari. Bisa di-revert via sub-menu 11.\n");
    printf("    Setelah 7 hari otomatis kembali ke PREPAID kalau tidak\n");
    printf("    di-konversi ke PRIO permanent.\n");
    printf("\n");

    /* Lihat info dulu */
    printf("[*] POST trial/info ...\n");
    cJSON* info = mig_bp_trial_info(base, key, xdata, sec, id_token);
    printf("Status: %s\n", mig_status(info));
    mig_dump("INFO", info);
    cJSON_Delete(info);

    if (!mig_read_yn("\nLanjut start trial sekarang? (y/N): ")) return;

    if (!mig_confirm_typed("TRIAL PRIO 7 HARI")) {
        printf("\n[*] Dibatalkan.\n");
        mig_pause();
        return;
    }

    printf("\n[*] POST trial start ...\n");
    cJSON* r = mig_bp_trial_start(base, key, xdata, sec, id_token);
    printf("Status: %s\n", mig_status(r));
    mig_dump("RESPONSE", r);
    cJSON_Delete(r);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 11: Revert Trial -> PREPAID
 * ------------------------------------------------------------------------- */

static void mig_revert_trial(const char* base, const char* key,
                             const char* xdata, const char* sec,
                             const char* id_token) {
    mig_clear();
    printf("=======================================================\n");
    printf("    11. REVERT TRIAL -> PREPAID\n");
    printf("=======================================================\n");
    printf("Endpoint: POST /registinfo/api/v8/brand-portability/trial/revert\n");
    printf("\n");

    if (!mig_read_yn("Yakin revert trial sekarang? (y/N): ")) return;

    printf("\n[*] POST trial/revert ...\n");
    cJSON* r = mig_bp_trial_revert(base, key, xdata, sec, id_token);
    printf("Status: %s\n", mig_status(r));
    mig_dump("RESPONSE", r);
    cJSON_Delete(r);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 12: Estimasi Billing Migrasi
 * ------------------------------------------------------------------------- */

static void mig_billing_estimate(const char* base, const char* key,
                                 const char* xdata, const char* sec,
                                 const char* id_token,
                                 const char* my_msisdn) {
    mig_clear();
    printf("=======================================================\n");
    printf("    12. ESTIMASI BILLING MIGRASI\n");
    printf("=======================================================\n");
    char offer_id[128];
    mig_read_line("Offer ID (kosongkan untuk default, 99=batal): ",
                  offer_id, sizeof(offer_id));
    if (strcmp(offer_id, "99") == 0) return;

    printf("\n[*] POST /sharings/api/v8/migration/billing-estimation-charge ...\n");
    cJSON* r = mig_billing_estimation(base, key, xdata, sec, id_token,
                                      offer_id, my_msisdn);
    printf("\nStatus: %s\n", mig_status(r));
    mig_dump("RESPONSE", r);
    cJSON_Delete(r);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 13: Co-brand Eligibility
 * ------------------------------------------------------------------------- */

static void mig_check_cobrand(const char* base, const char* key,
                              const char* xdata, const char* sec,
                              const char* id_token) {
    mig_clear();
    printf("=======================================================\n");
    printf("    13. CEK CO-BRAND (LinkAja, OVO, etc)\n");
    printf("=======================================================\n");
    printf("[*] POST /sharings/api/v8/co-brand/check ...\n");
    cJSON* r = mig_cobrand_check(base, key, xdata, sec, id_token);
    printf("Status: %s\n", mig_status(r));
    mig_dump("CHECK", r);
    cJSON_Delete(r);

    printf("\n[*] POST /sharings/api/v8/co-brand/check-eligibility ...\n");
    r = mig_cobrand_check_eligibility(base, key, xdata, sec, id_token);
    printf("Status: %s\n", mig_status(r));
    mig_dump("ELIGIBILITY", r);
    cJSON_Delete(r);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Sub-fitur 14: Status Migration PRIOHYBRID
 * ------------------------------------------------------------------------- */

static void mig_priohybrid_status(const char* base, const char* key,
                                  const char* xdata, const char* sec,
                                  const char* id_token) {
    mig_clear();
    printf("=======================================================\n");
    printf("    14. STATUS MIGRASI PRIOHYBRID\n");
    printf("=======================================================\n");
    printf("[*] POST /infos/api/v8/pre-to-prioh/migration-status ...\n");
    cJSON* r = mig_pre_to_prioh_status(base, key, xdata, sec, id_token);
    printf("\nStatus: %s\n", mig_status(r));
    mig_dump("RESPONSE", r);
    cJSON_Delete(r);
    mig_pause();
}

/* ---------------------------------------------------------------------------
 * Main migration menu
 * ------------------------------------------------------------------------- */

void migration_menu(const char* base_api, const char* api_key,
                    const char* xdata_key, const char* x_api_secret,
                    const char* id_token, const char* my_msisdn) {
    if (!id_token || !id_token[0]) {
        printf("[-] Belum login.\n");
        return;
    }

    while (1) {
        mig_clear();
        printf("=======================================================\n");
        printf("       MIGRASI & KONTRAK SUBSCRIPTION\n");
        printf("=======================================================\n");
        printf(" Akun aktif: %s | Tipe: %s\n",
               my_msisdn ? my_msisdn : "?",
               active_subs_type[0] ? active_subs_type : "?");
        printf("-------------------------------------------------------\n");
        printf("  1. Cek Status Subscription                  [READ]\n");
        printf("  2. Cek Eligibility PRIOHYBRID               [READ]\n");
        printf("  3. Migrasi PREPAID -> PRIORITAS             [WRITE]\n");
        printf("  4. Migrasi PREPAID -> PRIOHYBRID (KYC)      [WRITE]\n");
        printf("  5. UPGRADE PRIOHYBRID -> PRIORITAS *        [WRITE]\n");
        printf("  6. Browse MPD 6/12/24 Month Bundle          [READ]\n");
        printf("  7. AKTIVASI KONTRAK MPD (validity 2099) **  [BILLING]\n");
        printf("  8. Cek Recontract Eligibility               [READ]\n");
        printf(" 10. Trial PRIO 7-Hari (revertible)           [WRITE]\n");
        printf(" 11. Revert Trial -> PREPAID                  [WRITE]\n");
        printf(" 12. Estimasi Billing Migrasi                 [READ]\n");
        printf(" 13. Cek Co-brand (LinkAja/OVO)               [READ]\n");
        printf(" 14. Status Migration PRIOHYBRID              [READ]\n");
        printf("-------------------------------------------------------\n");
        printf(" READ    = read-only, aman dipanggil\n");
        printf(" WRITE   = ubah subscription_type permanent\n");
        printf(" BILLING = ciptakan auto-debit billing bulanan\n");
        printf("-------------------------------------------------------\n");
        printf(" 00. Kembali        99. Menu utama\n");
        printf("Pilihan: ");
        fflush(stdout);

        char ch[16]; if (!fgets(ch, sizeof(ch), stdin)) return;
        ch[strcspn(ch, "\n")] = 0;

        if (strcmp(ch, "00") == 0) return;
        else if (strcmp(ch, "99") == 0) { nav_trigger_goto_main(); return; }
        else if (strcmp(ch, "1") == 0)
            mig_check_status(base_api, api_key, xdata_key, x_api_secret,
                             id_token, my_msisdn);
        else if (strcmp(ch, "2") == 0)
            mig_check_priohybrid_eligibility(base_api, api_key, xdata_key,
                                             x_api_secret, id_token);
        else if (strcmp(ch, "3") == 0)
            mig_prepaid_to_prio(base_api, api_key, xdata_key, x_api_secret,
                                id_token, my_msisdn);
        else if (strcmp(ch, "4") == 0)
            mig_prepaid_to_priohybrid(base_api, api_key, xdata_key, x_api_secret,
                                      id_token);
        else if (strcmp(ch, "5") == 0)
            mig_priohybrid_to_prioritas(base_api, api_key, xdata_key, x_api_secret,
                                        id_token, my_msisdn);
        else if (strcmp(ch, "6") == 0)
            mig_browse_mpd_offers(base_api, api_key, xdata_key, x_api_secret,
                                  id_token);
        else if (strcmp(ch, "7") == 0)
            mig_activate_mpd_contract(base_api, api_key, xdata_key, x_api_secret,
                                      id_token, my_msisdn);
        else if (strcmp(ch, "8") == 0)
            mig_check_recontract(base_api, api_key, xdata_key, x_api_secret,
                                 id_token);
        else if (strcmp(ch, "10") == 0)
            mig_trial_prio_7day(base_api, api_key, xdata_key, x_api_secret,
                                id_token);
        else if (strcmp(ch, "11") == 0)
            mig_revert_trial(base_api, api_key, xdata_key, x_api_secret, id_token);
        else if (strcmp(ch, "12") == 0)
            mig_billing_estimate(base_api, api_key, xdata_key, x_api_secret,
                                 id_token, my_msisdn);
        else if (strcmp(ch, "13") == 0)
            mig_check_cobrand(base_api, api_key, xdata_key, x_api_secret, id_token);
        else if (strcmp(ch, "14") == 0)
            mig_priohybrid_status(base_api, api_key, xdata_key, x_api_secret,
                                  id_token);

        if (nav_should_return()) return;
    }
}

/* ===========================================================================
 * VERIFY ACTIVE PACKAGE (legit vs decoy check)
 *
 * Cek setiap paket aktif di /balance-and-credit, lalu cross-check via
 * /options/list dengan family_code-nya. Kalau server kembalikan SUCCESS
 * dengan name + variants > 0 -> LEGIT. Kalau gagal -> DECOY (cosmetic only).
 * =========================================================================== */

static void verify_one_package(const char* base, const char* key,
                               const char* xdata, const char* sec,
                               const char* id_token,
                               const char* family_code, const char* package_name) {
    if (!family_code || !family_code[0]) {
        printf("    [DECOY?] Tidak ada family_code di response server.\n");
        return;
    }

    /* Coba 7 migration_type x 2 enterprise sampai dapat SUCCESS dengan variants */
    const char* migrations[] = { "NORMAL", "NONE", "PRE_TO_PRIO", "PRIO_TO_PRE",
                                 "PRE_TO_PRIOH", "PRIOH_TO_PRIO", "PRIO_TO_PRIOH" };
    int legit = 0;
    int variants = 0;
    char fam_name_server[128] = "";
    for (size_t i = 0; i < sizeof(migrations)/sizeof(migrations[0]) && !legit; i++) {
        for (int ent = 0; ent <= 1 && !legit; ent++) {
            cJSON* r = get_family(base, key, xdata, sec, id_token,
                                  family_code, ent, migrations[i]);
            if (!r) continue;
            if (mig_is_success(r)) {
                cJSON* data = cJSON_GetObjectItem(r, "data");
                cJSON* pfam = data ? cJSON_GetObjectItem(data, "package_family") : NULL;
                cJSON* pvar = data ? cJSON_GetObjectItem(data, "package_variants") : NULL;
                const char* nm = pfam ? json_get_str(pfam, "name", "") : "";
                int nv = pvar && cJSON_IsArray(pvar) ? cJSON_GetArraySize(pvar) : 0;
                if (nm[0] && nv > 0) {
                    legit = 1;
                    variants = nv;
                    snprintf(fam_name_server, sizeof(fam_name_server), "%s", nm);
                }
            }
            cJSON_Delete(r);
        }
    }

    if (legit) {
        printf("    [LEGIT]  family_code resolve di catalog server\n");
        printf("             server name: %s | variants: %d\n",
               fam_name_server, variants);
        if (package_name && fam_name_server[0]
            && strcasecmp(package_name, fam_name_server) != 0) {
            printf("             [!] mismatch: nama lokal '%s' vs server '%s'\n",
                   package_name, fam_name_server);
        }
    } else {
        printf("    [DECOY]  family_code TIDAK resolve di catalog server.\n");
        printf("             Paket ini kemungkinan COSMETIC ONLY (UI override),\n");
        printf("             tidak ada di database catalog XL.\n");
    }
}

void verify_active_package_menu(const char* base_api, const char* api_key,
                                const char* xdata_key, const char* x_api_secret,
                                const char* id_token) {
    if (!id_token || !id_token[0]) {
        printf("[-] Belum login.\n"); return;
    }

    mig_clear();
    printf("=======================================================\n");
    printf("       VERIFY ACTIVE PACKAGE (legit vs decoy)\n");
    printf("=======================================================\n");
    printf("[*] Mengambil daftar paket aktif via /balance-and-credit ...\n\n");

    cJSON* bc = get_balance(base_api, api_key, xdata_key, x_api_secret, id_token);
    if (!bc) {
        printf("[-] Gagal ambil balance-and-credit.\n");
        mig_pause();
        return;
    }

    cJSON* data = cJSON_GetObjectItem(bc, "data");
    cJSON* quotas = data ? cJSON_GetObjectItem(data, "quotas") : NULL;
    if (!quotas) quotas = data ? cJSON_GetObjectItem(data, "packages") : NULL;

    if (!quotas || !cJSON_IsArray(quotas)) {
        printf("[-] Tidak ada array paket aktif di response.\n");
        mig_dump("RAW response", bc);
        cJSON_Delete(bc);
        mig_pause();
        return;
    }

    int n = cJSON_GetArraySize(quotas);
    if (n == 0) {
        printf("[i] Belum ada paket aktif.\n");
        cJSON_Delete(bc);
        mig_pause();
        return;
    }

    printf("Total paket aktif: %d\n", n);
    printf("-------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        cJSON* q = cJSON_GetArrayItem(quotas, i);
        const char* name = json_get_str(q, "name",
                              json_get_str(q, "package_name", "(unnamed)"));
        const char* fam = json_get_str(q, "family_code",
                              json_get_str(q, "package_family_code", ""));
        const char* opt = json_get_str(q, "package_option_code",
                              json_get_str(q, "option_code", ""));
        const char* exp_at = json_get_str(q, "expired_at",
                                json_get_str(q, "expire_date", "?"));

        printf("\n%d. %s\n", i + 1, name);
        printf("    family_code  : %s\n", fam[0] ? fam : "(none)");
        printf("    option_code  : %s\n", opt[0] ? opt : "(none)");
        printf("    expired_at   : %s\n", exp_at);
        if (strstr(exp_at, "2099") || strstr(exp_at, "2100")) {
            printf("                   ^^ marker auto-renewal/lifetime\n");
        }

        verify_one_package(base_api, api_key, xdata_key, x_api_secret,
                           id_token, fam, name);
    }

    printf("\n-------------------------------------------------------\n");
    printf("Legenda:\n");
    printf("  LEGIT  = family_code resolve di catalog server (paket asli)\n");
    printf("  DECOY  = family_code TIDAK resolve (cosmetic only / fake)\n");
    printf("-------------------------------------------------------\n");

    cJSON_Delete(bc);
    mig_pause();
}

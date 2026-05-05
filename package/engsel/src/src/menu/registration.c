/*
 * Menu Registrasi Kartu (Menu R baru, versi wizard).
 * Lihat include/menu/registration.h untuk arsitektur.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/cJSON.h"
#include "../include/menu/registration.h"
#include "../include/client/registration.h"
#include "../include/util/json_util.h"
#include "../include/util/phone.h"

#define W 60

/* ============== UI helpers ============== */

static void rule(char ch) { for (int i = 0; i < W; i++) putchar(ch); putchar('\n'); }

static void clear_screen(void) { printf("\033[H\033[J"); fflush(stdout); }

static void section_header(const char* title) {
    clear_screen();
    rule('=');
    printf("  %s\n", title);
    rule('=');
}

static void flush_line(void) { int c; while ((c = getchar()) != '\n' && c != EOF); }

static void pause_enter(void) {
    printf("\nTekan Enter untuk melanjutkan...");
    fflush(stdout);
    flush_line();
}

static void read_line(const char* prompt, char* buf, size_t n) {
    printf("%s", prompt); fflush(stdout);
    if (!fgets(buf, (int)n, stdin)) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\n")] = '\0';
}

static int read_msisdn(const char* prompt, char* buf, size_t n) {
    read_line(prompt, buf, n);
    if (!buf[0]) return 0;
    if (strcmp(buf, "99") == 0) return 0;
    char* norm = normalize_msisdn(buf);
    if (norm) {
        snprintf(buf, n, "%s", norm);
        free(norm);
    }
    return buf[0] != '\0';
}

static int read_required(const char* prompt, char* buf, size_t n) {
    read_line(prompt, buf, n);
    if (!buf[0] || strcmp(buf, "99") == 0) return 0;
    return 1;
}

/* Validate digit-only fixed-length input (e.g. NIK = 16, KK = 16, PUK = 8). */
static int is_digits_len(const char* s, int min_len, int max_len) {
    if (!s || !s[0]) return 0;
    int n = 0;
    for (const char* p = s; *p; p++, n++) {
        if (*p < '0' || *p > '9') return 0;
    }
    return n >= min_len && n <= max_len;
}

/* ============== Response parsing helpers ============== */

static const char* response_error_code(cJSON* r) {
    if (!r) return NULL;
    cJSON* st = cJSON_GetObjectItemCaseSensitive(r, "status");
    if (cJSON_IsString(st) && st->valuestring &&
        strcmp(st->valuestring, "SUCCESS") == 0) {
        return NULL;
    }
    cJSON* c = cJSON_GetObjectItemCaseSensitive(r, "code");
    if (cJSON_IsString(c) && c->valuestring && c->valuestring[0]) {
        return c->valuestring;
    }
    /* Fallback ke field "error.code" atau "data.code". */
    cJSON* err = cJSON_GetObjectItemCaseSensitive(r, "error");
    if (err) {
        cJSON* ec = cJSON_GetObjectItemCaseSensitive(err, "code");
        if (cJSON_IsString(ec) && ec->valuestring && ec->valuestring[0]) return ec->valuestring;
    }
    cJSON* data = cJSON_GetObjectItemCaseSensitive(r, "data");
    if (data) {
        cJSON* dc = cJSON_GetObjectItemCaseSensitive(data, "code");
        if (cJSON_IsString(dc) && dc->valuestring && dc->valuestring[0]) return dc->valuestring;
    }
    return NULL;
}

static void print_response_summary(cJSON* r, const char* fail_label,
                                   const char* success_label) {
    rule('=');
    if (json_status_is_success(r)) {
        printf("  [+] %s\n", success_label ? success_label : "Operasi BERHASIL.");
    } else {
        const char* code = response_error_code(r);
        printf("  [-] %s\n", fail_label ? fail_label : "Operasi GAGAL.");
        if (code) {
            printf("      Kode error: %s\n", code);
            printf("      Pesan     : %s\n", registration_error_message(code));
        } else {
            printf("      Server tidak mengembalikan kode error.\n");
        }
    }
    rule('=');
}

static void print_raw(cJSON* r) {
    if (!r) {
        printf("\n[!] Server tidak mengembalikan response (network error?)\n");
        return;
    }
    char* out = cJSON_Print(r);
    if (out) {
        printf("\n--- Response (raw JSON) ---\n%s\n", out);
        free(out);
    }
}

/* Pretty print info status registrasi. Return: 1 kalau struct.data ada,
 * dan tulis nilai-nilai key ke output param (semua nullable). */
static int print_registration_info(cJSON* info_resp,
                                   const char** out_status,
                                   const char** out_name,
                                   const char** out_disp_nik,
                                   const char** out_disp_kk,
                                   int* out_eligible_biometric) {
    if (out_status)             *out_status = NULL;
    if (out_name)               *out_name = NULL;
    if (out_disp_nik)           *out_disp_nik = NULL;
    if (out_disp_kk)            *out_disp_kk = NULL;
    if (out_eligible_biometric) *out_eligible_biometric = 0;

    if (!json_status_is_success(info_resp)) return 0;
    cJSON* d = cJSON_GetObjectItemCaseSensitive(info_resp, "data");
    if (!d) return 0;

    const char* st = json_get_str(d, "registration_status", "");
    const char* nm = json_get_str(d, "name",                "");
    const char* dn = json_get_str(d, "display_nik",         "");
    const char* dk = json_get_str(d, "display_kk",          "");
    const char* dt = json_get_str(d, "registration_date",   "");
    cJSON* eb_node = cJSON_GetObjectItemCaseSensitive(d, "eligible_biometric");
    int eligible_bio = cJSON_IsTrue(eb_node);

    rule('-');
    printf("  Status         : %s\n", st[0] ? st : "UNKNOWN");
    if (nm[0]) printf("  Nama (server)  : %s\n", nm);
    if (dn[0]) printf("  NIK (mask)     : %s\n", dn);
    if (dk[0]) printf("  KK  (mask)     : %s\n", dk);
    if (dt[0]) printf("  Tanggal regist : %s\n", dt);
    printf("  Biometric only : %s\n", eligible_bio ? "YA (butuh App MyXL)" : "tidak");
    rule('-');

    if (out_status)             *out_status = st;
    if (out_name)               *out_name = nm;
    if (out_disp_nik)           *out_disp_nik = dn;
    if (out_disp_kk)            *out_disp_kk = dk;
    if (out_eligible_biometric) *out_eligible_biometric = eligible_bio;
    return 1;
}

/* ============== Sub-fitur: read-only ============== */

static void sub_get_info(const char* base, const char* api_key,
                         const char* xdata_key, const char* api_secret,
                         const char* id_token) {
    char msisdn[32];
    section_header("1. Cek Status Registrasi MSISDN");
    if (!read_msisdn("MSISDN (08/62xxxx) atau 99=batal: ", msisdn, sizeof(msisdn))) return;

    section_header("1. Cek Status Registrasi MSISDN");
    printf("MSISDN: %s\n", msisdn);
    printf("[*] Memeriksa status registrasi...\n"); fflush(stdout);

    cJSON* r = registration_get_info(base, api_key, xdata_key, api_secret,
                                     id_token, msisdn);
    if (!r) {
        printf("[!] Server tidak respons.\n");
    } else {
        if (json_status_is_success(r)) {
            print_registration_info(r, NULL, NULL, NULL, NULL, NULL);
        } else {
            print_response_summary(r, "Gagal cek status", NULL);
        }
        print_raw(r);
        cJSON_Delete(r);
    }
    pause_enter();
}

static void sub_check_dukcapil(const char* base, const char* api_key,
                               const char* xdata_key, const char* api_secret,
                               const char* id_token) {
    char msisdn[32], nik[32], kk[32];
    section_header("2. Validasi NIK + KK + MSISDN");
    if (!read_msisdn("MSISDN (08/62xxxx) atau 99=batal: ", msisdn, sizeof(msisdn))) return;
    if (!read_required("NIK (16 digit) atau 99=batal: ", nik, sizeof(nik))) return;
    if (!read_required("KK  (16 digit) atau 99=batal: ", kk,  sizeof(kk)))  return;

    if (!is_digits_len(nik, 16, 16)) {
        printf("[!] NIK harus 16 digit angka.\n");
        pause_enter(); return;
    }
    if (!is_digits_len(kk, 16, 16)) {
        printf("[!] KK harus 16 digit angka.\n");
        pause_enter(); return;
    }

    printf("\n[*] Mengirim check-dukcapil...\n"); fflush(stdout);
    cJSON* r = registration_check_dukcapil(base, api_key, xdata_key, api_secret,
                                           id_token, msisdn, nik, kk);
    print_response_summary(r, "Validasi GAGAL.", "Validasi BERHASIL — kombinasi NIK+KK+MSISDN valid.");
    print_raw(r);
    cJSON_Delete(r);
    pause_enter();
}

static void sub_validate_puk(const char* base, const char* api_key,
                             const char* xdata_key, const char* api_secret,
                             const char* id_token) {
    char msisdn[32], puk[32];
    section_header("3. Validasi PUK SIM (V9 -> V8 auto-fallback)");
    if (!read_msisdn("MSISDN (08/62xxxx) atau 99=batal: ", msisdn, sizeof(msisdn))) return;
    if (!read_required("PUK (8 digit, dari kemasan SIM): ", puk, sizeof(puk)))      return;

    if (!is_digits_len(puk, 8, 8)) {
        printf("[!] PUK biasanya 8 digit angka.\n");
        pause_enter(); return;
    }

    printf("\n[*] Mencoba V9 endpoint...\n"); fflush(stdout);
    cJSON* r = registration_validate_puk_v9(base, api_key, xdata_key, api_secret,
                                            id_token, msisdn, puk);
    int ok9 = json_status_is_success(r);
    if (!ok9) {
        const char* code9 = response_error_code(r);
        printf("    V9 gagal (kode: %s). Coba V8...\n", code9 ? code9 : "n/a");
        if (r) cJSON_Delete(r);
        r = registration_validate_puk_v8(base, api_key, xdata_key, api_secret,
                                         id_token, msisdn, puk);
    }
    print_response_summary(r, "Validasi PUK GAGAL.", "PUK VALID — bisa dipakai untuk autopair.");
    print_raw(r);
    cJSON_Delete(r);
    pause_enter();
}

static void sub_biometric_check(const char* base, const char* api_key,
                                const char* xdata_key, const char* api_secret,
                                const char* id_token) {
    char msisdn[32];
    section_header("4. Cek Eligibility Biometric");
    if (!read_msisdn("MSISDN (08/62xxxx) atau 99=batal: ", msisdn, sizeof(msisdn))) return;

    printf("\n[*] Memeriksa biometric attempt...\n"); fflush(stdout);
    cJSON* r = registration_biometric_attempt(base, api_key, xdata_key, api_secret,
                                              id_token, msisdn);
    print_response_summary(r, "Biometric check GAGAL.",
                           "Biometric check OK — lihat raw response.");
    print_raw(r);
    cJSON_Delete(r);

    /* Tambahan info: biometric flow tidak didukung CLI. */
    rule('-');
    printf("  [i] Engsel CLI TIDAK mendukung flow biometric (KTP+selfie+\n");
    printf("      liveness) karena butuh kamera + image upload. Gunakan\n");
    printf("      App MyXL official untuk flow biometric.\n");
    rule('-');
    pause_enter();
}

/* ============== Sub-fitur: write operations ============== */

static int confirm_write(const char* op) {
    printf("\n[!] Operasi ini akan MENGUBAH status registrasi MSISDN target.\n");
    printf("    Jenis operasi: %s\n", op);
    char ans[8];
    read_line("Lanjut submit? (ketik YA persis): ", ans, sizeof(ans));
    return strcmp(ans, "YA") == 0;
}

static void sub_register_simple(const char* base, const char* api_key,
                                const char* xdata_key, const char* api_secret,
                                const char* id_token) {
    char msisdn[32], nik[32], kk[32];
    section_header("5. Registrasi Standar (NIK + KK)");
    printf("Untuk SIM yang sudah pernah aktif tapi butuh re-pair NIK.\n\n");

    if (!read_msisdn("MSISDN: ", msisdn, sizeof(msisdn))) return;
    if (!read_required("NIK (16 digit): ", nik, sizeof(nik))) return;
    if (!read_required("KK  (16 digit): ", kk,  sizeof(kk)))  return;

    if (!is_digits_len(nik, 16, 16) || !is_digits_len(kk, 16, 16)) {
        printf("[!] NIK dan KK harus 16 digit.\n"); pause_enter(); return;
    }

    if (!confirm_write("Registrasi standar (regist/dukcapil)")) {
        printf("Dibatalkan.\n"); pause_enter(); return;
    }

    printf("\n[*] Submit registrasi standar...\n"); fflush(stdout);
    cJSON* r = registration_dukcapil_simple(base, api_key, xdata_key, api_secret,
                                            id_token, msisdn, nik, kk);
    print_response_summary(r, "Registrasi GAGAL.",
                           "Registrasi BERHASIL — MSISDN sekarang terdaftar.");
    print_raw(r);
    cJSON_Delete(r);
    pause_enter();
}

static void sub_register_with_puk(const char* base, const char* api_key,
                                  const char* xdata_key, const char* api_secret,
                                  const char* id_token,
                                  int autopair) {
    char msisdn[32], nik[32], kk[32], puk[32];
    if (autopair) {
        section_header("6. Registrasi SIM Baru (NIK + KK + PUK + autopair)");
        printf("Untuk SIM BARU yang belum pernah dipakai.\n\n");
    } else {
        section_header("7. Registrasi Manual + PUK (re-pair non-autopair)");
        printf("Untuk SIM yang sudah pernah pair tapi ganti owner/NIK.\n\n");
    }

    if (!read_msisdn("MSISDN: ", msisdn, sizeof(msisdn))) return;
    if (!read_required("NIK (16 digit): ", nik, sizeof(nik))) return;
    if (!read_required("KK  (16 digit): ", kk,  sizeof(kk)))  return;
    if (!read_required("PUK (8 digit, dari kemasan SIM): ", puk, sizeof(puk))) return;

    if (!is_digits_len(nik, 16, 16) || !is_digits_len(kk, 16, 16)) {
        printf("[!] NIK dan KK harus 16 digit.\n"); pause_enter(); return;
    }
    if (!is_digits_len(puk, 8, 8)) {
        printf("[!] PUK biasanya 8 digit.\n"); pause_enter(); return;
    }

    if (!confirm_write(autopair ? "Registrasi PUK + autopair (SIM baru)"
                                 : "Registrasi PUK + non-autopair (re-pair)")) {
        printf("Dibatalkan.\n"); pause_enter(); return;
    }

    printf("\n[*] Submit registrasi PUK%s...\n",
           autopair ? " + autopair" : " (non-autopair)");
    fflush(stdout);

    cJSON* r = autopair
        ? registration_dukcapil_autopair(base, api_key, xdata_key, api_secret,
                                         id_token, msisdn, nik, kk, puk)
        : registration_dukcapil_with_puk(base, api_key, xdata_key, api_secret,
                                         id_token, msisdn, nik, kk, puk);
    print_response_summary(r, "Registrasi GAGAL.",
                           "Registrasi BERHASIL — MSISDN sekarang terdaftar.");
    print_raw(r);
    cJSON_Delete(r);
    pause_enter();
}

static void sub_request_otp(const char* base, const char* api_key,
                            const char* xdata_key, const char* api_secret,
                            const char* id_token) {
    char msisdn[32];
    section_header("8. Request OTP Registrasi");
    if (!read_msisdn("MSISDN: ", msisdn, sizeof(msisdn))) return;

    printf("\n[*] Request OTP...\n"); fflush(stdout);
    cJSON* r = registration_request_otp(base, api_key, xdata_key, api_secret,
                                        id_token, msisdn);
    print_response_summary(r, "Request OTP GAGAL.",
                           "OTP terkirim — cek SMS di MSISDN target.");
    print_raw(r);
    cJSON_Delete(r);
    pause_enter();
}

static void sub_validate_otp(const char* base, const char* api_key,
                             const char* xdata_key, const char* api_secret,
                             const char* id_token) {
    char msisdn[32], otp[16];
    section_header("9. Validate OTP Registrasi");
    if (!read_msisdn("MSISDN: ", msisdn, sizeof(msisdn))) return;
    if (!read_required("OTP (6 digit): ", otp, sizeof(otp))) return;

    printf("\n[*] Validate OTP...\n"); fflush(stdout);
    cJSON* r = registration_validate_otp(base, api_key, xdata_key, api_secret,
                                         id_token, msisdn, otp);
    print_response_summary(r, "Validate OTP GAGAL.",
                           "OTP VALID — sesi registrasi terbuka.");
    print_raw(r);
    cJSON_Delete(r);
    pause_enter();
}

/* ============== Wizard pintar ============== */

/* Decision tree wizard:
 *   1. Cek status
 *   2. Kalau eligible_biometric only      → kasih warning ke App MyXL
 *   3. Kalau status REGISTERED            → tanya konfirmasi sebelum re-pair
 *   4. Kalau status NOT_REGISTERED        → tanya user: punya PUK atau tidak?
 *      a. Punya PUK                       → autopair flow
 *      b. Tidak punya PUK                 → simple dukcapil flow
 *      c. Kalau simple dukcapil fail 165  → fallback: tanya PUK lagi → autopair
 */
static void sub_wizard(const char* base, const char* api_key,
                       const char* xdata_key, const char* api_secret,
                       const char* id_token) {
    char msisdn[32];
    section_header("W. Wizard Pintar Registrasi");
    printf("Wizard akan auto-detect flow optimal berdasarkan status MSISDN.\n\n");

    if (!read_msisdn("MSISDN target (08/62xxxx): ", msisdn, sizeof(msisdn))) return;

    printf("\n[1/4] Cek status registrasi...\n"); fflush(stdout);
    cJSON* info = registration_get_info(base, api_key, xdata_key, api_secret,
                                        id_token, msisdn);

    const char *status = NULL, *name = NULL, *dnik = NULL, *dkk = NULL;
    int eligible_bio = 0;
    int has_info = print_registration_info(info, &status, &name, &dnik, &dkk,
                                           &eligible_bio);

    if (!has_info) {
        printf("[!] Tidak dapat cek status. Coba mode manual (1-9).\n");
        if (info) {
            const char* code = response_error_code(info);
            if (code) printf("    Kode: %s — %s\n", code,
                             registration_error_message(code));
            cJSON_Delete(info);
        }
        pause_enter();
        return;
    }

    if (eligible_bio) {
        printf("\n[!] MSISDN ini eligible untuk biometric registration ONLY.\n");
        printf("    Engsel CLI tidak bisa lakukan biometric (KTP+selfie+\n");
        printf("    liveness). Silakan gunakan App MyXL official.\n");
        cJSON_Delete(info);
        pause_enter();
        return;
    }

    int already_registered = (status && strcmp(status, "REGISTERED") == 0);
    cJSON_Delete(info);

    if (already_registered) {
        printf("\n[!] Status: SUDAH REGISTERED (atas nama %s).\n",
               name && name[0] ? name : "tidak diketahui");
        printf("    Anda mau re-pair ke NIK lain? Ini butuh PUK SIM.\n");
        char ans[8];
        read_line("Lanjut re-pair? (y/N): ", ans, sizeof(ans));
        if (ans[0] != 'y' && ans[0] != 'Y') return;

        char nik[32], kk[32], puk[32];
        if (!read_required("NIK baru (16 digit): ", nik, sizeof(nik))) return;
        if (!read_required("KK  baru (16 digit): ", kk,  sizeof(kk)))  return;
        if (!read_required("PUK SIM   (8 digit): ", puk, sizeof(puk))) return;
        if (!is_digits_len(nik, 16, 16) || !is_digits_len(kk, 16, 16) ||
            !is_digits_len(puk, 8, 8)) {
            printf("[!] Format salah.\n"); pause_enter(); return;
        }

        if (!confirm_write("Re-pair MSISDN ke NIK baru via PUK")) {
            printf("Dibatalkan.\n"); pause_enter(); return;
        }

        printf("\n[2/4] Validate PUK...\n"); fflush(stdout);
        cJSON* pv = registration_validate_puk_v9(base, api_key, xdata_key, api_secret,
                                                 id_token, msisdn, puk);
        if (!json_status_is_success(pv)) {
            if (pv) cJSON_Delete(pv);
            printf("    V9 fail, coba V8...\n");
            pv = registration_validate_puk_v8(base, api_key, xdata_key, api_secret,
                                              id_token, msisdn, puk);
        }
        if (!json_status_is_success(pv)) {
            print_response_summary(pv, "PUK INVALID — abort.", NULL);
            if (pv) cJSON_Delete(pv);
            pause_enter(); return;
        }
        cJSON_Delete(pv);
        printf("    PUK valid.\n");

        printf("\n[3/4] Submit re-pair (auth/regist/dukcapil + PUK, non-autopair)...\n");
        fflush(stdout);
        cJSON* r = registration_dukcapil_with_puk(base, api_key, xdata_key, api_secret,
                                                  id_token, msisdn, nik, kk, puk);
        print_response_summary(r, "Re-pair GAGAL.",
                               "Re-pair BERHASIL — MSISDN sekarang teregistrasi atas NIK baru.");
        print_raw(r);
        cJSON_Delete(r);
        pause_enter();
        return;
    }

    /* Belum registered: tanya apakah user punya PUK. */
    printf("\n[!] Status: BELUM REGISTERED.\n");
    printf("    Anda punya PUK SIM (dari kemasan)?\n");
    char puk_choice[8];
    read_line("[1] Punya PUK (recommended, lebih reliable)\n"
              "[2] Tidak punya, coba simple dukcapil saja\n"
              "Pilih (1/2): ", puk_choice, sizeof(puk_choice));

    char nik[32], kk[32], puk[32];
    if (!read_required("\nNIK (16 digit): ", nik, sizeof(nik))) return;
    if (!read_required("KK  (16 digit): ", kk,  sizeof(kk)))  return;
    if (!is_digits_len(nik, 16, 16) || !is_digits_len(kk, 16, 16)) {
        printf("[!] NIK dan KK harus 16 digit.\n"); pause_enter(); return;
    }

    if (puk_choice[0] == '1') {
        if (!read_required("PUK (8 digit): ", puk, sizeof(puk))) return;
        if (!is_digits_len(puk, 8, 8)) {
            printf("[!] PUK 8 digit.\n"); pause_enter(); return;
        }

        if (!confirm_write("Registrasi SIM baru (autopair + PUK)")) {
            printf("Dibatalkan.\n"); pause_enter(); return;
        }

        printf("\n[2/4] Validate PUK...\n"); fflush(stdout);
        cJSON* pv = registration_validate_puk_v9(base, api_key, xdata_key, api_secret,
                                                 id_token, msisdn, puk);
        if (!json_status_is_success(pv)) {
            if (pv) cJSON_Delete(pv);
            pv = registration_validate_puk_v8(base, api_key, xdata_key, api_secret,
                                              id_token, msisdn, puk);
        }
        if (!json_status_is_success(pv)) {
            print_response_summary(pv, "PUK INVALID — abort.", NULL);
            if (pv) cJSON_Delete(pv);
            pause_enter(); return;
        }
        cJSON_Delete(pv);
        printf("    PUK valid.\n");

        printf("\n[3/4] Submit autopair...\n"); fflush(stdout);
        cJSON* r = registration_dukcapil_autopair(base, api_key, xdata_key, api_secret,
                                                  id_token, msisdn, nik, kk, puk);
        print_response_summary(r, "Autopair GAGAL.",
                               "Autopair BERHASIL — SIM aktif & teregistrasi.");
        print_raw(r);
        cJSON_Delete(r);
    } else {
        if (!confirm_write("Registrasi simple dukcapil tanpa PUK")) {
            printf("Dibatalkan.\n"); pause_enter(); return;
        }

        printf("\n[2/4] Submit simple dukcapil...\n"); fflush(stdout);
        cJSON* r = registration_dukcapil_simple(base, api_key, xdata_key, api_secret,
                                                id_token, msisdn, nik, kk);
        if (json_status_is_success(r)) {
            print_response_summary(r, NULL,
                                   "Registrasi BERHASIL — MSISDN sekarang terdaftar.");
            print_raw(r);
            cJSON_Delete(r);
        } else {
            const char* code = response_error_code(r);
            printf("    Simple dukcapil gagal");
            if (code) printf(" (kode: %s — %s)", code,
                             registration_error_message(code));
            printf(".\n");

            /* Smart fallback: kalau gagal 165 (NIK KK not match), tawarin
             * coba autopair via PUK. Sering kali Dukcapil database lag dan
             * autopair dengan PUK valid bypass cek tersebut. */
            int suggest_puk = (code &&
                               (strcmp(code, "165") == 0 ||
                                strcmp(code, "170") == 0 ||
                                strcmp(code, "171") == 0));
            if (r) cJSON_Delete(r);

            if (suggest_puk) {
                printf("\n    [*] Coba fallback autopair dengan PUK?\n");
                char yn[8];
                read_line("    (y/N): ", yn, sizeof(yn));
                if (yn[0] == 'y' || yn[0] == 'Y') {
                    if (read_required("    PUK (8 digit): ", puk, sizeof(puk)) &&
                        is_digits_len(puk, 8, 8)) {
                        printf("\n    [3/4] Submit autopair fallback...\n"); fflush(stdout);
                        cJSON* r2 = registration_dukcapil_autopair(
                            base, api_key, xdata_key, api_secret,
                            id_token, msisdn, nik, kk, puk);
                        print_response_summary(r2, "Autopair fallback juga GAGAL.",
                                               "Fallback BERHASIL.");
                        print_raw(r2);
                        cJSON_Delete(r2);
                    } else {
                        printf("    [!] PUK tidak valid format.\n");
                    }
                }
            }
        }
    }

    pause_enter();
}

/* ============== Main menu loop ============== */

void registration_menu(const char* base_api,
                       const char* api_key,
                       const char* xdata_key,
                       const char* x_api_secret,
                       const char* id_token) {
    while (1) {
        section_header("Registrasi Kartu (Wizard)");
        printf("  -- READ-ONLY (zero risk) --\n");
        printf("  1. Cek Status Registrasi MSISDN\n");
        printf("  2. Validasi NIK + KK + MSISDN\n");
        printf("  3. Validasi PUK SIM (V9 -> V8 fallback)\n");
        printf("  4. Cek Eligibility Biometric\n");
        printf("  -- WRITE OPERATIONS (butuh konfirmasi YA) --\n");
        printf("  5. Registrasi Standar (NIK + KK)\n");
        printf("  6. Registrasi SIM Baru (NIK + KK + PUK + autopair)\n");
        printf("  7. Registrasi Manual + PUK (re-pair non-autopair)\n");
        printf("  8. Request OTP Registrasi\n");
        printf("  9. Validate OTP Registrasi\n");
        printf("  -- AUTO PILOT --\n");
        printf("  W. WIZARD PINTAR (auto-pilih flow optimal)\n");
        printf("\n");
        printf("  99. Kembali ke menu utama\n");
        printf("\n");
        printf("Pilih: ");
        fflush(stdout);

        char buf[16];
        if (!fgets(buf, sizeof(buf), stdin)) return;
        buf[strcspn(buf, "\n")] = '\0';

        if      (strcmp(buf, "99") == 0)               return;
        else if (strcmp(buf, "1")  == 0) sub_get_info(base_api, api_key, xdata_key, x_api_secret, id_token);
        else if (strcmp(buf, "2")  == 0) sub_check_dukcapil(base_api, api_key, xdata_key, x_api_secret, id_token);
        else if (strcmp(buf, "3")  == 0) sub_validate_puk(base_api, api_key, xdata_key, x_api_secret, id_token);
        else if (strcmp(buf, "4")  == 0) sub_biometric_check(base_api, api_key, xdata_key, x_api_secret, id_token);
        else if (strcmp(buf, "5")  == 0) sub_register_simple(base_api, api_key, xdata_key, x_api_secret, id_token);
        else if (strcmp(buf, "6")  == 0) sub_register_with_puk(base_api, api_key, xdata_key, x_api_secret, id_token, 1);
        else if (strcmp(buf, "7")  == 0) sub_register_with_puk(base_api, api_key, xdata_key, x_api_secret, id_token, 0);
        else if (strcmp(buf, "8")  == 0) sub_request_otp(base_api, api_key, xdata_key, x_api_secret, id_token);
        else if (strcmp(buf, "9")  == 0) sub_validate_otp(base_api, api_key, xdata_key, x_api_secret, id_token);
        else if (strcmp(buf, "w")  == 0 || strcmp(buf, "W") == 0)
                                          sub_wizard(base_api, api_key, xdata_key, x_api_secret, id_token);
        else {
            printf("\n[!] Pilihan tidak valid.\n");
            pause_enter();
        }
    }
}

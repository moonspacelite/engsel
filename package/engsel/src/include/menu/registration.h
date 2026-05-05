/*
 * Menu Registrasi Kartu (Menu R) — versi proper.
 *
 * Old `show_register_menu` (di history.c) cuma 1 endpoint single-shot.
 * Versi ini bikin wizard lengkap yang mirip flow App MyXL native:
 *
 *   1. Cek Status Registrasi MSISDN              [READ]
 *   2. Validasi NIK + KK + MSISDN                [READ]
 *   3. Validasi PUK SIM (V8/V9 auto-fallback)    [READ]
 *   4. Cek Eligibility Biometric                 [READ]
 *   5. Registrasi Standar (NIK + KK)             [WRITE]
 *   6. Registrasi SIM Baru (NIK + KK + PUK)      [WRITE]
 *   7. Registrasi Manual + PUK (non-autopair)    [WRITE]
 *   8. Request OTP Registrasi                    [WRITE]
 *   9. Validate OTP Registrasi                   [WRITE]
 *   W. WIZARD PINTAR (auto-pilih flow)           [WRITE]
 *
 * Wizard pintar (W) auto-detect status MSISDN dan route ke endpoint yang tepat:
 *   - Belum register + ada PUK         → autopair
 *   - Belum register + tanpa PUK       → simple dukcapil
 *   - Sudah register tapi NIK beda     → re-pair via PUK (non-autopair)
 *   - Eligible biometric only          → kasih warning + redirect ke App MyXL
 */
#ifndef ENGSEL_MENU_REGISTRATION_H
#define ENGSEL_MENU_REGISTRATION_H

void registration_menu(const char* base_api,
                       const char* api_key,
                       const char* xdata_key,
                       const char* x_api_secret,
                       const char* id_token);

#endif

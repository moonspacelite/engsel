/* Migration & Kontrak (PRIO/HYBRID/MPD/Recontract/BrandPortability)
 *
 * Endpoint discovery dari decompile APK MyXL 9.1.0:
 *   /auths/api/v8/prio/*           - registration & activation PRIO
 *   /infos/api/v8/pre-to-prioh/*   - migrasi PREPAID -> PRIOHYBRID (KYC)
 *   /store/api/v8/pre-to-postpaid/*+ /sharings/api/v8/pre-to-postpaid/submit
 *                                  - aktivasi kontrak MPD 6/12/24m
 *   /refinancing/api/v8/recontract/*       - perpanjang kontrak existing
 *   /registinfo/api/v8/brand-portability/* - trial PRIO 7-hari (revertible)
 *   /sharings/api/v8/co-brand/*    - eligibility co-brand (LinkAja/OVO)
 *   /sharings/api/v8/migration/*   - estimasi billing migrasi
 *   /sharings/api/v8/packages/check-prio-hybrid - cek eligibility PRIOHYBRID
 *
 * Semua fungsi mengembalikan cJSON* yang sudah di-decrypt (harus di-cJSON_Delete).
 */
#ifndef ENGSEL_CLIENT_MIGRATION_H
#define ENGSEL_CLIENT_MIGRATION_H

#include "../cJSON.h"

/* ============== PRIO Registration & Activation =================== */

/* Cek apakah NIK sudah teregistrasi di dukcapil */
cJSON* mig_prio_check_dukcapil_registration(const char* base, const char* api_key,
                                            const char* xdata_key, const char* sec,
                                            const char* id_token,
                                            const char* nik, const char* family_name,
                                            const char* birthdate);

/* Cek status aktivasi dukcapil (setelah create-dukcapil) */
cJSON* mig_prio_check_dukcapil_activation(const char* base, const char* api_key,
                                          const char* xdata_key, const char* sec,
                                          const char* id_token, const char* msisdn);

/* Buat record dukcapil baru */
cJSON* mig_prio_create_dukcapil(const char* base, const char* api_key,
                                const char* xdata_key, const char* sec,
                                const char* id_token,
                                const char* nik, const char* family_name,
                                const char* birthdate, const char* msisdn);

/* DAFTAR ke PRIO (PREPAID -> PRIO conversion) — V1 */
cJSON* mig_prio_registration(const char* base, const char* api_key,
                             const char* xdata_key, const char* sec,
                             const char* id_token,
                             const char* msisdn, const char* last_iccid_digit,
                             const char* registration_number);

/* AKTIVASI PRIO (final step setelah registration) */
cJSON* mig_prio_activation(const char* base, const char* api_key,
                           const char* xdata_key, const char* sec,
                           const char* id_token,
                           const char* msisdn, const char* last_iccid_digit,
                           const char* registration_number);

/* PRIOHYBRID -> PRIORITAS conversion (V2: cuma butuh msisdn) */
cJSON* mig_prio_post_hybridactivation(const char* base, const char* api_key,
                                      const char* xdata_key, const char* sec,
                                      const char* id_token, const char* msisdn);

/* Cek status registrasi PRIO */
cJSON* mig_prio_registration_status(const char* base, const char* api_key,
                                    const char* xdata_key, const char* sec,
                                    const char* id_token, const char* msisdn);

/* Cek status aktivasi PRIO */
cJSON* mig_prio_activation_status(const char* base, const char* api_key,
                                  const char* xdata_key, const char* sec,
                                  const char* id_token, const char* msisdn);

/* ============== PRIOHYBRID (PRE -> PRIOH) ======================== */

/* Cek eligibility PRIOHYBRID upgrade (apakah subscription saat ini bisa) */
cJSON* mig_check_prio_hybrid(const char* base, const char* api_key,
                             const char* xdata_key, const char* sec,
                             const char* id_token);

/* Cek status migrasi PRIOHYBRID yang sedang berjalan */
cJSON* mig_pre_to_prioh_status(const char* base, const char* api_key,
                               const char* xdata_key, const char* sec,
                               const char* id_token);

/* Submit data migrasi PRIOHYBRID (KYC: KTP + selfie + alamat) */
cJSON* mig_pre_to_prioh_submit(const char* base, const char* api_key,
                               const char* xdata_key, const char* sec,
                               const char* id_token,
                               const char* name, const char* email,
                               const char* birthdate, const char* card_id_nik,
                               const char* card_image_b64,
                               const char* card_selfie_b64,
                               const char* address, const char* birthday_place,
                               int is_enterprise);

/* ============== Kontrak MPD (PRE -> POSTPAID) ==================== */

/* List MPD bundle offer yang tersedia (6m, 12m, 24m) */
cJSON* mig_pre_to_post_offers(const char* base, const char* api_key,
                              const char* xdata_key, const char* sec,
                              const char* id_token);

/* Cek eligibility kontrak MPD */
cJSON* mig_pre_to_post_check_eligibility(const char* base, const char* api_key,
                                         const char* xdata_key, const char* sec,
                                         const char* id_token,
                                         const char* offer_id);

/* Submit aktivasi kontrak MPD (FINAL — affects billing) */
cJSON* mig_pre_to_post_submit(const char* base, const char* api_key,
                              const char* xdata_key, const char* sec,
                              const char* id_token,
                              const char* offer_id, const char* msisdn);

/* ============== Recontract (perpanjang kontrak existing) ========= */

cJSON* mig_recontract_check_eligibility(const char* base, const char* api_key,
                                        const char* xdata_key, const char* sec,
                                        const char* id_token);

/* ============== Brand Portability Trial (PRIO 7-hari) ============ */

/* Cek info trial */
cJSON* mig_bp_trial_info(const char* base, const char* api_key,
                         const char* xdata_key, const char* sec,
                         const char* id_token);

/* Trigger trial PRIO 7-hari */
cJSON* mig_bp_trial_start(const char* base, const char* api_key,
                          const char* xdata_key, const char* sec,
                          const char* id_token);

/* Revert trial kembali ke PREPAID */
cJSON* mig_bp_trial_revert(const char* base, const char* api_key,
                           const char* xdata_key, const char* sec,
                           const char* id_token);

/* Redeem bonus trial */
cJSON* mig_bp_trial_redeem_bonus(const char* base, const char* api_key,
                                 const char* xdata_key, const char* sec,
                                 const char* id_token);

/* Kirim OTP migrasi (brand portability) */
cJSON* mig_bp_otp_send(const char* base, const char* api_key,
                       const char* xdata_key, const char* sec,
                       const char* id_token, const char* msisdn);

/* Validate OTP migrasi */
cJSON* mig_bp_otp_validate(const char* base, const char* api_key,
                           const char* xdata_key, const char* sec,
                           const char* id_token,
                           const char* msisdn, const char* otp);

/* ============== Co-brand (LinkAja, OVO, etc) ==================== */

cJSON* mig_cobrand_check(const char* base, const char* api_key,
                         const char* xdata_key, const char* sec,
                         const char* id_token);

cJSON* mig_cobrand_check_eligibility(const char* base, const char* api_key,
                                     const char* xdata_key, const char* sec,
                                     const char* id_token);

/* ============== Estimasi Billing Migrasi ========================= */

cJSON* mig_billing_estimation(const char* base, const char* api_key,
                              const char* xdata_key, const char* sec,
                              const char* id_token,
                              const char* offer_id, const char* msisdn);

#endif

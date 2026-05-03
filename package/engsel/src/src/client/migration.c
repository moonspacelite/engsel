/* Implementasi client untuk endpoint Migrasi & Kontrak.
 * Lihat client/migration.h untuk dokumentasi tiap fungsi. */
#include <stdlib.h>
#include <string.h>
#include "../include/client/migration.h"
#include "../include/client/engsel.h"

static cJSON* post(const char* base, const char* key, const char* xdata,
                   const char* sec, const char* path, cJSON* payload, const char* id_token) {
    return send_api_request(base, key, xdata, sec, path, payload, id_token, "POST", NULL);
}

/* ============== PRIO Registration & Activation =================== */

cJSON* mig_prio_check_dukcapil_registration(const char* base, const char* api_key,
                                            const char* xdata_key, const char* sec,
                                            const char* id_token,
                                            const char* nik, const char* family_name,
                                            const char* birthdate) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "nik", nik ? nik : "");
    cJSON_AddStringToObject(p, "family_name", family_name ? family_name : "");
    cJSON_AddStringToObject(p, "birthdate", birthdate ? birthdate : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "auths/api/v8/prio/check-dukcapil-registration", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_prio_check_dukcapil_activation(const char* base, const char* api_key,
                                          const char* xdata_key, const char* sec,
                                          const char* id_token, const char* msisdn) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "auths/api/v8/prio/check-dukcapil-activation", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_prio_create_dukcapil(const char* base, const char* api_key,
                                const char* xdata_key, const char* sec,
                                const char* id_token,
                                const char* nik, const char* family_name,
                                const char* birthdate, const char* msisdn) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "nik", nik ? nik : "");
    cJSON_AddStringToObject(p, "family_name", family_name ? family_name : "");
    cJSON_AddStringToObject(p, "birthdate", birthdate ? birthdate : "");
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "auths/api/v8/prio/create-dukcapil", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_prio_registration(const char* base, const char* api_key,
                             const char* xdata_key, const char* sec,
                             const char* id_token,
                             const char* msisdn, const char* last_iccid_digit,
                             const char* registration_number) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(p, "last_iccid_digit", last_iccid_digit ? last_iccid_digit : "");
    cJSON_AddStringToObject(p, "registration_number", registration_number ? registration_number : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "auths/api/v8/prio/registration", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_prio_activation(const char* base, const char* api_key,
                           const char* xdata_key, const char* sec,
                           const char* id_token,
                           const char* msisdn, const char* last_iccid_digit,
                           const char* registration_number) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(p, "last_iccid_digit", last_iccid_digit ? last_iccid_digit : "");
    cJSON_AddStringToObject(p, "registration_number", registration_number ? registration_number : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "auths/api/v8/prio/activation", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_prio_post_hybridactivation(const char* base, const char* api_key,
                                      const char* xdata_key, const char* sec,
                                      const char* id_token, const char* msisdn) {
    /* V2: cuma butuh msisdn (PRIOHYBRID sudah punya dukcapil di server) */
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "auths/api/v8/prio/post-hybridactivation", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_prio_registration_status(const char* base, const char* api_key,
                                    const char* xdata_key, const char* sec,
                                    const char* id_token, const char* msisdn) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "auths/api/v8/prio/registration-status", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_prio_activation_status(const char* base, const char* api_key,
                                  const char* xdata_key, const char* sec,
                                  const char* id_token, const char* msisdn) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "auths/api/v8/prio/activation-status", p, id_token);
    cJSON_Delete(p);
    return r;
}

/* ============== PRIOHYBRID (PRE -> PRIOH) ======================== */

cJSON* mig_check_prio_hybrid(const char* base, const char* api_key,
                             const char* xdata_key, const char* sec,
                             const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "sharings/api/v8/packages/check-prio-hybrid", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_pre_to_prioh_status(const char* base, const char* api_key,
                               const char* xdata_key, const char* sec,
                               const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "infos/api/v8/pre-to-prioh/migration-status", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_pre_to_prioh_submit(const char* base, const char* api_key,
                               const char* xdata_key, const char* sec,
                               const char* id_token,
                               const char* name, const char* email,
                               const char* birthdate, const char* card_id_nik,
                               const char* card_image_b64,
                               const char* card_selfie_b64,
                               const char* address, const char* birthday_place,
                               int is_enterprise) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "name", name ? name : "");
    cJSON_AddStringToObject(p, "email", email ? email : "");
    cJSON_AddStringToObject(p, "birthdate", birthdate ? birthdate : "");
    cJSON_AddStringToObject(p, "cardId", card_id_nik ? card_id_nik : "");
    cJSON_AddStringToObject(p, "cardImage", card_image_b64 ? card_image_b64 : "");
    cJSON_AddStringToObject(p, "cardSelfieImage", card_selfie_b64 ? card_selfie_b64 : "");
    cJSON_AddStringToObject(p, "address", address ? address : "");
    cJSON_AddStringToObject(p, "birthdayPlace", birthday_place ? birthday_place : "");
    cJSON_AddBoolToObject(p, "isEnterprise", is_enterprise ? 1 : 0);
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "infos/api/v8/pre-to-prioh/update-migration-data", p, id_token);
    cJSON_Delete(p);
    return r;
}

/* ============== Kontrak MPD (PRE -> POSTPAID) ==================== */

cJSON* mig_pre_to_post_offers(const char* base, const char* api_key,
                              const char* xdata_key, const char* sec,
                              const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddBoolToObject(p, "is_enterprise", 0);
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "store/api/v8/pre-to-postpaid/offers", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_pre_to_post_check_eligibility(const char* base, const char* api_key,
                                         const char* xdata_key, const char* sec,
                                         const char* id_token,
                                         const char* offer_id) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "offer_id", offer_id ? offer_id : "");
    cJSON_AddBoolToObject(p, "is_enterprise", 0);
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "store/api/v8/pre-to-postpaid/check-eligibility", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_pre_to_post_submit(const char* base, const char* api_key,
                              const char* xdata_key, const char* sec,
                              const char* id_token,
                              const char* offer_id, const char* msisdn) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "offer_id", offer_id ? offer_id : "");
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddBoolToObject(p, "is_enterprise", 0);
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "sharings/api/v8/pre-to-postpaid/submit", p, id_token);
    cJSON_Delete(p);
    return r;
}

/* ============== Recontract ======================================= */

cJSON* mig_recontract_check_eligibility(const char* base, const char* api_key,
                                        const char* xdata_key, const char* sec,
                                        const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "refinancing/api/v8/recontract/check-eligibility", p, id_token);
    cJSON_Delete(p);
    return r;
}

/* ============== Brand Portability Trial ========================== */

cJSON* mig_bp_trial_info(const char* base, const char* api_key,
                         const char* xdata_key, const char* sec,
                         const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "registinfo/api/v8/brand-portability/trial/info", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_bp_trial_start(const char* base, const char* api_key,
                          const char* xdata_key, const char* sec,
                          const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "registinfo/api/v8/brand-portability/trial", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_bp_trial_revert(const char* base, const char* api_key,
                           const char* xdata_key, const char* sec,
                           const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "registinfo/api/v8/brand-portability/trial/revert", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_bp_trial_redeem_bonus(const char* base, const char* api_key,
                                 const char* xdata_key, const char* sec,
                                 const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "registinfo/api/v8/brand-portability/trial/redeem-bonus", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_bp_otp_send(const char* base, const char* api_key,
                       const char* xdata_key, const char* sec,
                       const char* id_token, const char* msisdn) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "registinfo/api/v8/brand-portability/otp-send", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_bp_otp_validate(const char* base, const char* api_key,
                           const char* xdata_key, const char* sec,
                           const char* id_token,
                           const char* msisdn, const char* otp) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(p, "otp", otp ? otp : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "registinfo/api/v8/brand-portability/otp-validate", p, id_token);
    cJSON_Delete(p);
    return r;
}

/* ============== Co-brand ========================================= */

cJSON* mig_cobrand_check(const char* base, const char* api_key,
                         const char* xdata_key, const char* sec,
                         const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "sharings/api/v8/co-brand/check", p, id_token);
    cJSON_Delete(p);
    return r;
}

cJSON* mig_cobrand_check_eligibility(const char* base, const char* api_key,
                                     const char* xdata_key, const char* sec,
                                     const char* id_token) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "sharings/api/v8/co-brand/check-eligibility", p, id_token);
    cJSON_Delete(p);
    return r;
}

/* ============== Estimasi Billing Migrasi ========================= */

cJSON* mig_billing_estimation(const char* base, const char* api_key,
                              const char* xdata_key, const char* sec,
                              const char* id_token,
                              const char* offer_id, const char* msisdn) {
    cJSON* p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "offer_id", offer_id ? offer_id : "");
    cJSON_AddStringToObject(p, "msisdn", msisdn ? msisdn : "");
    cJSON_AddStringToObject(p, "lang", "en");
    cJSON* r = post(base, api_key, xdata_key, sec,
                    "sharings/api/v8/migration/billing-estimation-charge", p, id_token);
    cJSON_Delete(p);
    return r;
}

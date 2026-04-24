/* Navigasi "99 = kembali ke menu utama" untuk seluruh CLI.
 *
 * Problem sebelumnya: banyak sub-menu memperlakukan "99" == "00" (keduanya
 * return satu level ke parent), padahal user mengharapkan "99" unwind
 * sampai ke menu utama.
 *
 * Solusi: flag global `g_nav_goto_main`.
 *   - Sub-menu yang menerima "99" memanggil `nav_trigger_goto_main()`
 *     sebelum return.
 *   - Parent menu setelah memanggil sub-menu mengecek `nav_should_return()`
 *     dan ikut return kalau true.
 *   - Main loop (main.c) reset flag ke 0 di awal tiap iterasi.
 */
#ifndef ENGSEL_UTIL_NAV_H
#define ENGSEL_UTIL_NAV_H

extern int g_nav_goto_main;

/* Set flag unwind. */
void nav_trigger_goto_main(void);
/* Reset flag (dipanggil main loop di awal tiap iterasi). */
void nav_reset(void);
/* Cek apakah caller harus ikut return/break. */
int nav_should_return(void);

#endif

#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <stddef.h>

/* Baca seluruh isi file ke buffer heap (NUL-terminated). Caller free.
 * Kembalikan NULL bila file tidak ada/gagal baca. Jika out_size != NULL,
 * diisi ukuran byte aktual (tanpa NUL). */
char* file_read_all(const char* path, size_t* out_size);

/* Tulis data ke file secara atomic: tulis ke "{path}.tmp" lalu rename().
 * Kembalikan 0 jika sukses, -1 jika gagal. `content` boleh NULL (string kosong). */
int   file_write_atomic(const char* path, const char* content);

#endif

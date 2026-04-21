#ifndef RFID_HELPER_H
#define RFID_HELPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define UID_NOT_FOUND ((size_t)-1)

typedef struct {
    uint8_t uid[7];
    size_t uid_len;
    char name[32];
} rfid_record_t;

size_t check_uid(const rfid_record_t *records, size_t num_records, const uint8_t *target_uid, size_t uid_len);
void uid_to_string(const uint8_t *uid, size_t uid_len, char *out, size_t out_size);
#endif

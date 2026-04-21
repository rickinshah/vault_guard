#ifndef RFID_HELPER_H
#define RFID_HELPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint8_t uid[16];
    uint8_t uid_len;
    char name[32];
} rfid_record_t;

bool uid_exists(const rfid_record_t *records, size_t num_records, const uint8_t *target_uid, size_t uid_len);

#endif

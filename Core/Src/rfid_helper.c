#include "rfid_helper.h"
#include <string.h>

bool uid_exists(const rfid_record_t *records, size_t num_records, const uint8_t *target_uid, size_t uid_len) {
    for(int i = 0; i < num_records; i++) {
        if(records->uid_len == uid_len && memcmp(records->uid, target_uid, uid_len)) {
            return true;
        }
    }
    return false;
}
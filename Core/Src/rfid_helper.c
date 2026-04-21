#include "rfid_helper.h"
#include <string.h>

size_t check_uid(const rfid_record_t *records, size_t num_records, const uint8_t *target_uid, size_t uid_len) {
    for(size_t i = 0; i < num_records; i++) {
        if(records[i].uid_len == uid_len && memcmp(records[i].uid, target_uid, uid_len) == 0) {
            return i;
        }
    }
    return UID_NOT_FOUND;
}

// need char size of 5 * uid_len + 1
void uid_to_string(const uint8_t *uid, size_t uid_len, char *out, size_t out_size) {
    char map[] = "0123456789ABCDEF";
    size_t index = 0;
    for(size_t i = 0; i < uid_len; i++) {
        if(index + 5 >= out_size) break;
        
        uint8_t high = uid[i] >> 4;
        uint8_t low = uid[i] & 0x0F;
        out[index++] = '0';
        out[index++] = 'x';
        out[index++] =  map[high];
        out[index++] = map[low];
        out[index++] = ' ';
    }
    if (index > 0)
        out[index - 1] = '\0';
    else if (out_size > 0)
        out[index] = '\0';
}

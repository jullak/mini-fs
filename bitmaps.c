#include "bitmaps.h"
#include "errors.h"


int32_t bitmap_find_free(char * bitmap) {
  for (int i = 0; i < sizeof(bitmap) / sizeof(char); ++i) {
    for (int shift = 7; shift >= 0; --shift) {
      if ((bitmap[i] & (1 << shift)) == 0) {
        return i * 8 + (7 - shift);
      }
    }
  }
  return ENOSPACE;
}

int32_t bitmap_set_lock(char * bitmap, int32_t id) {
  uint32_t bitmap_id = id / 8;
  uint32_t pos = id % 8;

  if (bitmap[bitmap_id] & (1 << (7 - pos))) {
    return ELOCK;
  }

  bitmap[bitmap_id] |= (1 << (7 - pos));
  return 0;
}

int32_t bitmap_set_free(char * bitmap, int32_t id) {
  uint32_t bitmap_id = id / 8;
  uint32_t pos = id % 8;

  bitmap[bitmap_id] &= ~(1 << (7 - pos));
  return 0;
}

bool bitmap_is_lock(char * bitmap, int32_t id) {
  uint32_t bitmap_id = id / 8;
  uint32_t pos = id % 8;

  return bitmap[bitmap_id] & (1 << (7 - pos));
}
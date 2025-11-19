#if defined(WLED_ENABLE_FULL_FONTS)

#include "codepages.h"
#include <string.h>

// translates the next unicode UTF-8 item to 2-byte "code points" (reduced UTF-16)
uint16_t unicodeToWchar16(const char* utf8, size_t maxLen) {
  // TODO: implement proper UTF‑8 → reduced UTF‑16 decoding
  (void)utf8;
  (void)maxLen;
  return 0;  // sentinel: "no character"
}

// returns a pointer to the next unicode item - can be used to "advance" conversion after unicodeToWchar16()
// return nullptr at end of input
const char* nextUnicode(const char* utf8, size_t maxLen) {
  (void)maxLen;
  if (!utf8) return nullptr;
  if (strlen(utf8)>0) return utf8+1;
  else return  nullptr; // last code read
  // TODO: implement proper UTF‑8 iteration
}

#endif

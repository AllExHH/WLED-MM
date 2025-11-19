#if defined(WLED_ENABLE_FULL_FONTS)

#include "codepages.h"
#include <string.h>

// translates the next unicode UTF-8 item to 2-byte "code points" (reduced UTF-16)
uint16_t unicodeToWchar16(const char* utf8, size_t maxLen) {
  // to be implemented
}

// returns a pointer to the next unicode item - can be used to "advance" conversion after unicodeToWchar16()
// return nullptr at end of input
const char* nextUnicode(const char* utf8, size_t maxLen) {
  // to be implemented
}

#endif

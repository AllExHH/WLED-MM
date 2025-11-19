#ifndef WLED_CODEPAGES_H
#define WLED_CODEPAGES_H
#include <stdlib.h> // needed to get uint16_t definition

// translates unicode UTF-8 "character code" into 2-byte "code point" (reduced UTF-16)
uint16_t unicodeToWchar16(const char* utf8, size_t maxLen);     // unicodetool.cpp

// returns a pointer to the next unicode item - can be used to "advance" conversion after unicodeToWchar16()
// return nullptr at end of input
const char* nextUnicode(const char* utf8, size_t maxLen);      // unicodetool.cpp

// translates unicode 2-byte (UTF-16) "code point" into corresponding character in codepage 437 (IBM PC aka PC-8)
uint16_t wchar16ToCodepage437(uint16_t wideChar);              // codepage437.cpp

#endif

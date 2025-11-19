#ifndef WLED_CODEPAGES_H
#define WLED_CODEPAGES_H
#include <stdlib.h> // needed to get uint16_t definition

constexpr uint16_t UNKNOWN_CODE = 0x2022; //  •
constexpr uint16_t BAD_CODE     = 0x2022; //  •

// UTF‑8 → reduced UTF‑16 decoding
// translates the next unicode UTF-8 item into a 2-byte "code point"
// return "•" in case of input errors, and for unsupported/invalid UTF-8
uint16_t unicodeToWchar16(const unsigned char* utf8, size_t maxLen);           // unicodetool.cpp

// returns a pointer to the next unicode item - can be used to "advance" conversion after unicodeToWchar16()
// return nullptr at end of input
const unsigned char* nextUnicode(const unsigned char* utf8, size_t maxLen);    // unicodetool.cpp

// translates unicode 2-byte (UTF-16) "code point" into corresponding character in codepage 437 (IBM PC aka PC-8)
uint16_t wchar16ToCodepage437(uint16_t wideChar);                              // codepage437.cpp

#endif

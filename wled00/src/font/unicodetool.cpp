#if defined(WLED_ENABLE_FULL_FONTS)

#include "codepages.h"
#include <string.h>
#include <algorithm>  // adds std::min / std::max
using namespace std;  // I don't want to write std::min

// Helper to validate continuation byte
static inline bool isValidContinuation(unsigned char byte) {
  return (byte & 0b11000000) == 0b10000000;
}

// UTF‑8 → reduced UTF‑16 decoding
// translates the next unicode UTF-8 item into a 2-byte "code point"
uint16_t unicodeToWchar16(const unsigned char* utf8, size_t maxLen) {
  if (!utf8 || (maxLen < 1) || *utf8 == '\0') return 0; // sanity check

  size_t length = strlen((const char*) utf8);
  length = min(length, maxLen);
  if (length < 1) return 0;            // sanity check

  unsigned char ch0 = *utf8;           // get leading character
  uint32_t codepoint = ch0;            // our resulting UTF-16 code point

  if (ch0 <= 0x7F) return ch0;                    // 1-byte ASCII (0x00-0x7F)
  if ((ch0 & 0b11100000) == 0b11000000) {         // 2-byte sequence (0xC2-0xDF)
    // uses lower 5 bits of the first byte, and lower 6 bits from the next byte
    if (length < 2 || !isValidContinuation(utf8[1])) return 0x2022; // • for malformed
    codepoint = ((ch0 & 0b00011111) << 6) | (utf8[1] & 0b00111111);
    if (codepoint < 0x80) return 0x2022; // Reject overlong encodings (must be >= 0x80)
    return uint16_t(codepoint);
  } else {
    if ((ch0 & 0b11110000) == 0b11100000) {       // 3-byte sequence (0xE0-0xEF)
      // uses lower 4 bits of the first byte, and lower 6 bits from the next byte, lower 6 bits from third byte
      if (length < 3 || !isValidContinuation(utf8[1]) || !isValidContinuation(utf8[2])) return 0x2022; // • for malformed
      codepoint = ((ch0 & 0b00001111) << 12) | ((utf8[1] & 0b00111111) << 6) | (utf8[2] & 0b00111111);
      if (codepoint < 0x800) return 0x2022;                          // Reject overlong encodings (must be >= 0x800)
      if (codepoint >= 0xD800 && codepoint <= 0xDFFF) return 0x2022; // Reject UTF-16 surrogate pairs (U+D800..U+DFFF)
      if (codepoint >= 0x010000) codepoint = 0x2022;                 // result exceeds uint16_t => return • for "unknown"
      return uint16_t(codepoint);
    }
  }
  // 4-byte sequence or invalid lead byte - since we only support up to 0xFFFF, return error marker
  return 0x2022; // • for unsupported/invalid
}

// returns a pointer to the next unicode item - can be used to "advance" conversion after unicodeToWchar16()
// return nullptr at end of input
const unsigned char* nextUnicode(const unsigned char* utf8, size_t maxLen) {
  if ((!utf8) || (maxLen < 1) || (*utf8 == 0)) return nullptr; // sanity check
  size_t length = strlen((const char*) utf8); // safe, because utf8 is a C string with proper NUL termination
  length = min(length, maxLen);
  if (length < 1) return nullptr;      // we are at end of input

  unsigned char ch0 = *utf8;           // get leading character
  size_t codeLength = 1;               // default:  1-byte ASCII
  if (ch0 >= 0x80) {
    if      ((ch0 & 0b11100000) == 0b11000000) codeLength = 2; // 2-byte sequence
    else if ((ch0 & 0b11110000) == 0b11100000) codeLength = 3; // 3-byte sequence
    else if ((ch0 & 0b11111000) == 0b11110000) codeLength = 4; // 4-byte sequence (not fully supported but we need to skip it)
    else codeLength = 1; // Skip single invalid byte and try to resync
  }

  if (length < codeLength) return nullptr; // Check if we have enough bytes
  else return utf8 + codeLength; // success: advance stream
}

#endif

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

  size_t length = strnlen((const char*) utf8, maxLen);
  length = min(length, maxLen);
  if (length < 1) return 0;            // sanity check

  unsigned char ch0 = *utf8;           // get leading character
  uint32_t codepoint = ch0;            // our resulting UTF-16 code point

  if (ch0 <= 0x7F) return ch0;                    // 1-byte ASCII (0x00-0x7F)
  if ((ch0 & 0b11100000) == 0b11000000) {         // 2-byte sequence (0xC2-0xDF)
    // uses lower 5 bits of the first byte, and lower 6 bits from the next byte
    if (length < 2 || !isValidContinuation(utf8[1])) return BAD_CODE; // malformed
    codepoint = ((ch0 & 0b00011111) << 6) | (utf8[1] & 0b00111111);
    if (codepoint < 0x80) return UNKNOWN_CODE;    // Reject overlong encodings (must be >= 0x80)
    return uint16_t(codepoint);
  } else {
    if ((ch0 & 0b11110000) == 0b11100000) {       // 3-byte sequence (0xE0-0xEF)
      // uses lower 4 bits of the first byte, and lower 6 bits from the next byte, lower 6 bits from third byte
      if (length < 3 || !isValidContinuation(utf8[1]) || !isValidContinuation(utf8[2])) return BAD_CODE; // malformed
      codepoint = ((ch0 & 0b00001111) << 12) | ((utf8[1] & 0b00111111) << 6) | (utf8[2] & 0b00111111);
      if (codepoint < 0x800) return UNKNOWN_CODE;                          // Reject overlong encodings (must be >= 0x800)
      if (codepoint >= 0xD800 && codepoint <= 0xDFFF) return UNKNOWN_CODE; // Reject UTF-16 surrogate pairs (U+D800..U+DFFF)
      if (codepoint >= 0x010000) codepoint = UNKNOWN_CODE;                 // result exceeds uint16_t => "unknown"
      return uint16_t(codepoint);
    }
  }
  // 4-byte sequence or invalid lead byte - since we only support up to 0xFFFF, return error marker
  return BAD_CODE; // unsupported/invalid
}

// returns a pointer to the next unicode item - can be used to "advance" conversion after unicodeToWchar16()
// return nullptr at end of input
const unsigned char* nextUnicode(const unsigned char* utf8, size_t maxLen) {
  if ((!utf8) || (maxLen < 1) || (*utf8 == 0)) return nullptr; // sanity check
  size_t length = strnlen((const char*) utf8, maxLen);
  length = min(length, maxLen);
  if (length < 1) return nullptr;      // we are at end of input

  unsigned char ch0 = *utf8;           // get leading character

  // Calculate code length based on lead byte
  size_t codeLength = 1;                                       // default:  1-byte ASCII
  if (ch0 >= 0x80) {
    if      ((ch0 & 0b11100000) == 0b11000000) codeLength = 2; // 2-byte sequence
    else if ((ch0 & 0b11110000) == 0b11100000) codeLength = 3; // 3-byte sequence
    else if ((ch0 & 0b11111000) == 0b11110000) codeLength = 4; // 4-byte sequence (not fully supported but we need to skip it)
    else codeLength = 1; // Skip single invalid byte and try to resync
  }

  // handle invalid continuation bytes
  if ((codeLength >= 2) && (length < 2 || !isValidContinuation(utf8[1]))) codeLength = 1; // try to re-sync
  if ((codeLength >= 3) && (length < 3 || !isValidContinuation(utf8[2]))) codeLength = 1; // try to re-sync
  if ((codeLength >= 4) && (length < 4 || !isValidContinuation(utf8[3]))) codeLength = 1; // try to re-sync

  if (length < codeLength) return nullptr;                     // Check if we have enough bytes
  else return utf8 + codeLength;                               // success: advance stream
}

// unicode-aware string length
size_t strlenUC(const unsigned char* utf8) {
  if ((utf8 == nullptr) || (utf8[0] == '\0')) return 0;
  size_t maxLen = strlen((const char *)utf8);
  size_t letters = 0;
  for(const unsigned char* now = utf8; now != nullptr && now[0] != '\0'; now = nextUnicode(now, maxLen)) // iterates over utf-8 and count letters
    letters++;

  return letters;
}

// returns the next (lesser) string index that is safe for cutting an UTF-8 string
// Important: calling code is responsible to provide a string with at least _where_ chars
size_t cutUnicodeAt(const unsigned char* utf8, size_t where) {
  if (utf8[where] <= 127) return where;  // ASCII
  size_t loopMin = max(0, int(where)-4); // max 4 characters backwards
  size_t whereStart = where;
  while ((isValidContinuation(utf8[where])) && (where > loopMin)) where--; // UTF-8: back until we find a non-continuation char

  if ((utf8[where] > 127) && isValidContinuation(utf8[whereStart])) where = max(0, int(where)-1); // most likely a UTF-8 lead byte -> go back one step
  return where;
}

#endif

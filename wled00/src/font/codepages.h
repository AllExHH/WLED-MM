#include <string.h>
#include <stdlib.h>

// translates unicode 2-byte (UTF-16) "code points" into corresponding characters in codepage 347 (IBM PC aka PC-8)
uint16_t wchar16ToCodepage437(uint16_t wideChar);     // codepage437.cpp
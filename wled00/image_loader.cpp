#include "wled.h"

#ifdef WLED_ENABLE_GIF

#include "GifDecoder.h"


/*
 * Functions to render images from filesystem to segments, used by the "Image" effect
 */

static File file;
static char lastFilename[34] = "/";
#if !defined(BOARD_HAS_PSRAM)
  static GifDecoder<256,256,11,true> decoder;  // WLEDMM use less RAM on boards without PSRAM - avoids crashes due to out-of-memory
#else
  static GifDecoder<320,320,12,true> decoder;
#endif
static bool gifDecodeFailed = false;
static unsigned long lastFrameDisplayTime = 0, currentFrameDelay = 0;

bool fileSeekCallback(unsigned long position) {
  return file.seek(position);
}

unsigned long filePositionCallback(void) {
  return file.position();
}

int fileReadCallback(void) {
  return file.read();
}

int fileReadBlockCallback(void * buffer, int numberOfBytes) {
  return file.read((uint8_t*)buffer, numberOfBytes);
}

int fileSizeCallback(void) {
  return file.size();
}

bool openGif(const char *filename) {  // side-effect: updates "file"
  file = WLED_FS.open(filename, "r");
  DEBUG_PRINTF("opening GIF file %s\n", filename);

  if (!file) return false;
  return true;
}

static Segment* activeSeg;
static uint16_t gifWidth, gifHeight;  // these two must stay uint16_t, because they are passed by reference
static unsigned segCols = 1;
static unsigned segRows = 1;
//static unsigned segLen = 1; // for future 1D support
static int expandX = 1;
static int expandY = 1;
static int lastX = -1, lastY = -1;

void screenClearCallback(void) {
  activeSeg->fill(0);
}

void updateScreenCallback(void) {
  // this callback runs when the decoder has finished painting all pixels
  // perfect time for adding blur
  if (activeSeg->intensity > 1) {
    uint8_t blurAmount = activeSeg->intensity >> 2;
    if ((blurAmount < 24) && (activeSeg->is2D())) activeSeg->blurRows(activeSeg->intensity >> 1);  // some blur - fast
    else activeSeg->blur(blurAmount);                                                              // more blur - slower
  }
  lastX = lastY = -1; // invalidate last position
}
void draw2DPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  // simple nearest-neighbor downscaling
  int outY = y * segRows / gifHeight;
  int outX = x * segCols / gifWidth;
  if ((unsigned(outX) >= segCols) || (unsigned(outY) >= segRows)) return; // out of range
  if ((lastX == outX) && (lastY == outY)) return;                           // downscaling optimization: skip re-painting same pixel
  lastX = outX; lastY = outY;

  // set multiple pixels if upscaling
  // softhack007: changed loop x/y order -> minor speedup from better cache locality
  for (int j = 0; j < expandY; j++) {
    for (int i = 0; i < expandX; i++) {
      activeSeg->setPixelColorXY(outX + i, outY + j, gamma8(red), gamma8(green), gamma8(blue));
    }
  }
}

#define IMAGE_ERROR_NONE 0
#define IMAGE_ERROR_NO_NAME 1
#define IMAGE_ERROR_SEG_LIMIT 2
#define IMAGE_ERROR_UNSUPPORTED_FORMAT 3
#define IMAGE_ERROR_FILE_MISSING 4
#define IMAGE_ERROR_DECODER_ALLOC 5
#define IMAGE_ERROR_GIF_DECODE 6
#define IMAGE_ERROR_FRAME_DECODE 7
#define IMAGE_ERROR_WAITING 254
#define IMAGE_ERROR_PREV 255

// renders an image (.gif only; .bmp and .fseq to be added soon) from FS to a segment
byte renderImageToSegment(Segment &seg) {
  if (!seg.name) return IMAGE_ERROR_NO_NAME;
  // disable during effect transition, causes flickering, multiple allocations and depending on image, part of old FX remaining
  // TODO: if (seg.mode != seg.currentMode()) return IMAGE_ERROR_WAITING;
  if (activeSeg && activeSeg != &seg) return IMAGE_ERROR_SEG_LIMIT; // only one segment at a time
  activeSeg = &seg;
  segCols = activeSeg->virtualWidth();
  segRows = activeSeg->virtualHeight();
  // segLen = activeSeg->virtualLength(); // for future 1D and expand1D support

  if (strncmp(lastFilename +1, seg.name, 32) != 0) { // segment name changed, load new image
    strncpy(lastFilename +1, seg.name, 32);
    lastFilename[33] = '\0'; // make sure that lastFilename is always null-terminated
    gifDecodeFailed = false;
    size_t fnameLen = strlen(lastFilename);
    if ((fnameLen < 4) || strcmp(lastFilename + fnameLen - 4, ".gif") != 0) { // empty segment name, name too short, or name not ending in .gif
      gifDecodeFailed = true;
      USER_PRINTF("GIF decoder unsupported file: %s\n", lastFilename);
      return IMAGE_ERROR_UNSUPPORTED_FORMAT;
    }
    if (file) file.close();

    if (!openGif(lastFilename)) {
      gifDecodeFailed = true; 
      USER_PRINTF("GIF file not found: %s\n", lastFilename);
      return IMAGE_ERROR_FILE_MISSING; 
    }
    decoder.setScreenClearCallback(screenClearCallback);
    decoder.setUpdateScreenCallback(updateScreenCallback);
    decoder.setDrawPixelCallback(draw2DPixelCallback);
    decoder.setFileSeekCallback(fileSeekCallback);
    decoder.setFilePositionCallback(filePositionCallback);
    decoder.setFileReadCallback(fileReadCallback);
    decoder.setFileReadBlockCallback(fileReadBlockCallback);
    decoder.setFileSizeCallback(fileSizeCallback);
#if __cpp_exceptions // use exception handler if we can (some targets don't support exceptions)
    try {            
#endif
    decoder.alloc(); // WLEDMM this function may throw out-of memory and cause a crash
#if __cpp_exceptions
    } catch (...) {  // if we arrive here, the decoder has thrown an OOM exception
      gifDecodeFailed = true;
      errorFlag = ERR_NORAM_PX;
      USER_PRINTLN("\nGIF decoder out of memory. I'm going to shoot myself now.\n");
      return IMAGE_ERROR_DECODER_ALLOC;
    }
#endif

    DEBUG_PRINTLN(F("Starting decoding"));
    int derr = 0;
    if((derr = decoder.startDecoding()) < 0) { 
      gifDecodeFailed = true;
      USER_PRINTF("GIF Decoding error %d\n", derr);
      if ((derr == ERROR_GIF_TOO_WIDE) || (derr == ERROR_GIF_UNSUPPORTED_FEATURE) || (derr == ERROR_GIF_INVALID_PARAMETER)) 
        errorFlag = ERR_NORAM_PX;
      return IMAGE_ERROR_GIF_DECODE; 
    }
    if ((errorFlag == ERR_NORAM_PX) || (errorFlag == ERR_NORAM)) errorFlag = ERR_NONE; // success -> reset previous memory error codes
    DEBUG_PRINTLN(F("Decoding started"));
  }

  if (gifDecodeFailed) return IMAGE_ERROR_PREV;
  if (!file) { gifDecodeFailed = true; return IMAGE_ERROR_FILE_MISSING; }
  //if (!decoder) { gifDecodeFailed = true; return IMAGE_ERROR_DECODER_ALLOC; }

  // speed 0 = half speed, 128 = normal, 255 = full FX FPS
  // TODO: 0 = 4x slow, 64 = 2x slow, 128 = normal, 192 = 2x fast, 255 = 4x fast
  uint32_t wait = currentFrameDelay * 2 - seg.speed * currentFrameDelay / 128;

  // TODO consider handling this on FX level with a different frametime, but that would cause slow gifs to speed up during transitions
  if (millis() - lastFrameDisplayTime < wait) return IMAGE_ERROR_WAITING;

  decoder.getSize(&gifWidth, &gifHeight);
  // bad gif size: prevent division by zero
  if (gifWidth == 0 || gifHeight == 0) {
    gifDecodeFailed = true;
    USER_PRINTF("Invalid GIF dimensions: %dx%d\n", gifWidth, gifHeight);
    return IMAGE_ERROR_GIF_DECODE;
  }
  // softhack007: pre-calculate upscaling for speedup
  expandX = (segCols+(gifWidth-1)) / gifWidth;
  expandY = (segRows+(gifHeight-1)) / gifHeight;

  int result = decoder.decodeFrame(false);
  if (result < 0) {
    gifDecodeFailed = true;
    USER_PRINTF("GIF Frame decode failed %d\n", result);
    return IMAGE_ERROR_FRAME_DECODE;
   }

  currentFrameDelay = decoder.getFrameDelay_ms();
  unsigned long tooSlowBy = (millis() - lastFrameDisplayTime) - wait; // if last frame was longer than intended, compensate
  currentFrameDelay = tooSlowBy > currentFrameDelay ? 0 : currentFrameDelay - tooSlowBy;
  lastFrameDisplayTime = millis();

  return IMAGE_ERROR_NONE;
}

void endImagePlayback(Segment *seg) {
  DEBUG_PRINTLN(F("Image playback end called"));
  if (!activeSeg || activeSeg != seg) return;
  if (file) file.close();
  decoder.dealloc();
  gifDecodeFailed = false;
  activeSeg = nullptr;
  lastFilename[1] = '\0';
  DEBUG_PRINTLN(F("Image playback ended"));
}

#endif
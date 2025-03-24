#include <Arduino_GFX_Library.h> //tested with version 1.5.5
#include <Adafruit_FT6206.h>     //tested with version 1.1.0
#include <Adafruit_CST8XX.h>     //"tested" with version 1.1.1 (I don't have the touchscreen display)
#include <AnimatedGIF.h> //tested with version 2.2.0

#include "turbojorb90.h" //converted via https://github.com/bitbank2/image_to_c.git
#define JORB_WIDTH 240
#define JORB_HEIGHT 240

#define SCALE 3 //scale 3 to turn 240x240 into 720x720 for the 4" round display; use a scale of 2 with the 480x480 displays

#define DISPLAY_WIDTH SCALE*JORB_WIDTH
#define DISPLAY_HEIGHT SCALE*JORB_HEIGHT

Arduino_XCA9554SWSPI *expander = new Arduino_XCA9554SWSPI(
    PCA_TFT_RESET, PCA_TFT_CS, PCA_TFT_SCK, PCA_TFT_MOSI,
    &Wire, 0x3F);
    
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    TFT_DE, TFT_VSYNC, TFT_HSYNC, TFT_PCLK,
    TFT_R1, TFT_R2, TFT_R3, TFT_R4, TFT_R5,
    TFT_G0, TFT_G1, TFT_G2, TFT_G3, TFT_G4, TFT_G5,
    TFT_B1, TFT_B2, TFT_B3, TFT_B4, TFT_B5,
//    1 /* hsync_polarity */, 50 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */, //use this row with everything but the 4" round display
//    1 /* vsync_polarity */, 16 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 18 /* vsync_back_porch */  //use this row with everything but the 4" round display
    1 /* hync_polarity */, 46 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,    //use this row with the 4" round display
    1 /* vsync_polarity */, 50 /* vsync_front_porch */, 16 /* vsync_pulse_width */, 16 /* vsync_back_porch */   //use this row with the 4" round display
//    ,1, 30000000
    );

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
// 2.1" 480x480 round display
//    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
//    expander, GFX_NOT_DEFINED /* RST */, TL021WVC02_init_operations, sizeof(TL021WVC02_init_operations));

// 2.8" 480x480 round display
//    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
//    expander, GFX_NOT_DEFINED /* RST */, TL028WVC01_init_operations, sizeof(TL028WVC01_init_operations));

// 3.4" 480x480 square display
//    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
//    expander, GFX_NOT_DEFINED /* RST */, tl034wvs05_b1477a_init_operations, sizeof(tl034wvs05_b1477a_init_operations));

// 3.2" 320x820 rectangle bar display
//    320 /* width */, 820 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
//    expander, GFX_NOT_DEFINED /* RST */, tl032fwv01_init_operations, sizeof(tl032fwv01_init_operations));

// 3.7" 240x960 rectangle bar display
//    240 /* width */, 960 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
//    expander, GFX_NOT_DEFINED /* RST */, HD371001C40_init_operations, sizeof(HD371001C40_init_operations), 120 /* col_offset1 */);

// 4.0" 720x720 square display
//    720 /* width */, 720 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
//    expander, GFX_NOT_DEFINED /* RST */, NULL, 0);

// 4.0" 720x720 round display
    720 /* width */, 720 /* height */, rgbpanel, 0 /* rotation */, false /* auto_flush */,
    expander, GFX_NOT_DEFINED /* RST */, hd40015c40_init_operations, sizeof(hd40015c40_init_operations));
// needs also the rgbpanel to have these pulse/sync values:
//    1 /* hync_polarity */, 46 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
//    1 /* vsync_polarity */, 50 /* vsync_front_porch */, 16 /* vsync_pulse_width */, 16 /* vsync_back_porch */

AnimatedGIF gif;
uint8_t *pTurboBuffer;
uint8_t *pFrameBuffer;
uint16_t *pRenderBuffer;

void setup() {
  Serial.begin(115200);
#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif
  
  Wire.setClock(1000000); // speed up I2C 
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  Serial.println("Initialized!!");
  gfx->fillScreen(BLACK);
  expander->pinMode(PCA_TFT_BACKLIGHT, OUTPUT);
  expander->digitalWrite(PCA_TFT_BACKLIGHT, HIGH); 
  gfx->enableRoundMode();

  gif.begin(GIF_PALETTE_RGB565_LE);
  pTurboBuffer = (uint8_t *)heap_caps_malloc(TURBO_BUFFER_SIZE + (JORB_WIDTH*JORB_HEIGHT), MALLOC_CAP_8BIT);
  pFrameBuffer = (uint8_t *)heap_caps_malloc(JORB_WIDTH*JORB_HEIGHT*sizeof(uint16_t), MALLOC_CAP_8BIT); // this is for the full frame RGB565 pixels
  
  //I didn't figure out the gif raw frame mode, so I take one row at a time in the bufferAndFlush callback function and put it here.
  //Plus, I need to upscale each frame for my 720x720 display anyway
  pRenderBuffer = (uint16_t*)heap_caps_malloc(DISPLAY_WIDTH*DISPLAY_HEIGHT*sizeof(uint16_t), MALLOC_CAP_8BIT);
}

void bufferAndFlush(GIFDRAW *pDraw)
{
    uint16_t* pixels = (uint16_t*)pDraw->pPixels;
    int x;
    int y = pDraw->iY + pDraw->y; // current line
    if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || pDraw->iWidth < 1)
       return; 
    for (x=0; x<pDraw->iWidth; x++)
    {
      //copy one pixel
      pRenderBuffer[(SCALE*y+0)*DISPLAY_HEIGHT+SCALE*x+0] = pixels[x]; 
      #if SCALE > 1   
      //expand to a 2x2 grid                                       
      pRenderBuffer[(SCALE*y+0)*DISPLAY_HEIGHT+SCALE*x+1] = pixels[x];
      pRenderBuffer[(SCALE*y+1)*DISPLAY_HEIGHT+SCALE*x+0] = pixels[x];
      pRenderBuffer[(SCALE*y+1)*DISPLAY_HEIGHT+SCALE*x+1] = pixels[x];
      #endif
      #if SCALE > 2
      //further expand to a 3x3 grid
      pRenderBuffer[(SCALE*y+0)*DISPLAY_HEIGHT+SCALE*x+2] = pixels[x];
      pRenderBuffer[(SCALE*y+1)*DISPLAY_HEIGHT+SCALE*x+2] = pixels[x];
      pRenderBuffer[(SCALE*y+2)*DISPLAY_HEIGHT+SCALE*x+0] = pixels[x];
      pRenderBuffer[(SCALE*y+2)*DISPLAY_HEIGHT+SCALE*x+1] = pixels[x];
      pRenderBuffer[(SCALE*y+2)*DISPLAY_HEIGHT+SCALE*x+2] = pixels[x];
      #endif
    }
    if (y == JORB_HEIGHT-1) //we just got the last row of pixels for this frame, yeet it to the display
    {
      gfx->draw16bitRGBBitmap(0, 0, pRenderBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }
}

void loop() {
  if (gif.openFLASH((uint8_t *)turbojorb90, sizeof(turbojorb90), bufferAndFlush))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d, scale = %d\n", gif.getCanvasWidth(), gif.getCanvasHeight(), SCALE);
    gif.setFrameBuf(pFrameBuffer); // for Turbo+cooked, we need to supply a full sized output framebuffer
    gif.setTurboBuf(pTurboBuffer); // Set this before calling playFrame()
    gif.setDrawType(GIF_DRAW_COOKED); // We want the library to generate ready-made 16-bit pixels
    while (gif.playFrame(false, NULL))
    {}
    gif.close();
  }
  else
  {
    Serial.printf("gif.open failed\n");
  }
}
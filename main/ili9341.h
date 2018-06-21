#ifndef ILI9341_H
#define ILI9341_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"

#define PIN_CLK 33
#define PIN_MOSI 32
#define PIN_MISO  -1 //not used
#define PIN_RST 23
#define PIN_DC 21
//#define PIN_BCKL

//To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many. More means more memory use,
//but less overhead for setting up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

#define ILI9341_INVON 0x21
#define ILI9341_INVOFF 0x20
#define ILI9341_PAGESET 0x2B
#define ILI9341_COLSET 0x2A
#define ILI9341_MEMWR 0x2C

#define LCD_HEIGHT 240
#define LCD_WIDTH 320


typedef struct {
  uint8_t cmd;
  uint8_t data[16];
  uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} color_t;

spi_device_handle_t spi;

typedef struct { // Data stored PER GLYPH
	uint16_t bitmapOffset;     // Pointer into GFXfont->bitmap
	uint8_t  width, height;    // Bitmap dimensions in pixels
	uint8_t  xAdvance;         // Distance to advance cursor (x axis)
	int8_t   xOffset, yOffset; // Dist from cursor pos to UL corner
} GFXglyph;

typedef struct { // Data stored for FONT AS A WHOLE:
	uint8_t  *bitmap;      // Glyph bitmaps, concatenated
	GFXglyph *glyph;       // Glyph array
	uint8_t   first, last; // ASCII extents
	uint8_t   yAdvance;    // Newline distance (y axis)
} GFXfont;

#include "FreeMono9pt7b.h"

static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t textsize;
static GFXfont *gfxFont = &FreeMono9pt7b;
static bool wrap = 0;
static uint16_t textcolor;
static uint16_t textbgcolor;

DRAM_ATTR static const lcd_init_cmd_t lcd_init_cmds[] = {
    /* Power contorl B, power control = 0, DC_ENA = 1 */
    {0xCF, {0x00, 0x83, 0X30}, 3},
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
    {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
    /* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
    {0xE8, {0x85, 0x01, 0x79}, 3},
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    /* Pump ratio control, DDVDH=2xVCl */
    {0xF7, {0x20}, 1},
    /* Driver timing control, all=0 unit */
    {0xEA, {0x00, 0x00}, 2},
    /* Power control 1, GVDD=4.75V */
    {0xC0, {0x26}, 1},
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    {0xC1, {0x11}, 1},
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    {0xC5, {0x35, 0x3E}, 2},
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    {0xC7, {0xBE}, 1},
    /* Memory access contorl, MX=MY=1, MV=0, ML=0, BGR=1, MH=0 */
    {0x36, {0xE8}, 1},
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Frame rate control, f=fosc, 70Hz fps */
    {0xB1, {0x00, 0x1B}, 2},
    /* Enable 3G, disabled */
    {0xF2, {0x08}, 1},
    /* Gamma set, curve 1 */
    {0x26, {0x01}, 1},
    /* Positive gamma correction */
    {0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
    /* Negative gamma correction */
    {0XE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
    /* Column address set, SC=0, EC=0xEF */
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    /* Page address set, SP=0, EP=0x013F */
    {0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
    /* Memory write */
    {0x2C, {0}, 0},
    /* Entry mode set, Low vol detect disabled, normal display */
    {0xB7, {0x07}, 1},
    /* Display function control */
    {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
    /* Sleep out */
    {0x11, {0}, 0x80},
    /* Display on */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff},
};

void lcd_cmd(const uint8_t cmd);
void lcd_data(const uint8_t *data, int len);
void lcd_spi_pre_transfer_callback(spi_transaction_t *t);
void lcd_init();

uint16_t color_to_uint(color_t color);
uint16_t bgr_to_uint(uint8_t b, uint8_t g, uint8_t r);

void invert_display(bool inv);
static void send_lines(int ypos, uint16_t *linedata);
static void send_line_finish();
void lcd_fill(uint16_t color);
void drawPixel(int x0, int y0, uint16_t color);
void drawLine(int x0, int x1, int y0, int y1, uint16_t color);
void drawFastVLine(int x, int y, int h, uint16_t color);
void drawFastHLine(int x, int y, int w, uint16_t color);
void fillRect(int x, int y, int w, int h, uint16_t color);
void drawCircle(int x0, int y0, int r, uint16_t color);
void drawCircleHelper( int x0, int y0, int r, uint8_t cornername, uint16_t color);
void fillCircleHelper(int x0, int y0, int r, uint8_t cornername, int delta, uint16_t color);
void fillCircle(int x0, int y0, int r, uint16_t color);
void drawRect(int x, int y, int w, int h, uint16_t color);
void drawRoundRect(int x, int y, int w, int h, int r, uint16_t color);
void fillRoundRect(int x, int y, int w, int h, int r, uint16_t color);
void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void drawBitmap_1bit(int x, int y, const uint8_t bitmap[], int w, int h, uint16_t color);
void drawRGBBitmap(int x, int y, uint16_t *bitmap, int w, int h);
void drawChar(int x, int y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
void setCursor(int x, int y);
void setTextsize(int size);
void setTextcolor(uint16_t c);
void setTextBgcolor(uint16_t c);
void setTextwrap(bool w);
void writeChar(char c);
void writeString(char *str);

#endif

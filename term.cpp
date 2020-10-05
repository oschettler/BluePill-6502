/*
 * Simple terminal for the M5Stack, supporting
 * - 320x240 pixel display
 * - CardKB keyboard
 * 
 * Original header:
 * 
    This sketch implements a simple serial receive terminal
    program for monitoring serial debug messages from another
    board.
    
    Connect GND to target board GND
    Connect RX line to TX line of target board
    Make sure the target and terminal have the same baud rate
    and serial stettings!
    
    The sketch works with the ILI9341 TFT 240x320 display and
    the called up libraries.
    
    The sketch uses the hardware scrolling feature of the
    display. Modification of this sketch may lead to problems
    unless the ILI9341 data sheet has been understood!
    
    Updated by Bodmer 21/12/16 for TFT_eSPI library:
    https://github.com/Bodmer/TFT_eSPI
    
    BSD license applies, all text above must be included in any
    redistribution
 */
#include <Wire.h>
#include <M5Stack.h>

#define CARDKB_ADDR 0x5F

// The scrolling area must be a integral multiple of TEXT_HEIGHT
#define TEXT_HEIGHT 16 // Height of text to be printed and scrolled
#define TOP_FIXED_AREA 14 // Number of lines in top fixed area (lines counted from top of screen)
#define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
#define YMAX 240 // Bottom of screen area

// The initial y coordinate of the top of the scrolling area
uint16_t yStart = 0;
// yArea must be a integral multiple of TEXT_HEIGHT
uint16_t yArea = YMAX-TOP_FIXED_AREA-BOT_FIXED_AREA;
// The initial y coordinate of the top of the bottom text line
// uint16_t yDraw = YMAX - BOT_FIXED_AREA - TEXT_HEIGHT;
uint16_t yDraw = 0;

// Keep track of the drawing x coordinate
uint16_t xPos = 0;

// For the byte we read from the serial port
byte data = 0;

// A few test variables used during debugging
boolean change_colour = 1;
boolean selected = 1;

// We have to blank the top line each time the display is scrolled, but this takes up to 13 milliseconds
// for a full width line, meanwhile the serial buffer may be filling... and overflowing
// We can speed up scrolling of short text lines by just blanking the character we drew
int blank[19]; // We keep all the strings pixel lengths to optimise the speed of the top line blanking

// ##############################################################################################
// Setup the vertical scrolling start address pointer
// ##############################################################################################
void scrollAddress(uint16_t vsp) {
  M5.Lcd.writecommand(ILI9341_VSCRSADD); // Vertical scrolling pointer
  M5.Lcd.writedata(vsp>>8);
  M5.Lcd.writedata(vsp);
}

// ##############################################################################################
// Call this function to scroll the display one text line
// ##############################################################################################
int scroll_line() {
  int yTemp = yStart; // Store the old yStart, this is where we draw the next line
  // Use the record of line lengths to optimise the rectangle size we need to erase the top line
  // M5.Lcd.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT],TEXT_HEIGHT, TFT_BLACK);
  M5.Lcd.fillRect(0,yStart,320,TEXT_HEIGHT, TFT_BLACK);

  // Change the top of the scroll area
  yStart+=TEXT_HEIGHT;
  // The value must wrap around as the screen memory is a circular buffer
  // if (yStart >= YMAX - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - YMAX + BOT_FIXED_AREA);
  if (yStart >= YMAX) yStart = 0;
  // Now we can scroll the display
  scrollAddress(yStart);
  return  yTemp;
}

// ##############################################################################################
// Setup a portion of the screen for vertical scrolling
// ##############################################################################################
// We are using a hardware feature of the display, so we can only scroll in portrait orientation
void setupScrollArea(uint16_t tfa, uint16_t bfa) {
  M5.Lcd.writecommand(ILI9341_VSCRDEF); // Vertical scroll definition
  M5.Lcd.writedata(tfa >> 8);           // Top Fixed Area line count
  M5.Lcd.writedata(tfa);
  M5.Lcd.writedata((YMAX-tfa-bfa)>>8);  // Vertical Scrolling Area line count
  M5.Lcd.writedata(YMAX-tfa-bfa);
  M5.Lcd.writedata(bfa >> 8);           // Bottom Fixed Area line count
  M5.Lcd.writedata(bfa);
}

void
term_init() {
  M5.begin();
  M5.Power.begin();

  Wire.begin();
  pinMode(5, INPUT);
  digitalWrite(5, HIGH);

  // M5.Lcd.setRotation(5); // Must be setRotation(0) for this sketch to work correctly
  M5.Lcd.fillScreen(TFT_BLACK);
  
  // Setup baud rate and draw top banner
  // Serial.begin(115200);
  
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
  M5.Lcd.fillRect(0,0,320,TEXT_HEIGHT, TFT_BLUE);
  M5.Lcd.drawCentreString(" Basic Computer - 115200 baud ",320/2,0,2);

  // Change colour for scrolling zone text
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  // Setup scroll area
  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
  //setupScrollArea(0, 0);

  // Zero the array
  for (byte i = 0; i<18; i++) blank[i]=0;
}

void
term_write(byte data) {
  if (data == '\r' || xPos>311) {
    xPos = 0;
    yDraw = scroll_line(); // It can take 13ms to scroll and blank 16 pixel lines
  }
  if (data > 31 && data < 128) {
    xPos += M5.Lcd.drawChar(data,xPos,yDraw,2);
    // blank[(18+(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT)%19]=xPos; // Keep a record of line lengths
  }
}

int
term_available() {
  Wire.requestFrom(CARDKB_ADDR, 1);
  return Wire.available();
}

char
term_read() {
  return Wire.read();
}

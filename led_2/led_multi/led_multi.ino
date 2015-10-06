//#include <PololuLedStrip.h>

/* This is AVR code for driving the RGB LED strips from Pololu.
   It allows complete control over the color of an arbitrary number of LEDs.
   This implementation disables interrupts while it does bit-banging with inline assembly.
 */

/* This line specifies the frequency your AVR is running at.
   This code supports 20 MHz and 16 MHz. */
#define F_CPU 16000000

// These lines specify what pin the LED strip is on.
// You will either need to attach the LED strip's data line to PC0 or change these
// lines to specify a different pin.
#define LED_STRIP_PORT PORTD
#define LED_STRIP_DDR  DDRD
#define LED_STRIP_PIN  7

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>

/** The rgb_color struct represents the color for an 8-bit RGB LED.
    Examples:
      Black:      (rgb_color){ 0, 0, 0 }
      Pure red:   (rgb_color){ 255, 0, 0 }
      Pure green: (rgb_color){ 0, 255, 0 }
      Pure blue:  (rgb_color){ 0, 0, 255 }
      White:      (rgb_color){ 255, 255, 255} */
typedef struct rgb_color
{
  unsigned char red, green, blue;
} rgb_color;

/** led_strip_write sends a series of colors to the LED strip, updating the LEDs.
 The colors parameter should point to an array of rgb_color structs that hold the colors to send.
 The count parameter is the number of colors to send.
ff
 This function takes less than 2 ms to update 30 LEDs.
 Interrupts must be disabled during that time, so any interrupt-based library
 can be negatively affected by this function.
 
 Timing details at 20 MHz (the numbers slightly different at 16 MHz):
  0 pulse  = 700 ns
  1 pulse  = 1300 ns
  "period" = 2500 ns
 */
void __attribute__((noinline)) led_strip_write(rgb_color * colors, unsigned int count) 
{
  // Set the pin to be an output driving low.
  LED_STRIP_PORT &= ~(1<<LED_STRIP_PIN);
  LED_STRIP_DDR |= (1<<LED_STRIP_PIN);

  cli();   // Disable interrupts temporarily because we don't want our pulse timing to be messed up.
  while(count--)
  {
    // Send a color to the LED strip.
    // The assembly below also increments the 'colors' pointer,
    // it will be pointing to the next color at the end of this loop.
    asm volatile(
        "rcall send_led_strip_byte%=\n"  // Send red component.
        "rcall send_led_strip_byte%=\n"  // Send green component.
        "rcall send_led_strip_byte%=\n"  // Send blue component.
        "rjmp led_strip_asm_end%=\n"     // Jump past the assembly subroutines.

        // send_led_strip_byte subroutine:  Sends a byte to the LED strip.
        "send_led_strip_byte%=:\n"
        "ld __tmp_reg__, %a0+\n"        // Read the next color brightness byte and advance the pointer
        "rcall send_led_strip_bit%=\n"  // Send most-significant bit (bit 7).
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"
        "rcall send_led_strip_bit%=\n"  // Send least-significant bit (bit 0).
        "ret\n"

        // send_led_strip_bit subroutine:  Sends single bit to the LED strip by driving the data line
        // high for some time.  The amount of time the line is high depends on whether the bit is 0 or 1,
        // but this function always takes the same time (2 us).
        "send_led_strip_bit%=:\n"
        "sbi %2, %3\n"                           // Drive the line high.
        "rol __tmp_reg__\n"                      // Rotate left through carry.

#if F_CPU == 16000000
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "nop\n" "nop\n"
#elif F_CPU == 20000000
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
#else
#error "Unsupported F_CPU"
#endif

        "brcs .+2\n" "cbi %2, %3\n"              // If the bit to send is 0, drive the line low now.    

#if F_CPU == 16000000
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "nop\n" "nop\n" "nop\n"
#elif F_CPU == 20000000
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
#endif

        "brcc .+2\n" "cbi %2, %3\n"              // If the bit to send is 1, drive the line low now.

#if F_CPU == 16000000
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "nop\n" "nop\n" "nop\n"
#elif F_CPU == 20000000
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
        "nop\n" "nop\n" "nop\n" "nop\n" "nop\n"
#endif

        "ret\n"
        "led_strip_asm_end%=: "
        : "=b" (colors)
        : "0" (colors),         // %a0 points to the next color to display
          "I" (_SFR_IO_ADDR(LED_STRIP_PORT)),   // %2 is the port register (e.g. PORTC)
          "I" (LED_STRIP_PIN)     // %3 is the pin number (0-8)
    );

    // Uncomment the line below to temporarily enable interrupts between each color.
    //sei(); asm volatile("nop\n"); cli();
  }
  sei();          // Re-enable interrupts now that we are done.
  _delay_us(24);  // Hold the line low for 15 microseconds to send the reset signal.
}

#include "SPI.h"
#include "Adafruit_WS2801.h"

int dataPin  = 2;    // Yellow wire on Adafruit Pixels
int clockPin = 3;    // Green wire on Adafruit Pixels
Adafruit_WS2801 strip = Adafruit_WS2801(50, dataPin, clockPin);

#define LEDA 60
#define LEDB 48
rgb_color colorsa[LEDA];
uint32_t colorsb[50];
//byte cmd[2];
int pos;
//int pos2;
//int c_r;
//int c_g;
//int c_b;
//int mode;
//byte cols[8];
//  int i;
  
void setup() {
  Serial.begin(115200);
  int i=0;
  for(i=0;i<50;i++){
    colorsb[i]=Color(0,0,0);
  }
  for(i=0;i<LEDA;i++){
    colorsa[i]=(rgb_color){0,0,0};
  }
  strip.begin();
//  mode=0;
}

void setNextA(int i){
  int r = getNextColor();
  int g = getNextColor();
  int b = getNextColor();
  colorsa[i]=(rgb_color){r,g,b};
}
void setNextB(int i){
  int r = getNextColor();
  int g = getNextColor();
  int b = getNextColor();
  colorsb[i]=Color(r,g,b);
}

int getNextColor() {
  int hi=getNextChar();
  int lo=getNextChar();
  return hi*16+lo;
}

int getNextChar(){
  while(true){
    while(!Serial.available()){}
  int c=Serial.read();
  switch(c){
        case '0':
        return 0;
        break;
        case '1':
        return 1;
        break;
        case '2':
        return 2;
        break;
        case '3':
        return 3;
        break;
        case '4':
        return 4;
        break;
        case '5':
        return 5;
        break;
        case '6':
        return 6;
        break;
        case '7':
        return 7;
        break;
        case '8':
        return 8;
        break;
        case '9':
        return 9;
        break;
        case 'A':
        return 10;
        break;
        case 'B':
        return 11;
        break;
        case 'C':
        return 12;
        break;
        case 'D':
        return 13;
        break;
        case 'E':
        return 14;
        break;
        case 'F':
        return 15;
        break;
    }
  }
}

void loop()
{
  int i;
    if(Serial.available()){
      int c=Serial.read();
        switch(c){
          case 'Y':
            for (i=0; i < LEDB; i++) {
              setNextB(i);
            }
            break;
          case 'W':
            for (i=0; i < LEDA; i++) {
              setNextA(i);
            }
            break;
        case 'X':
          int i;
          for (i=0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, colorsb[i]);
          }
          strip.show();
        break;
        case 'Z':
        led_strip_write(colorsa, LEDA);
        break;
      }
    }
}


uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}


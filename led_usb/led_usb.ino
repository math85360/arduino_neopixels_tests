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

#define LED_COUNT 60
rgb_color colors[LED_COUNT];
//byte cmd[2];
void setup() {
  Serial.begin(768000);
//  Serial.write("AT+BAUDC");
  //Serial.begin(384000);
 // cmd[8]={B0,B0};
  setall(255,255,255);
  led_strip_write(colors, LED_COUNT);
  delay(1000);
//  setall(0,0,0);
//  led_strip_write(colors, LED_COUNT);

}
int pos;
int pos2;
int c_r;
int c_g;
int c_b;
int cols[8];

void loop()
{
    if(Serial.available()){
      int c=Serial.read();
      switch(c){
        case 'S':
        pos=-1;
        break;
        case 'U':
        setcolrgb(cols[0],cols[1],cols[2]*16+cols[3],cols[4]*16+cols[5],cols[6]*16+cols[7]);
        break;
        case 'V':
        setpartrgb(cols[1],10,cols[2]*16+cols[3],cols[4]*16+cols[5],cols[6]*16+cols[7]);
        break;
        case 'W':
        setrgbparts(cols[0]*16+cols[1],cols[2]*16+cols[3],cols[4]*16+cols[5]);
        break;
        case 'X':
        colors[cols[0]*16+cols[1]]=(rgb_color){cols[2]*16+cols[3],cols[4]*16+cols[5],cols[6]*16+cols[7]};
        break;
        case 'Y':
        setall(cols[0]*16+cols[1],cols[2]*16+cols[3],cols[4]*16+cols[5]);
        break;
        case 'Z':
        led_strip_write(colors, LED_COUNT);
        break;
        case '0':
        cols[pos]=0;
        break;
        case '1':
        cols[pos]=1;
        break;
        case '2':
        cols[pos]=2;
        break;
        case '3':
        cols[pos]=3;
        break;
        case '4':
        cols[pos]=4;
        break;
        case '5':
        cols[pos]=5;
        break;
        case '6':
        cols[pos]=6;
        break;
        case '7':
        cols[pos]=7;
        break;
        case '8':
        cols[pos]=8;
        break;
        case '9':
        cols[pos]=9;
        break;
        case 'A':
        cols[pos]=10;
        break;
        case 'B':
        cols[pos]=11;
        break;
        case 'C':
        cols[pos]=12;
        break;
        case 'D':
        cols[pos]=13;
        break;
        case 'E':
        cols[pos]=14;
        break;
        case 'F':
        cols[pos]=15;
        break;
        default:
        pos-=1;
      }
      pos+=1;
      if(pos>8) {
        pos=8;
      }
      
//      colors[0]=(rgb_color){255, 0, 0};
//      led_strip_write(colors, LED_COUNT);
    }
}

void setall(int r, int g, int b){
   rgb_color v=(rgb_color){r, g, b};
   for(int i=0;i<LED_COUNT;i++){
    colors[i]=v;
  } 
}
void setpartrgb(int part,int sizepart,int r, int g, int b){
  rgb_color v=(rgb_color){r, g, b};
  int k=part*sizepart+sizepart;
  for(int i=part*sizepart;i<k;i++){
      colors[i]=v;
  }
}
void setrgbparts(int r, int g, int b){
//int  k=LED_COUNT/6;
  setpartrgb(0,10,r,0,0);
  setpartrgb(1,10,0,g,0);
  setpartrgb(2,10,0,0,b);
  setpartrgb(3,10,0,0,b);
  setpartrgb(4,10,0,g,0);
  setpartrgb(5,10,r,0,0);
}

void setcolrgb(int mode, int p,int r, int g, int b){
  rgb_color black=(rgb_color){0,0,0};
  rgb_color v=(rgb_color){r, g, b};
  if(mode==0){
    for(int i=0;i<LED_COUNT/2;i++){
      if(i<p){
        colors[i]=v;
        colors[LED_COUNT-i-1]=v;
      }else{
        colors[i]=black;
        colors[LED_COUNT-i-1]=black;
      }
    }
  }
  if(mode==1){
    for(int i=0;i<LED_COUNT/4;i++){
      if(i<p){
        colors[14-i]=v;
        colors[14+i]=v;
        colors[44-i]=v;
        colors[44+i]=v;
      }else{
        colors[14-i]=black;
        colors[14+i]=black;
        colors[44-i]=black;
        colors[44+i]=black;
      }
    }
  }
}

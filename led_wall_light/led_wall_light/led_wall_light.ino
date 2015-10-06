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
  Serial.begin(9600);
  setall(255,255,255);
  led_strip_write(colors, LED_COUNT);
  delay(500);
  setall(0,0,0);
  led_strip_write(colors, LED_COUNT);
  randomSeed(analogRead(0));
  initzones();
}
int pos;
int pos2;
int c_r;
int c_g;
int c_b;
int cols[8];
int t;
int zones[LED_COUNT];

void loop()
{
  digitalWrite(11, LOW);
  digitalWrite(10, HIGH);
  if(digitalRead(12)==HIGH){
    fire();
  }else{
    ocean();
  }
}

void initzones(){
  int i;
  for(i=0;i<LED_COUNT;i++){
    zones[i]=random(0,256);
  }
}

void ocean() {
    int i, r, g, b;
    float fb, fg;
    for(i=0;i<LED_COUNT;i++){
//      colors[i]=(rgb_color){0, int(256 * sin(i+t)) % 256, int(256 * cos(i+t)) % 256};
fb = (i+t);
fb=fb/30.0;
b=int(192+63 * cos(fb));
//Serial.print((b));
//Serial.print(" ");
fg = t;
fg = fg /7.0;
fg+=i;
fg=fg/8.0;
//fg=0;
fg=sin(fg);
//fg=fg*fg*fg;
if(fg<0.5){
  fg=0;
}else{
  fg=(fg-0.5)*2;
}
g=int(255 * fg)*b/255;
//b=0;
//if(sin(fg/2)<0){
  //g=0;
//}
//g=g*b/256;
//zones[i]++;
//if(zones[i]==256) zones[i]=0;
//b=256;
//if(zones[i]<128){
  //g=2*zones[i]*b/255;
//}else{
  //g=0;
//}
//b=0;
//g=random(0,b);
//if(g>b) g= b;
//g=0;
//r=0;
//b=i+t;
      colors[i]=(rgb_color){r, g, b};
}
//Serial.print("\n");
    t=t+1;
    led_strip_write(colors, LED_COUNT);
    delay(10);
}
void fire() {
      int i;
    led_strip_write(colors, LED_COUNT);
    for(i=0;i<LED_COUNT;i++){
float fb = (i+t);
fb=fb/4.0;
float fg = (i+t);
fg=fg;
//      colors[i]=(rgb_color){0, int(127+128 * sin(fg)) % 256, int(175+80 * cos(fb)) % 256};
//      colors[i]=(rgb_color){int(223+32* cos(fb)) % 256, int(63+32*cos(fb)+120 * sin(fg)) % 256, 0};
int power = 223+32*cos(fb);
int color = random(0,256);
int r = 255;
int g = color;
      colors[i]=(rgb_color){power * r/256,power * g /224,0};
    }
    t=t+1;
    delay(10);
}


void setall(int r, int g, int b){
   rgb_color v=(rgb_color){r, g, b};
   for(int i=0;i<LED_COUNT;i++){
    colors[i]=v;
  } 
}

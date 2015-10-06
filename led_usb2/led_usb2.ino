#include "SPI.h"
#include "Adafruit_WS2801.h"

/*****************************************************************************
Example sketch for driving Adafruit WS2801 pixels!


  Designed specifically to work with the Adafruit RGB Pixels!
  12mm Bullet shape ----> https://www.adafruit.com/products/322
  12mm Flat shape   ----> https://www.adafruit.com/products/738
  36mm Square shape ----> https://www.adafruit.com/products/683

  These pixels use SPI to transmit the color data, and have built in
  high speed PWM drivers for 24 bit color per pixel
  2 pins are required to interface

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution

*****************************************************************************/

// Choose which 2 pins you will use for output.
// Can be any valid output pins.
// The colors of the wires may be totally different so
// BE SURE TO CHECK YOUR PIXELS TO SEE WHICH WIRES TO USE!
int dataPin  = 2;    // Yellow wire on Adafruit Pixels
int clockPin = 3;    // Green wire on Adafruit Pixels

// Don't forget to connect the ground wire to Arduino ground,
// and the +5V wire to a +5V supply

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
Adafruit_WS2801 strip = Adafruit_WS2801(50, dataPin, clockPin);

// Optional: leave off pin numbers to use hardware SPI
// (pinout is then specific to each board and can't be changed)
//Adafruit_WS2801 strip = Adafruit_WS2801(25);

// For 36mm LED pixels: these pixels internally represent color in a
// different format.  Either of the above constructors can accept an
// optional extra parameter: WS2801_RGB is 'conventional' RGB order
// WS2801_GRB is the GRB order required by the 36mm pixels.  Other
// than this parameter, your code does not need to do anything different;
// the library will handle the format change.  Examples:
//Adafruit_WS2801 strip = Adafruit_WS2801(25, dataPin, clockPin, WS2801_GRB);
//Adafruit_WS2801 strip = Adafruit_WS2801(25, WS2801_GRB);



/*void loop() {
  // Some example procedures showing how to display to the pixels
  
  colorWipe(Color(255, 0, 0), 50);
  colorWipe(Color(0, 255, 0), 50);
  colorWipe(Color(0, 0, 255), 50);
  rainbow(20);
  rainbowCycle(20);
}*/

/*void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 256; j++) {     // 3 cycles of all 256 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 255));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}*/

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
/*void rainbowCycle(uint8_t wait) {
  int i, j;
  
  for (j=0; j < 256 * 5; j++) {     // 5 cycles of all 25 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 256 / strip.numPixels()) + j) % 256) );
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}*/
/*
// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint8_t wait) {
  int i;
  
  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}*/

/* Helper functions */
byte relays[5];
int relay0 = 14;
int relay1 = 15;
int relay2 = 16;
int relay3 = 17;
int relay4 = 11;
// Create a 24 bit color value from R,G,B
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

void showrelays() {
  digitalWrite(relay0, relays[0]==0 ? 0 : 1);
  digitalWrite(relay1, relays[1]==0 ? 0 : 1);
  digitalWrite(relay2, relays[2]==0 ? 0 : 1);
  digitalWrite(relay3, relays[3]==0 ? 0 : 1);
  analogWrite(relay4, relays[4]);
}

void led_strip_write(uint32_t * cols, int numb){
  showrelays();
  int i;
  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, cols[i]);
  }
  strip.show();
//  delay(20);
}

#define LED_COUNT 48
uint32_t colors[50];


//byte cmd[2];
void setup() {
  Serial.begin(768000);
  int i=0;
  withrelay(255,255,255,255,255);
  pinMode(relay0, OUTPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(11, OUTPUT);
  showrelays();
  for(i=0;i<50;i++){
    colors[i]=Color(0,0,0);
  }
    strip.begin();
  // Update LED contents, to start they are all 'off'
  strip.show();
//  Serial.write("AT+BAUDC");
  //Serial.begin(384000);
 // cmd[8]={B0,B0};
 for(i=254;i>=0;i-=2){
  setall(i,i,i);
  withrelay(i,i,i,i,i);
  led_strip_write(colors, LED_COUNT);
  delay(50);
}
  setall(0,0,0);
  withrelay(0,0,0,0,0);
  showrelays();
  delay(1000);
  //setall(0,0,0);
//  led_strip_write(colors, LED_COUNT);

}
int pos;
int pos2;
int c_r;
int c_g;
int c_b;
byte cols[8];
int lastcolor[3];

void loop()
{
    if(Serial.available()){
      int c=Serial.read();
      switch(c){
        case 'L':
        lastcolor[0]=cols[0]*16+cols[1];
        lastcolor[1]=cols[2]*16+cols[3];
        lastcolor[2]=cols[4]*16+cols[5];
        break;
        case 'l':
        colors[cols[0]*16+cols[1]]=Color(lastcolor[0],lastcolor[1],lastcolor[2]);
        break;
        case 'Q':
        relays[4]=cols[0]*16+cols[1];
        break;
        case 'R':
        withrelay(cols[0]*16+cols[1],cols[2]*16+cols[3],cols[4]*16+cols[5],cols[6]*16+cols[7],relays[4]+0);
        break;
        case 'S':
        pos=-1;
        break;
        case 'U':
        setcolrgb(cols[0],cols[1],cols[2]*16+cols[3],cols[4]*16+cols[5],cols[6]*16+cols[7]);
        break;
        case 'V':
        setpartrgb(cols[0],cols[1],cols[2]*16+cols[3],cols[4]*16+cols[5],cols[6]*16+cols[7]);
        break;
        case 'W':
        setrgbparts(cols[0]*16+cols[1],cols[2]*16+cols[3],cols[4]*16+cols[5]);
        break;
        case 'X':
        colors[cols[0]*16+cols[1]]=Color(cols[2]*16+cols[3],cols[4]*16+cols[5],cols[6]*16+cols[7]);
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

void setall(byte r, byte g, byte b){
   uint32_t v=Color(r, g, b);
   int i;
   for(int i=0;i<LED_COUNT;i++){
    colors[i]=v;
  } 
  withrelay(0,0,0,0,0);
}

void withrelay(byte r, byte g, byte b, byte uv, byte laser){
  relays[0]=r;
  relays[1]=g;
  relays[2]=b;
  relays[3]=uv;
  relays[4]=laser;
}

void setpartrgb(int mode,int part,byte r, byte g, byte b){
  uint32_t v=Color(r, g, b);
  if(mode==0){
    int sizepart = 8;
    int k=part*sizepart+sizepart;
    for(int i=part*sizepart;i<k;i++){
        colors[i]=v;
    }
  }else{
//    int k=part*sizepart+sizepart;
part++;
    for(int i=0;i<48;i=i+16){
        colors[i+part]=v;
        colors[i+15-part]=v;
    }
  }
}
void setrgbparts(byte r, byte g, byte b){
//int  k=LED_COUNT/6;
  setpartrgb(0,8,r,0,0);
  setpartrgb(1,8,0,g,0);
  setpartrgb(2,8,0,0,b);
  setpartrgb(3,8,0,0,b);
  setpartrgb(4,8,0,g,0);
  setpartrgb(5,8,r,0,0);
}

void setcolrgb(int mode, int p,byte r, byte g, byte b){
  uint32_t black=Color(0,0,0);
  uint32_t v=Color(r, g, b);
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
        colors[11-i]=v;
        colors[12+i]=v;
        colors[35-i]=v;
        colors[36+i]=v;
      }else{
        colors[11-i]=black;
        colors[12+i]=black;
        colors[35-i]=black;
        colors[36+i]=black;
      }
    }
  }
}

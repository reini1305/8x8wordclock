
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
//#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>
#include <Bounce2.h>

//#define PROGRAM_TIME  // Uncomment this if you want to set the time to the RTC

// define masks for each word. we add them with "bitwise or" to generate a mask for the entire "phrase".
#define MFIVE    minute_mask |= 0x0F0000       // these are in hexadecimal
#define MTEN     minute_mask |= 0x1A
#define AQUARTER minute_mask |= 0x7F01
#define TWENTY   minute_mask |= 0x7E
#define HALF     minute_mask |= 0xF00000
#define PAST     minute_mask |= 0x1E000000
#define TO       minute_mask |= 0x30000000
  
#define LED_PIN          1
#define BUTTON_PIN       3
#define NUM_LEDS        64
#define SHIFTDELAY      50 // controls color shifting speed
#define READDELAY    60000
#define DAYBRIGHTNESS   80
#define NIGHTBRIGHTNESS 20
#define NUM_MODES        8

// cutoff times for day / night brightness. feel free to modify.
#define MORNINGCUTOFF    7  // when does daybrightness begin?   7am
#define NIGHTCUTOFF     19 // when does nightbrightness begin?  7pm

Adafruit_NeoPixel matrix = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
RTC_DS3231 rtc;
Bounce debouncer = Bounce();
//                                   TWELVE,   ONE,        TWO,       THREE,  FOUR,       FIVE, SIX,   SEVEN,    EIGHT,NINE,       TEN,       ELEVEN
const uint32_t digits[12] PROGMEM = {0x6F0000, 0x92000000, 0x2030000, 0xF800, 0x0F000000, 0x0F, 0x700, 0xF00100, 0xF8, 0xF0000000, 0x808080,  0xFC0000};
const uint32_t color_black = matrix.Color(0,0,0);
uint32_t       minute_mask;
uint32_t       hour_mask;
uint8_t        shift;
boolean        shift_up;
uint8_t        hour;
uint8_t        minute;
byte           current_mode;
long unsigned  previousTimeMillis    = -READDELAY;
long unsigned  previousDisplayMillis = -SHIFTDELAY;

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code
  matrix.begin();
  matrix.setBrightness(NIGHTBRIGHTNESS);
  rtc.begin();
#ifdef PROGRAM_TIME
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
#endif
  debouncer.attach(BUTTON_PIN,INPUT_PULLUP); // Attach the dedebouncer to a pin with INPUT_PULLUP mode
  debouncer.interval(25); // Use a debounce interval of 25 milliseconds
  current_mode = EEPROM.read(0);
  shift = 0;
  shift_up = true;
}

void loop() {
#ifndef PROGRAM_TIME
  unsigned long now = millis();
  if( now - previousTimeMillis > READDELAY) { // only update the time once per minute
    previousTimeMillis = now;
    readTime();
    adjustBrightness();
    calculateMask();
  }

  // update button
  debouncer.update(); 
   
  if ( debouncer.fell() ) {
    current_mode = (current_mode + 1) % NUM_MODES;
    EEPROM.write(0,current_mode); 
  }

  // update screen
  if( now - previousDisplayMillis > SHIFTDELAY) {
    previousDisplayMillis = now;
    applyMask(minute_mask, 0);
    applyMask(hour_mask, 32);
  }
#endif
}


// ----------------- Helper Functions ---------------------------

inline void readTime(void) {
  hour   = rtc.now().hour();
  minute = rtc.now().minute() + 2;
  if (minute > 59) {
    minute -= 60;
    hour = (hour + 1) % 24;
  }
}

inline void adjustBrightness(void) {
  
  //change brightness if it's night time
  if (hour < MORNINGCUTOFF || hour >= NIGHTCUTOFF) {
    matrix.setBrightness(NIGHTBRIGHTNESS);
  } else {
    matrix.setBrightness(DAYBRIGHTNESS);
  }
}

inline void calculateMask(void) {
  // reset mask
  minute_mask = 0;
  hour_mask = 0;
  // time we display the appropriate minute counter
  if (((minute > 4) && (minute < 10)) || (minute > 54)) {
    MFIVE;
  }
  if(((minute > 9) && (minute < 15)) || ((minute > 49) && (minute < 55))) {
    MTEN;
  }
  if (((minute > 14) && (minute < 20)) || ((minute > 44) && (minute < 50))){
    AQUARTER;
  }
  if (((minute > 19) && (minute < 25)) || ((minute > 39) && (minute < 45))){
    TWENTY;
  }
  if (((minute > 24) && (minute < 30)) || ((minute > 34) && (minute < 40))){
    TWENTY;
    MFIVE;
  }
  if ((minute > 29) && (minute < 35)) {
    HALF;
  }

  // Now handle the hours
  if ((minute < 5))
  {
    hour_mask |= pgm_read_dword(&digits[hour%12]);
  }
  else if ((minute < 35) && (minute > 4))
  {
    PAST;
    hour_mask |= pgm_read_dword(&digits[hour%12]);
  }
  else
  {
    // if we are greater than 34 minutes past the hour then display
    // the next hour, as we will be displaying a 'to' sign
    TO;
    hour_mask |= pgm_read_dword(&digits[(hour + 1) % 12]);
  }
}

// show colorshift through the phrase mask. for each NeoPixel either show a color or show nothing!
void applyMask(uint32_t mask, byte offset) {

  for (byte i = offset; i < offset+32; i++) {
    if (!bitRead(mask,i-offset)) {
      matrix.setPixelColor(i, color_black);
    } else {
      uint16_t current_brightness = ((i * 4) + shift);
      uint8_t range_brightness = keepInRange(current_brightness);
      uint32_t new_color;
      switch (current_mode){ 
        case 0: // Rainbow
          new_color = Wheel(current_brightness % 256);
          break;
        case 1: // Red pulse
          new_color = matrix.Color(255, range_brightness, range_brightness);
          break;
        case 2: // Green pulse
          new_color = matrix.Color(range_brightness, 255, range_brightness);
          break;
        case 3: // Blue pulse
          new_color = matrix.Color(range_brightness, range_brightness, 255);
          break;
        case 4: // Just red
          new_color = matrix.Color(255,0,0);
          break;
        case 5: // Just green
          new_color = matrix.Color(0,255,0);
          break;
        case 6: // Just blue
          new_color = matrix.Color(0,0,255);
          break;
        default: // Just white
          new_color = matrix.Color(255,255,255);
          break;
      }
      matrix.setPixelColor(i, (new_color));
    }
  }
  matrix.show(); // show it!
  if (current_mode >= 1 && current_mode <= 3) {
    if (shift == 0)
      shift_up = true;
    if (shift == 255)
      shift_up = false;
    shift += shift_up? 1 : -1;
  } else {
    shift++;
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint8_t keepInRange(int16_t val) {
  if (val>255)
    return 255 - (val-255);
  if (val < 0)
    return -val;
  return val;
}

#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 60
#define LED_PIN D1

// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 255

#define digital_pir_sensor D0 // connect to Pin 5

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

unsigned long lastMotionTime = 0;
const unsigned long motionTimeout = 5000; // 5 seconds

const int motionCountThreshold = 2; // Number of consecutive motion detections to trigger flicker effect
int motionCount = 0;
bool meteorDirection = true; // true for right to left, false for left to right

void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue);
  }
  showStrip();
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  strip.setPixelColor(Pixel, strip.Color(red, green, blue));
}

void showStrip() {
  strip.show();
}

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay, bool rightToLeft) {  
  setAll(0,0,0);
 
  for(int i = 0; i < NUM_LEDS+NUM_LEDS; i++) {
   
   
    // fade brightness all LEDs one step
    for(int j=0; j<NUM_LEDS; j++) {
      if( (!meteorRandomDecay) || (random(10)>5) ) {
        fadeToBlack(j, meteorTrailDecay );        
      }
    }
   
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if(rightToLeft) {
        if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
          setPixel(i-j, red, green, blue);
        }
      } else {
        if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
          setPixel(NUM_LEDS-1-(i-j), red, green, blue);
        }
      }
    }
   
    showStrip();
    delay(SpeedDelay);
  }
}

void fadeToBlack(int ledNo, byte fadeValue) {
 #ifdef ADAFRUIT_NEOPIXEL_H
    // NeoPixel
    uint32_t oldColor;
    uint8_t r, g, b;
    int value;
   
    oldColor = strip.getPixelColor(ledNo);
    r = (oldColor & 0x00ff0000UL) >> 16;
    g = (oldColor & 0x0000ff00UL) >> 8;
    b = (oldColor & 0x000000ffUL);

    r=(r<=10)? 0 : (int) r-(r*fadeValue/256);
    g=(g<=10)? 0 : (int) g-(g*fadeValue/256);
    b=(b<=10)? 0 : (int) b-(b*fadeValue/256);
   
    strip.setPixelColor(ledNo, r,g,b);
 #endif
 #ifndef ADAFRUIT_NEOPIXEL_H
   // FastLED
   leds[ledNo].fadeToBlackBy( fadeValue );
 #endif  
}

void randomFlicker() {
  for(int i=0; i<NUM_LEDS; i++) {
    if(random(10) < 2) {
      setPixel(i, 255, 255, 255);
    } else {
      setPixel(i, 0, 0, 0);
    }
  }
  showStrip();
  delay(100);
}

void setup()
{
  Serial.begin(115200);  // set baud rate as 9600
  pinMode(digital_pir_sensor, INPUT); // set Pin mode as input
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);
}

void loop()
{
  bool state = digitalRead(digital_pir_sensor); // read from PIR sensor
  if (state == 1){
    Serial.println("A Motion has occured");  // When there is a response
    motionCount++;
    if(motionCount >= motionCountThreshold) {
      // randomFlicker();
      meteorRain(0xff, 0xff, 0xff, 10, 64, true, 30, meteorDirection);
      meteorDirection = !meteorDirection;
    } else {
      // meteorRain(0xff, 0xff, 0xff, 10, 64, true, 30);
      meteorRain(0xff, 0xff, 0xff, 10, 64, true, 30, true);
    }
  }
  else{
    Serial.println("Nothing Happened");  // Far from PIR sensor
    motionCount = 0;
    setAll(0, 0, 0); // Turn off all LEDs when no motion is detected
    delay(100);
  }
}

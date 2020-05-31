#include <Adafruit_NeoPixel.h> 
#include <NewPing.h>

#ifdef SEND_STATUS_TO_CONTROLLER
#define MY_DEBUG 

#include <SPI.h>
#include <MySensors.h>
#endif

#define NEO_PIN      6 // Pin D6 du NEOPIXEL

#define TRIGGER_PIN  3  // Pin D3 pour la ping TRIGGER du module HC-SR04
#define ECHO_PIN     2  // Pin D2 pour la ping ECHO du module HC-SR04

#define NUMPIXELS      24 // Nombre de pixel sur le neopixel
#define MAX_INTESITY   20  // intensité de la luminosté (en pourcentage)

// La distance max que peut voir le module HC-SR04 est d'environ 4 mètres.
#define MAX_DISTANCE 200 // Distance à partir du moment où la voiture est detectée (en cm)
#define PANIC_DISTANCE 50 // Distance à laquelle le neopixel se met en warning (en cm)
#define PARKED_DISTANCE 60

#define PARK_OFF_TIMEOUT 15000 // temps pour que le NEOPIXEL s'éteigne après que la voiture soit garée (en miliseconde)

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.


unsigned long sendInterval = 5000;  // Send park status at maximum every 5 second.
unsigned long lastSend;

int oldParkedStatus=-1;

unsigned long blinkInterval = 0; // blink interval (milliseconds)
unsigned long lastBlinkPeriod;
bool blinkColor = true;

// To make a fading motion on the led ring/tape we only move one pixel/distDebounce time
unsigned long distDebounce = 30; 
unsigned long lastDebouncePeriod;
int numLightPixels=0;
int skipZero=0;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting distance sensor");
  pixels.begin(); // This initializes the NeoPixel library.
  Serial.println("Neopixels initialized");
}

void loop() {
  unsigned long now = millis();
  unsigned int fullDist = (sonar.ping_median() / US_ROUNDTRIP_CM);
//  Serial.println(fullDist);
  int displayDist = min(fullDist, MAX_DISTANCE);
  if (displayDist == 0 && skipZero<10) {
    // Try to filter zero readings
    skipZero++;
    return;
  }
  // Check if it is time to alter the leds
  if (now-lastDebouncePeriod > distDebounce) {
    lastDebouncePeriod = now;

    // Update parked status
    int parked = displayDist != 0 && displayDist<PARKED_DISTANCE;
    if (parked != oldParkedStatus && now-lastSend > sendInterval) {
      if (parked)
        Serial.println("Car Parked");
      else
        Serial.println("Car Gone");
      oldParkedStatus = parked;
      lastSend = now;
    }

    if (now-lastSend > PARK_OFF_TIMEOUT) {
      // We've been parked for a while now. Turn off all pixels
      for(int i=0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(0,0,0)); 
      }
    } else {
      if (displayDist == 0) {
        // No reading from sensor, assume no object found
        numLightPixels--;
      } else {
        skipZero = 0;
        int newLightPixels = NUMPIXELS - (NUMPIXELS*(displayDist-PANIC_DISTANCE)/(MAX_DISTANCE-PANIC_DISTANCE));
        if (newLightPixels>numLightPixels) {
          // Fast raise
          numLightPixels += max((newLightPixels - numLightPixels) / 2, 1);
        } else if (newLightPixels<numLightPixels) {
          // Slow decent
          numLightPixels--;
        }
      }

      if (numLightPixels>=NUMPIXELS) {
        // Do some intense red blinking 
        if (now-lastBlinkPeriod > blinkInterval) {
          blinkColor = !blinkColor;
          lastBlinkPeriod = now;
        }
        for(int i=0;i<numLightPixels;i++){
          pixels.setPixelColor(i, pixels.Color(blinkColor?255*MAX_INTESITY/100:0,0,0)); 
        }              
      } else {
        for(int i=0;i<numLightPixels;i++){
          int r = 255 * i/NUMPIXELS;
          int g = 255 - r;     
          // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
          pixels.setPixelColor(i, pixels.Color(r*MAX_INTESITY/100,g*MAX_INTESITY/100,0)); 
        }
        // Turn off the rest
        for(int i=numLightPixels;i<NUMPIXELS;i++){
          pixels.setPixelColor(i, pixels.Color(0,0,0)); 
        }
      }
    }
    pixels.show(); // This sends the updated pixel color to the hardware.
  }
}


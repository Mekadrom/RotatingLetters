#include "FastLED.h"
#include <Servo.h>

// definitions purely for use with adafruit servo driver
#define MIN_PULSE_WIDTH     650
#define MAX_PULSE_WIDTH     2350
#define DEFAULT_PULSE_WIDTH 1500
#define FREQUENCY           50

// project-specific definitions for easy changing throughout testing
#define SERVO_COUNT         11
#define DELAY               7000
#define ONE_BY_ONE_DELAY    100
#define AERONAUTICS         45  // need to make adjustments to setup so that this can be 45
#define ENGINEERING         135 // || and set this to 135, to allow for a wider range of pointing
#define ZERO        AERONAUTICS

// various state constants; values don't matter here as long as they're different
#define ROTATE_ALL          0x00
#define ROTATE_ONE_BY_ONE   0x01

#define LEFT_TO_RIGHT       0x10
#define RIGHT_TO_LEFT       0x11

/** 
 * the two colors below are kent state blue and kent state orange, as defined at
 * https://www.kent.edu/ucm/color-palettes-primary-palette
 */
#define KENT_STATE_BLUE     0x002664 // blue
#define KENT_STATE_GOLD     0xeaab00 // gold

#define DEFAULT_COLOR       0xffffff // white

// set the brightness to use (the maximum is 255 now).
#define BRIGHTNESS          127

#define LED_PER_SERVO       6
#define LED_COUNT         120

CRGB leds[LED_COUNT];

#define LED_DATA_PIN  4 // green wire is data
#define LED_CLOCK_PIN 5 // yellow wire is clock

#define LED_OFFSET          0

// safety limits for servos
#define SERVO_LOWER_LIMIT   0
#define SERVO_UPPER_LIMIT 179

//Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

Servo servos[SERVO_COUNT];

int servoPins[SERVO_COUNT] = { 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 };

int offsets[SERVO_COUNT] = { 21, 7, 17, 12, 13, 8, 6, 20, 19, 21, 19 }; // these are subject to change
int curAngles[SERVO_COUNT]; // easy access to servos' angles

long pointStartTime;

int pointStaticOffset = AERONAUTICS; 

void setup() {
  FastLED.addLeds<DOTSTAR, LED_DATA_PIN, LED_CLOCK_PIN, BGR>(leds, LED_COUNT);
  FastLED.setBrightness(BRIGHTNESS);
  
  for(int i = 0; i < SERVO_COUNT; i++) {
    servos[i].attach(servoPins[i]);
  }

  rotateAll(AERONAUTICS);

  setAllLEDS(DEFAULT_COLOR);
  
  delay(2000);
}

void loop() {
  for(int i = 0; i < SERVO_COUNT; i++) {
    setLED(i, KENT_STATE_GOLD);
    delay(100);
  }

  delay(1000);
  
  for(int i = SERVO_COUNT - 1; i >= 0; i--) {
    setLED(i, KENT_STATE_BLUE);
    delay(100);
  }

  sub(ROTATE_ONE_BY_ONE);
}

/** 
 * delegate routining; for adding more routines
 */
void sub(int subroutine) {
  switch(subroutine) {
    case ROTATE_ONE_BY_ONE: {
      rotateOneByOneColorsRoutine();
      break;
    } default: {
      rotateOneByOneColorsRoutine();
      break;
    }
  }
}

void rotateOneByOneColorsRoutine() {
  rotateAllDelayDirWithColor(AERONAUTICS, ONE_BY_ONE_DELAY, LEFT_TO_RIGHT, KENT_STATE_BLUE);
  delay(DELAY);
  rotateAllDelayDirWithColor(ENGINEERING, ONE_BY_ONE_DELAY, RIGHT_TO_LEFT, KENT_STATE_GOLD);
  delay(DELAY);
}

void rotateAll(int angle) {
  rotateAllDelay(angle, 0);
}

void rotateAllDelay(int angle, long d) {
  for(int i = 0; i < SERVO_COUNT; i++) {
    rotate(i, angle);
    delay(d);
  }
}

void rotateAllDelayDir(int angle, long d, int dirCode) {
  if(dirCode == LEFT_TO_RIGHT) {
    for(int pin = 0; pin < SERVO_COUNT; pin++) {
      rotate(pin, angle);
      delay(d);
    }
  } else {
    for(int pin = SERVO_COUNT; pin >= 0; pin--) {
      rotate(pin, angle);
      delay(d);
    }
  }
}

void rotateAllDelayDirWithColor(int angle, long d, int dirCode, long color) {
  if(dirCode == LEFT_TO_RIGHT) {
    for(int pin = 0; pin < SERVO_COUNT; pin++) {
      setServoAndLED(pin, angle, color);
      delay(d);
    }
  } else {
    for(int pin = SERVO_COUNT - 1; pin >= 0; pin--) {
      setServoAndLED(pin, angle, color);
      delay(d);
    }
  }
}

void setServoAndLED(int pin, int angle, long color) {
  rotate(pin, angle);
  setLED(pin, color);
}


void rotate(int pin, int angle) {
  angle += offsets[pin];
  if(angle >= SERVO_UPPER_LIMIT) {
    angle = SERVO_UPPER_LIMIT;
  }
  if(angle <= SERVO_LOWER_LIMIT) {
    angle = SERVO_LOWER_LIMIT;
  }
  curAngles[pin] = angle;
  servos[pin].write(angle);
}

void setAllLEDS(long color) {
  for(int i = 0; i < SERVO_COUNT; i++) {
    setLED(i, color);
  }
}

/**
 * Given a pin corresponding to a servo, lights all of the 
 */
void setLED(int pin, long color) {
  for(int i = (6 * pin) + LED_OFFSET; i < (6 * pin) + 6 + LED_OFFSET; i++) {
    setLEDAbsolute(i, color);
  }
}

void setLEDAbsolute(int led, long color) {
  byte red =   (color & 0xff0000) >> 16;
  byte green = (color & 0x00ff00) >>  8;
  byte blue =  (color & 0x0000ff) >>  0;
  
  leds[led] = CRGB(red * 1, green * 1, blue * 1);
  FastLED.show();
}


#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO

#include <APA102.h>
#include <EEPROM.h>

#include <Adafruit_PWMServoDriver.h>

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
#define KENT_STATE_ORANGE   0xeaab00 // orange

// led defines

// set the brightness to use (the maximum is 31).
#define BRIGHTNESS          2
#define LED_PER_SERVO       6
#define LED_COUNT LED_PER_SERVO * SERVO_COUNT

#define LED_DATA_PIN 11 // green wire is data
#define LED_CLOCK_PIN 12 // yellow wire is clock

#define LED_OFFSET          0

// safety limits for servos
#define SERVO_LOWER_LIMIT   0
#define SERVO_UPPER_LIMIT   179

APA102<LED_DATA_PIN, LED_CLOCK_PIN> ledStrip;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

int offsets[SERVO_COUNT] = { 21, 7, 17, 12, 13, 8, 6, 20, 19, 21, 19 }; // these are subject to change
int curAngles[SERVO_COUNT]; // easy access to servos' angles

long pointStartTime;

int pointStaticOffset = AERONAUTICS; 

void setup() {
  Serial.begin(9600);

  pwm.begin();

  pwm.setPWMFreq(FREQUENCY);

  rotateAll(AERONAUTICS);
}

void loop() {
  sub(ROTATE_ONE_BY_ONE);
}

/** 
 * delegate routining
 */
void sub(int subroutine) {
  switch(subroutine) {
    case ROTATE_ALL: {
      rotateAllRoutine();
      break;
    } case ROTATE_ONE_BY_ONE: {
      rotateOneByOneRoutine();
      break;
    } default: {
      rotateAllRoutine();
      break;
    }
  }
}

/**
 * not really used anymore, as the delayed routine was deemed a better (cooler) demonstration
 */
void rotateAllRoutine() {
  rotateAll(AERONAUTICS);
  delay(DELAY);
  rotateAll(ENGINEERING);
  delay(DELAY);
}

/**
 * default procedure; rotates each servo with a delay of ONE_BY_ONE_DELAY between each,
 * and changes the direction in between
 */
void rotateOneByOneRoutine() {
  rotateAllDelayDir(AERONAUTICS, ONE_BY_ONE_DELAY, LEFT_TO_RIGHT);
  delay(DELAY);
  rotateAllDelayDir(ENGINEERING, ONE_BY_ONE_DELAY, RIGHT_TO_LEFT);
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

void setServoAndLED(int pin, int angle, long color) {
  unsigned int red = (color & 0xff0000) >> 16;
  unsigned int green = (color & 0x00ff00) >> 8;
  unsigned int blue = (color & 0x0000ff) >> 0;

  rgb_color rgbColor(red, green, blue);

  rotate(pin, angle);
  setLED(pin, rgbColor);
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
  pwm.setPWM(pin, 0, pulseWidth(angle));
}

/**
 * Given a pin corresponding to a servo, lights all of the 
 */
void setLED(int pin, rgb_color color) {
  rgb_color* colors;
  for(int i = LED_OFFSET; i < (6 * pin) + LED_OFFSET; i++) {
    colors[i] = color;
  }
  ledStrip.write(colors, LED_COUNT, BRIGHTNESS);
}

int pulseWidth(int angle) {
  int pulse_wide, analog_value;
  pulse_wide   = map(angle, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
  analog_value = int(float(pulse_wide) / 1000000 * FREQUENCY * 4096);
  return analog_value;
}

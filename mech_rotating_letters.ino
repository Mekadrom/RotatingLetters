#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define MIN_PULSE_WIDTH     650
#define MAX_PULSE_WIDTH     2350
#define DEFAULT_PULSE_WIDTH 1500
#define FREQUENCY           50
#define SERVO_COUNT         11
#define DELAY               7000
#define ONE_BY_ONE_DELAY    100
#define AERONAUTICS         0
#define ENGINEERING         90
#define DEFAULT_COLOR       0xffffff

#define ROTATE_ALL          0x00
#define ROTATE_ONE_BY_ONE   0x01
#define POINT               0x02

#define LEFT_TO_RIGHT       0x03
#define RIGHT_TO_LEFT       0x04
 
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

boolean dormant = false;
int dormantTimer = 0;
int dormantLimit = 50000;

uint8_t offsets[SERVO_COUNT] = { 22, 8, 18, 14, 15, 10, 8, 23, 21, 21, 20 };

void setup() {
  Serial.begin(9600);
 
  pwm.begin();
  pwm.setPWMFreq(FREQUENCY);

  rotateAll(0, 0xffffff); // zeroing
}

void loop() {
  if(!dormant) {
    doSubroutine(ROTATE_ONE_BY_ONE);
  }
  dormantCheck();
}

void doSubroutine(int subroutine) {
  switch(subroutine) {
    case ROTATE_ALL: {
      rotateAllRoutine();
      break;
    } case ROTATE_ONE_BY_ONE: {
      rotateOneByOneRoutine();
      break;
    } case POINT: {
      point(0.0, 1.0);
      break;
    } default: {
      rotateAllRoutine();
      break;
    }
  }
}

void point(double centerOffset, double distance) { // distance in meters
  
}

void rotateAllRoutine() {
  rotateAll(AERONAUTICS, 0x0000ff);
  wait(DELAY);
  rotateAll(ENGINEERING, 0xecab23);
  wait(DELAY);
}

void rotateOneByOneRoutine() {
  rotateAllWithDelay(AERONAUTICS, ONE_BY_ONE_DELAY, LEFT_TO_RIGHT, 0x0000ff);
  wait(DELAY);
  rotateAllWithDelay(ENGINEERING, ONE_BY_ONE_DELAY, RIGHT_TO_LEFT, 0xecab23);
  wait(DELAY);
}

void rotateAll(int angle, long color) {
  long l[1] = { color };
  rotateAll(angle, l);
}

void rotateAll(int angle, long* colors) {
  rotateAllWithDelay(angle, 0, LEFT_TO_RIGHT, colors);
}

void rotateAllWithDelay(int angle, int delayTime, int dir, long color) {
  long l[1] = { color };
  rotateAllWithDelay(angle, delayTime, dir, l);
}

void rotateAllWithDelay(int angle, int delayTime, int dir, long* colors) {
  int numColors = sizeof(colors) / sizeof(long);
  if(dir == LEFT_TO_RIGHT) {
    for(uint8_t pin = 0; pin < SERVO_COUNT; pin++) {
      if(numColors == 1) {
        outputServoAndLED(pin, angle, colors[0]);
      } else {
        outputServoAndLED(pin, angle, colors[pin]);
      }
      wait(delayTime);
    }
  } else {
    for(uint8_t pin = SERVO_COUNT; pin > 0; pin--) {
      if(numColors == 1) {
        outputServoAndLED(pin, angle, colors[0]);
      } else {
        outputServoAndLED(pin, angle, colors[pin]);
      }
      wait(delayTime);
    }
  }
}

/**
 * Does output for the servo involved and its
 * corresponding led
 */
void outputServoAndLED(int pin, int angle, long color) {
  setAngle(pin, angle);
  setLed(pin, color);
}

void setAngle(int pin, int angle) {
  pwm.setPWM(pin, 0, pulseWidth(angle + offsets[pin]));
}

void setLed(int ledNum, long color) {
  // todo: code for setting color of led at ledNum pin
//  long colorLong = *color;
//  Color color = Color(colorLong);
}

int pulseWidth(int angle) {
  int pulse_wide, analog_value;
  pulse_wide   = map(angle, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
  analog_value = int(float(pulse_wide) / 1000000 * FREQUENCY * 4096);
  return analog_value;
}

void dormantCheck() {
  if(personPresent()) { // if person detected, reset dormantTimer
    dormantTimer = 0;
  }
  dormantTimer++;
  dormant = dormantTimer >= dormantLimit; // change dormancy depending on input
}

boolean personPresent() {
  return false; // todo: detect people
}

void wait(long microseconds) {
  delay(microseconds);
  dormantTimer += microseconds;
}

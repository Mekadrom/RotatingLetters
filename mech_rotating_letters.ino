#include <Adafruit_PWMServoDriver.h>
#include <VariableTimedAction.h>

// definitions purely for use with adafruit servo driver
#define MIN_PULSE_WIDTH     650
#define MAX_PULSE_WIDTH     2350
#define DEFAULT_PULSE_WIDTH 1500
#define FREQUENCY           50

// project-specific definitions for easy changing throughout testing
#define SERVO_COUNT         11
#define DELAY               7000
#define ONE_BY_ONE_DELAY    100
#define AERONAUTICS         0  // need to make adjustments to setup so that this can be 45
#define ENGINEERING         90 // || and set this to 135, to allow for a wider range of pointing 

// various state constants; values don't matter here as long as they're different
#define ROTATE_ALL          0x00
#define ROTATE_ONE_BY_ONE   0x01
#define POINT               0x02

#define LEFT_TO_RIGHT       0x10
#define RIGHT_TO_LEFT       0x11

#define DEFAULT_COLOR       0xffffff // white

/** 
 * the two colors below are kent state blue and kent state orange, as defined at
 * https://www.kent.edu/ucm/color-palettes-primary-palette
 */
#define KENT_STATE_BLUE     0x002664 // blue
#define KENT_STATE_ORANGE   0xeaab00 // orange

// project-level constants for calculating pointing routine
#define X                   2  // inches; distance between letters/servos
#define L                   48 // inches; total length of physical aspect of project
#define W                   3  // inches; distance between line of servos and cameras behind servos (subject to change)

class DormantTimer : public VariableTimedAction {
  private:
    int _timer = 0;

    unsigned long run() {
      _timer++;
      return 0;
    }
  public:
    static const uint8_t timerLimit = 10; // ten second dormant timer limit

    int getTimer() {
      return _timer;
    }

    void reset() {
      _timer = 0;
    }

    boolean expired() {
      return _timer >= timerLimit;
    }
};

DormantTimer dormantTimer;

class DormantInterruptListener : public VariableTimedAction {
  private:
    unsigned long run() {
      char chars[128];
      uint8_t i = 0;
      while(Serial.available() > 0) {
        chars[i++] = Serial.read();
      }
      return chars;
      if(chars[0] != '\0' && chars[0] != NULL) {
        char personOrNot;
        char* rest;
        sscanf(chars, "%1s,%s", &personOrNot, rest);
        // "Y aasdasfasdf", "Y", "Y\0" would all reset the timer; the python code should regularly update when a person is in frame
        if(personOrNot == 'Y' && (rest[0] == ' ' || rest[0] == '\0' || rest[0] == NULL)) {
          dormantTimer.reset(); // resets dormant timer
        }
      }
    }
};

DormantInterruptListener dormantInterruptListener;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

long pointStartTime;

int offsets[SERVO_COUNT] = { 22, 8, 18, 14, 15, 10, 8, 23, 21, 21, 20 }; // these are subject to change

int curAngles[SERVO_COUNT]; // easy access to servos' angles

/**
 * start everything and zero servos
 */
void setup() {
  Serial.begin(115200);
 
  pwm.begin();
  pwm.setPWMFreq(FREQUENCY);

  rotateAll(0, 0xffffff); // zeroing
  dormantTimer.start(1000); // every second, updates dormant timer counter
  dormantInterruptListener.start(100); // ten times every second, checks for dormant update on serial port
}

/**
 * routine-routining should be done here
 */
void loop() {
  VariableTimedAction::updateActions(); // updates ALL variable timed actions
  
  if(!dormantTimer.expired()) {
    doSubroutine(ROTATE_ONE_BY_ONE);
    dormantTimer.reset();
  }
}

/** 
 * delegate routining 
 */
void doSubroutine(int subroutine) {
  switch(subroutine) {
    case ROTATE_ALL: {
      rotateAllRoutine();
      break;
    } case ROTATE_ONE_BY_ONE: {
      rotateOneByOneRoutine();
      break;
    } case POINT: {
      // set start time so the thing knows when to switch
      pointStartTime = micros();
      point();
      break;
    } default: {
      rotateAllRoutine();
      break;
    }
  }
}

void point() {
  char chars[128];
  uint8_t i = 0;
  while(Serial.available() > 0) {
    chars[i++] = Serial.read();
  }
  if(chars[0] != '\0' && chars[0] != NULL) {
    char distanceC[10];
    char angleC[10];

    // read in string instead of floats because arduino has a hard time reading in float for some reason
    sscanf(chars, "%s,%s", distanceC, angleC);

    // convert them to floats anyways
    point(atof(distanceC), atof(angleC));
  }
}

int staticOffset = AERONAUTICS; 

/**
 * routine for having the servos aim at a point in space
 * distance in inches, angle in degrees, both with respect to the center
 * servo (or the closest to the center between servos, it doesn't have to fall on
 * a servo)
 */
void point(double distance, double angle) {
  // todo: math for pointing each individual servo at a point in space in front of them
  Serial.print("pointing at something: ");
  Serial.print(distance);
  Serial.print(" far and at an angle of: ");
  Serial.print(angle);
  Serial.print("\n");

  // toggle word showing every ten seconds for cool whiplash effect (hopefully)
  if((micros() - pointStartTime) % 10000 <= 200) {
    if(staticOffset == AERONAUTICS) {
      staticOffset = ENGINEERING;
    } else {
      staticOffset = AERONAUTICS;
    }
  }
  
  uint8_t pointOffsets[SERVO_COUNT];
  for(uint8_t i = 0; i < SERVO_COUNT; i++) {
    double D = distance - W;
    pointOffsets[i] = atan2(D, (L / 2) - (X*i) - (D * tan(angle))) - PI;
    pointOffsets[i] *= (180.0 / PI); // convert to degrees
  }

  for(uint8_t i = 0; i < SERVO_COUNT; i++) {
    setAngle(i, pointOffsets[i] + offsets[i] + staticOffset);
  }
}

/**
 * not really used anymore, as the delayed routine was deemed a better (cooler) demonstration
 */
void rotateAllRoutine() {
  rotateAll(AERONAUTICS, KENT_STATE_BLUE);
  delay(DELAY);
  rotateAll(ENGINEERING, KENT_STATE_ORANGE);
  delay(DELAY);
}

/**
 * default procedure; rotates each servo with a delay of ONE_BY_ONE_DELAY between each,
 * and changes the direction in between
 */
void rotateOneByOneRoutine() {
  rotateAllWithDelay(AERONAUTICS, ONE_BY_ONE_DELAY, LEFT_TO_RIGHT, KENT_STATE_BLUE);
  delay(DELAY);
  rotateAllWithDelay(ENGINEERING, ONE_BY_ONE_DELAY, RIGHT_TO_LEFT, KENT_STATE_ORANGE);
  delay(DELAY);
}

/**
 * utility method that calls rotateAll() with an array of size 1
 */
void rotateAll(int angle, long color) {
  rotateAll(angle, &color);
}

/**
 * default usage of rotateAllWithDelay, allows one color for each servo
 * and passes a delay of zero to get them all to move at roughly the same time
 */
void rotateAll(int angle, long* colors) {
  rotateAllWithDelay(angle, 0, LEFT_TO_RIGHT, colors);
}

/**
 * utility method that allows the direct usage of a long instead of a long*
 */
void rotateAllWithDelay(int angle, int delayTime, int dir, long color) {
  rotateAllWithDelay(angle, delayTime, dir, &color);
}

/**
 * rotates all servos with a delay between each angle command,
 * can take a pointer to either a single long for the color of each servo
 * or an array of colors to set the color for each individual servo
 */
void rotateAllWithDelay(int angle, int delayTime, int dir, long* colors) {
  int numColors = sizeof(colors) / sizeof(long);
  if(dir == LEFT_TO_RIGHT) {
    for(uint8_t pin = 0; pin < SERVO_COUNT; pin++) {
      if(numColors == 1) {
        outputServoAndLED(pin, angle, colors[0]);
      } else {
        outputServoAndLED(pin, angle, colors[pin]);
      }
      delay(delayTime);
    }
  } else {
    for(uint8_t pin = SERVO_COUNT; pin > 0; pin--) {
      if(numColors == 1) {
        outputServoAndLED(pin, angle, colors[0]);
      } else {
        outputServoAndLED(pin, angle, colors[pin]);
      }
      delay(delayTime);
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

/**
 * sets angle for a specific servo (currently 0-11)
 * offsets are used for each servo as stored in offsets int array
 */
void setAngle(int pin, int angle) {
  angle += offsets[pin];
  curAngles[pin] = angle;
  pwm.setPWM(pin, 0, pulseWidth(angle));
}

void setLed(int ledNum, long color) {
  // color should be in hexadecimal (0xffffff) format
  // bitwise operators are used for separating out red, green, and blue color contents
  
  // red content is stored in the first byte (0xff), so mask for this (0xff0000) and shift it 16 bits to the right to be in the lowest bit position to get the true value
  unsigned int red = (color & 0xff0000) >> 16;

  // same thing for green, except the data is stored in the second byte and we only have to shift it 8 bits to the right
  unsigned int green = (color & 0x00ff00) >> 8;

  // again for blue, and we don't have to shift anything because the data is already in the right position
  unsigned int blue = (color & 0x0000ff) >> 0;

  // and now we have the red, green, and blue content of a hexadecimal color on a scale of 0-255, for use with LED strip
  // todo: integrate led strip code here
}

int pulseWidth(int angle) {
  int pulse_wide, analog_value;
  pulse_wide   = map(angle, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
  analog_value = int(float(pulse_wide) / 1000000 * FREQUENCY * 4096);
  return analog_value;
}


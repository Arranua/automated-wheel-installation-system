//ESP32 Magazine Code
//Expected responsibilities:

//- magazine servo A
//- magazine servo B
//- PLC start input
//- wheel type selection input
//- done pulse output
//- ESTOP/abort input


#include <ESP32Servo.h>

Servo servo1;
Servo servo2;

// ==========================
// PINS
// ==========================

const int SERVO1_PIN = 16;   // wheel type A rotary servo
const int SERVO2_PIN = 17;   // wheel type B rotary servo

const int START_PIN  = 33;   // PLC start signal in
const int TYPE_PIN   = 32;   // PLC wheel type select in

const int DONE_PIN   = 25;   // cycle done signal out to PLC
const int ESTOP_PIN  = 5;    // PLC emergency stop in

// ==========================
// SERVO ANGLES
// ==========================

const int SAFE_LOW  = 15;
const int SAFE_HIGH = 170;
const int MIDDLE    = 90;

// Servos start at middle on power-up
const int SERVO1_POWERUP_ANGLE = MIDDLE;
const int SERVO2_POWERUP_ANGLE = MIDDLE;

// Each cycle goes to 15 first, then 170
const int SERVO1_CYCLE_START_ANGLE = SAFE_LOW;    // 15 deg
const int SERVO1_CYCLE_END_ANGLE   = SAFE_HIGH;   // 170 deg

const int SERVO2_CYCLE_START_ANGLE = SAFE_LOW;    // 15 deg
const int SERVO2_CYCLE_END_ANGLE   = SAFE_HIGH;   // 170 deg

// ==========================
// SPEED SETTINGS
// ==========================

const int STEP_DEGREES = 5;
const unsigned long STEP_INTERVAL_MS = 20;

// ==========================
// TIMING
// ==========================

const unsigned long DROP_HOLD_MS = 500;
const unsigned long DONE_PULSE_MS = 500;

// ==========================
// STATE
// ==========================

int servo1Angle = SERVO1_POWERUP_ANGLE;
int servo2Angle = SERVO2_POWERUP_ANGLE;

bool lastStartState = HIGH;

void setup() {
  Serial.begin(9600);

  pinMode(START_PIN, INPUT_PULLUP);
  pinMode(TYPE_PIN, INPUT_PULLUP);
  pinMode(ESTOP_PIN, INPUT_PULLUP);

  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);

  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);

  servo1.attach(SERVO1_PIN, 500, 2500);
  servo2.attach(SERVO2_PIN, 500, 2500);

  servo1.write(servo1Angle);
  servo2.write(servo2Angle);

  Serial.println();
  Serial.println("=== ESP32 MAGAZINE CONTROLLER READY ===");
  Serial.println("Both servos start at 90 degrees.");
  Serial.println("Cycle motion: selected servo goes 15 deg, then 170 deg.");
  printHelp();
}

void loop() {
  readSerialCommand();
  checkPLCStart();
}

// ==========================
// PLC START LOGIC
// ==========================

void checkPLCStart() {
  bool currentStartState = digitalRead(START_PIN);

  // Detect falling edge: HIGH -> LOW
  if (lastStartState == HIGH && currentStartState == LOW) {
    if (estopActive()) {
      Serial.println("PLC start ignored. ESTOP is active.");
    } else {
      runMagazineCycleFromPLC();
    }
  }

  lastStartState = currentStartState;
}

void runMagazineCycleFromPLC() {
  Serial.println();
  Serial.println("===== PLC MAGAZINE CYCLE START =====");

  int typeState = digitalRead(TYPE_PIN);

  if (typeState == HIGH) {
    Serial.println("TYPE_PIN = HIGH");
    Serial.println("Running Servo 1 / Wheel Type A");
    runServo1Cycle();
  } else {
    Serial.println("TYPE_PIN = LOW");
    Serial.println("Running Servo 2 / Wheel Type B");
    runServo2Cycle();
  }

  if (!estopActive()) {
    pulseDone();
    Serial.println("===== PLC MAGAZINE CYCLE DONE =====");
  } else {
    Serial.println("Cycle stopped by ESTOP. No done pulse sent.");
  }

  waitForStartRelease();
}

void waitForStartRelease() {
  Serial.println("Waiting for START signal to release...");

  while (digitalRead(START_PIN) == LOW) {
    delay(10);
  }

  lastStartState = HIGH;
  Serial.println("START released. Ready for next cycle.");
}

void pulseDone() {
  Serial.println("Sending DONE pulse...");
  digitalWrite(DONE_PIN, HIGH);
  delay(DONE_PULSE_MS);
  digitalWrite(DONE_PIN, LOW);
  Serial.println("DONE pulse complete.");
}

// ==========================
// MAGAZINE CYCLES
// ==========================

void runServo1Cycle() {
  Serial.println("Servo 1 cycle running...");

  Serial.println("Servo 1 step 1: moving to 15 degrees.");
  smoothMoveServo1(SERVO1_CYCLE_START_ANGLE);
  if (estopActive()) return;

  delayWithEstopCheck(DROP_HOLD_MS);
  if (estopActive()) return;

  Serial.println("Servo 1 step 2: moving to 170 degrees.");
  smoothMoveServo1(SERVO1_CYCLE_END_ANGLE);
  if (estopActive()) return;

  Serial.println("Servo 1 cycle complete.");
}

void runServo2Cycle() {
  Serial.println("Servo 2 cycle running...");

  Serial.println("Servo 2 step 1: moving to 15 degrees.");
  smoothMoveServo2(SERVO2_CYCLE_START_ANGLE);
  if (estopActive()) return;

  delayWithEstopCheck(DROP_HOLD_MS);
  if (estopActive()) return;

  Serial.println("Servo 2 step 2: moving to 170 degrees.");
  smoothMoveServo2(SERVO2_CYCLE_END_ANGLE);
  if (estopActive()) return;

  Serial.println("Servo 2 cycle complete.");
}

// ==========================
// SERVO MOVEMENT
// ==========================

void smoothMoveServo1(int targetAngle) {
  targetAngle = constrain(targetAngle, SAFE_LOW, SAFE_HIGH);

  Serial.print("Servo 1 moving to ");
  Serial.println(targetAngle);

  while (servo1Angle != targetAngle) {
    if (estopActive()) {
      Serial.println("ESTOP active. Servo 1 movement stopped.");
      return;
    }

    if (servo1Angle < targetAngle) {
      servo1Angle += STEP_DEGREES;
      if (servo1Angle > targetAngle) servo1Angle = targetAngle;
    } else {
      servo1Angle -= STEP_DEGREES;
      if (servo1Angle < targetAngle) servo1Angle = targetAngle;
    }

    servo1.write(servo1Angle);
    delay(STEP_INTERVAL_MS);
  }

  Serial.print("Servo 1 at ");
  Serial.println(servo1Angle);
}

void smoothMoveServo2(int targetAngle) {
  targetAngle = constrain(targetAngle, SAFE_LOW, SAFE_HIGH);

  Serial.print("Servo 2 moving to ");
  Serial.println(targetAngle);

  while (servo2Angle != targetAngle) {
    if (estopActive()) {
      Serial.println("ESTOP active. Servo 2 movement stopped.");
      return;
    }

    if (servo2Angle < targetAngle) {
      servo2Angle += STEP_DEGREES;
      if (servo2Angle > targetAngle) servo2Angle = targetAngle;
    } else {
      servo2Angle -= STEP_DEGREES;
      if (servo2Angle < targetAngle) servo2Angle = targetAngle;
    }

    servo2.write(servo2Angle);
    delay(STEP_INTERVAL_MS);
  }

  Serial.print("Servo 2 at ");
  Serial.println(servo2Angle);
}

// ==========================
// MANUAL SERIAL COMMANDS
// ==========================

void readSerialCommand() {
  if (Serial.available() == 0) {
    return;
  }

  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toLowerCase();

  if (input == "help") {
    printHelp();
  }

  else if (input == "run1" || input == "a") {
    Serial.println("Manual run: Servo 1 / Type A");
    runServo1Cycle();
  }

  else if (input == "run2" || input == "b") {
    Serial.println("Manual run: Servo 2 / Type B");
    runServo2Cycle();
  }

  else if (input == "done") {
    pulseDone();
  }

  else if (input == "pos") {
    printServoPositions();
  }

  else if (input == "s1l") {
    smoothMoveServo1(SAFE_LOW);
  }

  else if (input == "s1m") {
    smoothMoveServo1(MIDDLE);
  }

  else if (input == "s1h") {
    smoothMoveServo1(SAFE_HIGH);
  }

  else if (input == "s2l") {
    smoothMoveServo2(SAFE_LOW);
  }

  else if (input == "s2m") {
    smoothMoveServo2(MIDDLE);
  }

  else if (input == "s2h") {
    smoothMoveServo2(SAFE_HIGH);
  }

  else if (input.startsWith("s1 ")) {
    int angle = input.substring(3).toInt();

    if (angle >= SAFE_LOW && angle <= SAFE_HIGH) {
      smoothMoveServo1(angle);
    } else {
      printAngleError();
    }
  }

  else if (input.startsWith("s2 ")) {
    int angle = input.substring(3).toInt();

    if (angle >= SAFE_LOW && angle <= SAFE_HIGH) {
      smoothMoveServo2(angle);
    } else {
      printAngleError();
    }
  }

  else {
    Serial.println("Unknown command. Type 'help'.");
  }
}

// ==========================
// SAFETY / ESTOP
// ==========================

bool estopActive() {
  return digitalRead(ESTOP_PIN) == LOW;
}

void delayWithEstopCheck(unsigned long waitTime) {
  unsigned long startTime = millis();

  while (millis() - startTime < waitTime) {
    if (estopActive()) {
      Serial.println("ESTOP active during delay.");
      return;
    }

    delay(10);
  }
}

// ==========================
// PRINT HELPERS
// ==========================

void printServoPositions() {
  Serial.println("Current servo positions:");
  Serial.print("Servo 1 = ");
  Serial.println(servo1Angle);
  Serial.print("Servo 2 = ");
  Serial.println(servo2Angle);

  Serial.print("TYPE_PIN state = ");
  Serial.println(digitalRead(TYPE_PIN));

  Serial.print("START_PIN state = ");
  Serial.println(digitalRead(START_PIN));

  Serial.print("ESTOP_PIN state = ");
  Serial.println(digitalRead(ESTOP_PIN));
}

void printAngleError() {
  Serial.print("Angle out of safe range. Use ");
  Serial.print(SAFE_LOW);
  Serial.print(" to ");
  Serial.println(SAFE_HIGH);
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  help       = show commands");
  Serial.println("  run1 / a   = manually run Servo 1 / wheel type A cycle");
  Serial.println("  run2 / b   = manually run Servo 2 / wheel type B cycle");
  Serial.println("  done       = manually pulse DONE output");
  Serial.println("  pos        = show servo positions and input states");
  Serial.println();
  Serial.println("Servo 1 manual:");
  Serial.println("  s1l        = servo 1 safe low / 15 deg");
  Serial.println("  s1m        = servo 1 middle / 90 deg");
  Serial.println("  s1h        = servo 1 safe high / 170 deg");
  Serial.println("  s1 120     = servo 1 exact angle");
  Serial.println();
  Serial.println("Servo 2 manual:");
  Serial.println("  s2l        = servo 2 safe low / 15 deg");
  Serial.println("  s2m        = servo 2 middle / 90 deg");
  Serial.println("  s2h        = servo 2 safe high / 170 deg");
  Serial.println("  s2 120     = servo 2 exact angle");
  Serial.println();
  Serial.println("Power-up/default:");
  Serial.println("  Servo 1 starts at 90 deg");
  Serial.println("  Servo 2 starts at 90 deg");
  Serial.println();
  Serial.println("Cycle movement:");
  Serial.println("  selected servo moves to 15 deg first");
  Serial.println("  then selected servo moves to 170 deg");
  Serial.println();
  Serial.println("PLC logic:");
  Serial.println("  START_PIN LOW triggers cycle");
  Serial.println("  TYPE_PIN HIGH = Servo 1 / Type A");
  Serial.println("  TYPE_PIN LOW  = Servo 2 / Type B");
  Serial.println("  DONE_PIN pulses HIGH for 500 ms when cycle completes");
  Serial.println("  ESTOP_PIN LOW stops movement");
}

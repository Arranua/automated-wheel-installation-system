#include <ESP32Servo.h>

Servo gripperServo;

// ======================================================
// AXIS PINS
// ======================================================

// OLD Axis 1 pins are now Z
const int STEP_Z_PIN = 25;
const int DIR_Z_PIN  = 26;
const int EN_Z_PIN   = 27;
const int HOME_Z_PIN = 32;

// OLD Axis 2 pins are now X
const int STEP_X_PIN = 18;
const int DIR_X_PIN  = 19;
const int EN_X_PIN   = 21;
const int HOME_X_PIN = 22;

// OLD Axis 3 pins are now Y
const int STEP_Y_PIN = 23;
const int DIR_Y_PIN  = 14;
const int EN_Y_PIN   = 13;
const int HOME_Y_PIN = 4;

// ======================================================
// SERVO
// ======================================================

const int GRIPPER_SERVO_PIN = 16;
const int GRIPPER_OPEN_ANGLE  = 150;
const int GRIPPER_CLOSE_ANGLE = 54;

const int GRIPPER_MIN_ANGLE = 0;
const int GRIPPER_MAX_ANGLE = 180;

int currentGripperAngle = GRIPPER_OPEN_ANGLE;

// ======================================================
// PLC SIGNALS
// ======================================================

const int START_PIN = 33;       // PLC start input, active LOW
const int DONE_PIN  = 17;       // Cycle done output pulse to PLC
const int ESTOP_PIN = 5;        // ESTOP input, active LOW

const unsigned long DONE_PULSE_MS = 500;

// ======================================================
// DRIVER / DIRECTION SETTINGS
// ======================================================

const int DRIVER_ENABLED  = HIGH;
const int DRIVER_DISABLED = LOW;

const int X_PLUS_DIR = HIGH;
const int Y_PLUS_DIR = HIGH;
const int Z_PLUS_DIR = HIGH;

const int X_HOME_DIR = LOW;
const int Y_HOME_DIR = LOW;
const int Z_HOME_DIR = LOW;

const int X_AWAY_DIR = !X_HOME_DIR;
const int Y_AWAY_DIR = !Y_HOME_DIR;
const int Z_AWAY_DIR = !Z_HOME_DIR;

// ======================================================
// SPEED / LIMITS
// ======================================================

const int STEP_DELAY_US = 500;

const long X_BACKOFF_STEPS = 100;
const long Y_BACKOFF_STEPS = 200;
const long Z_BACKOFF_STEPS = 200;

const long X_MIN_POS = 0;
const long X_MAX_POS = 8250;

const long Y_MIN_POS = 0;
const long Y_MAX_POS = 3400;

const long Z_MIN_POS = 0;
const long Z_MAX_POS = 5200;

const long MAX_SINGLE_JOG_STEPS = 2500;

// ======================================================
// MAIN CALIBRATION POINTS
// ONLY CHANGE THESE WHEN RECALIBRATING
// ======================================================

struct Point {
  long x;
  long y;
  long z;
};

Point WHEEL1_POS      = {660, 2075, 20};
Point WHEEL2_POS      = {6815, 2025, 130};
Point DROPOFF_POS     = {8190, 0, 150};
Point PICKUP_POS      = {3725, 3100, 3975};
Point PICKUP_BACKOFF  = {3725, 0, 3000};

// ======================================================
// STATE
// ======================================================

long xPos = 0;
long yPos = 0;
long zPos = 0;

bool cycleRunning = false;
bool manualXHold = false;
bool lastStartState = HIGH;
bool lastEstopState = HIGH;
bool abortRequested = false;

// 0 = ready to remove old wheels and pick up wheel 1
// 1 = wheel 1 in gripper, ready to install wheel 1 and pick up wheel 2
// 2 = wheel 2 in gripper, ready to install wheel 2
int cycleStage = 0;

// ======================================================
// SETUP / LOOP
// ======================================================

void setup() {
  Serial.begin(9600);

  pinMode(STEP_X_PIN, OUTPUT);
  pinMode(DIR_X_PIN, OUTPUT);
  pinMode(EN_X_PIN, OUTPUT);
  pinMode(HOME_X_PIN, INPUT_PULLUP);

  pinMode(STEP_Y_PIN, OUTPUT);
  pinMode(DIR_Y_PIN, OUTPUT);
  pinMode(EN_Y_PIN, OUTPUT);
  pinMode(HOME_Y_PIN, INPUT_PULLUP);

  pinMode(STEP_Z_PIN, OUTPUT);
  pinMode(DIR_Z_PIN, OUTPUT);
  pinMode(EN_Z_PIN, OUTPUT);
  pinMode(HOME_Z_PIN, INPUT_PULLUP);

  pinMode(START_PIN, INPUT_PULLUP);
  pinMode(ESTOP_PIN, INPUT_PULLUP);

  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);

  digitalWrite(STEP_X_PIN, LOW);
  digitalWrite(STEP_Y_PIN, LOW);
  digitalWrite(STEP_Z_PIN, LOW);

  disableAllDrivers();

  gripperServo.setPeriodHertz(50);
  gripperServo.attach(GRIPPER_SERVO_PIN, 500, 2500);
  gripperServo.write(currentGripperAngle);

  lastEstopState = digitalRead(ESTOP_PIN);

  Serial.println();
  Serial.println("=== WHEEL REMOVE / PICKUP / INSTALL READY ===");
  printHelp();
}

void loop() {
  checkEstopAlways();
  checkPLCStart();

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();
    handleSerialCommand(input);
  }
}

// ======================================================
// SERIAL COMMANDS
// ======================================================

void handleSerialCommand(String input) {
  if (input == "run" || input == "next" || input == "start") {
    startOrContinueCycle();
  }
  else if (input == "home") {
    homeAll();
  }
  else if (input == "homex") {
    homeAxis("X", STEP_X_PIN, DIR_X_PIN, EN_X_PIN, HOME_X_PIN, X_HOME_DIR, X_AWAY_DIR, X_BACKOFF_STEPS, xPos);
  }
  else if (input == "homey") {
    homeAxis("Y", STEP_Y_PIN, DIR_Y_PIN, EN_Y_PIN, HOME_Y_PIN, Y_HOME_DIR, Y_AWAY_DIR, Y_BACKOFF_STEPS, yPos);
  }
  else if (input == "homez") {
    homeAxis("Z", STEP_Z_PIN, DIR_Z_PIN, EN_Z_PIN, HOME_Z_PIN, Z_HOME_DIR, Z_AWAY_DIR, Z_BACKOFF_STEPS, zPos);
  }
  else if (input == "open") {
    openGripper();
  }
  else if (input == "close") {
    closeGripper();
  }
  else if (input.startsWith("grip ")) {
    int angle = input.substring(5).toInt();
    setGripperAngle(angle);
  }
  else if (input.startsWith("gripper ")) {
    int angle = input.substring(8).toInt();
    setGripperAngle(angle);
  }
  else if (input.startsWith("goto ")) {
    handleGotoCommand(input);
  }
  else if (input == "pos") {
    printPosition();
  }
  else if (input == "done") {
    pulseDone();
  }
  else if (input == "xholdon") {
    manualXHold = true;
    digitalWrite(EN_X_PIN, DRIVER_ENABLED);
    Serial.println("Manual X hold ON.");
  }
  else if (input == "xholdoff") {
    manualXHold = false;
    if (!cycleRunning) {
      digitalWrite(EN_X_PIN, DRIVER_DISABLED);
    }
    Serial.println("Manual X hold OFF.");
  }
  else if (input == "xhold") {
    printXHoldStatus();
  }
  else if (input == "driversoff") {
    cycleRunning = false;
    manualXHold = false;
    disableAllDrivers();
    Serial.println("All drivers OFF.");
  }
  else if (input == "resetstage") {
    cycleStage = 0;
    cycleRunning = false;
    abortRequested = false;
    manualXHold = false;
    disableAllDrivers();
    Serial.println("Cycle stage reset to 0.");
  }
  else if (input == "help") {
    printHelp();
  }
  else if (input.startsWith("x+") || input.startsWith("x-")) {
    moveFromCommand(input, 'x');
  }
  else if (input.startsWith("y+") || input.startsWith("y-")) {
    moveFromCommand(input, 'y');
  }
  else if (input.startsWith("z+") || input.startsWith("z-")) {
    moveFromCommand(input, 'z');
  }
  else {
    Serial.println("Unknown command. Type help.");
  }
}

// ======================================================
// ALWAYS-RUNNING ESTOP CHECK
// ======================================================

void checkEstopAlways() {
  bool currentEstopState = digitalRead(ESTOP_PIN);

  // Detect ESTOP press edge: HIGH -> LOW
  if (lastEstopState == HIGH && currentEstopState == LOW) {
    abortCycle("ESTOP pressed while idle/waiting.");
  }

  lastEstopState = currentEstopState;
}

// ======================================================
// PLC START
// ======================================================

void checkPLCStart() {
  bool currentStartState = digitalRead(START_PIN);

  if (lastStartState == HIGH && currentStartState == LOW) {
    if (estopActive()) {
      Serial.println("PLC start ignored. ESTOP active.");
    } else {
      startOrContinueCycle();
    }
  }

  lastStartState = currentStartState;
}

void startOrContinueCycle() {
  if (estopActive()) {
    Serial.println("Start ignored. ESTOP active.");
    return;
  }

  abortRequested = false;

  if (cycleStage == 0) {
    runStage0();
  }
  else if (cycleStage == 1) {
    runStage1();
  }
  else if (cycleStage == 2) {
    runStage2();
  }
  else {
    cycleStage = 0;
    Serial.println("Stage was invalid. Resetting to 0.");
  }
}

// ======================================================
// STAGES
// ======================================================

void runStage0() {
  Serial.println();
  Serial.println("===== STAGE 0: REMOVE OLD WHEELS, PICKUP WHEEL 1 =====");

  cycleRunning = true;

  if (!homeAll()) return;

  // X stays locked during active cycle to prevent sliding.
  digitalWrite(EN_X_PIN, DRIVER_ENABLED);

  if (!removeOldWheel(WHEEL1_POS, "OLD WHEEL 1")) return;
  if (!removeOldWheel(WHEEL2_POS, "OLD WHEEL 2")) return;
  if (!pickupNewWheel("NEW WHEEL 1")) return;

  if (!pulseDone()) return;

  cycleStage = 1;

  Serial.println("===== STAGE 0 COMPLETE =====");
  Serial.println("Wheel 1 is now in gripper.");
  Serial.println("Waiting for next PLC start or manual next.");
  printPosition();

  waitForStartRelease();
}

void runStage1() {
  Serial.println();
  Serial.println("===== STAGE 1: INSTALL WHEEL 1, PICKUP WHEEL 2 =====");

  cycleRunning = true;

  // X stays locked during active cycle to prevent sliding.
  digitalWrite(EN_X_PIN, DRIVER_ENABLED);

  if (!leavePickupWithWheel()) return;
  if (!installWheel(WHEEL1_POS, "WHEEL 1")) return;
  if (!pickupNewWheel("NEW WHEEL 2")) return;

  if (!pulseDone()) return;

  cycleStage = 2;

  Serial.println("===== STAGE 1 COMPLETE =====");
  Serial.println("Wheel 1 installed. Wheel 2 is now in gripper.");
  Serial.println("Waiting for next PLC start or manual next.");
  printPosition();

  waitForStartRelease();
}

void runStage2() {
  Serial.println();
  Serial.println("===== STAGE 2: INSTALL WHEEL 2, FINISH =====");

  cycleRunning = true;

  // X stays locked during active cycle to prevent sliding.
  digitalWrite(EN_X_PIN, DRIVER_ENABLED);

  if (!leavePickupWithWheel()) return;
  if (!installWheel(WHEEL2_POS, "WHEEL 2")) return;

  if (!pulseDone()) return;

  cycleRunning = false;
  cycleStage = 0;   // AUTO RESET BACK TO 0

  // Full cycle complete: turn everything off, including X.
  disableAllDrivers();

  Serial.println("===== FULL MC CYCLE COMPLETE =====");
  Serial.println("Cycle stage auto-reset to 0.");
  Serial.println("All drivers disabled, including X.");
  printPosition();

  waitForStartRelease();
}

// ======================================================
// SEQUENCE HELPERS
// ======================================================

bool removeOldWheel(Point wheel, const char* label) {
  Serial.println();
  Serial.print("----- REMOVE ");
  Serial.print(label);
  Serial.println(" -----");

  if (!openGripper()) return false;

  if (!moveToY0()) return false;
  if (!moveToPosition(wheel.x, 0, wheel.z)) return false;
  if (!moveToPosition(wheel.x, wheel.y, wheel.z)) return false;

  if (!closeGripper()) return false;

  if (!moveToPosition(wheel.x, 0, wheel.z)) return false;
  if (!moveToPosition(DROPOFF_POS.x, 0, DROPOFF_POS.z)) return false;

  if (!openGripper()) return false;

  return true;
}

bool pickupNewWheel(const char* label) {
  Serial.println();
  Serial.print("----- PICKUP ");
  Serial.print(label);
  Serial.println(" -----");

  if (!openGripper()) return false;

  if (!moveToY0()) return false;

  // Travel to pickup safely.
  if (!moveToPosition(PICKUP_BACKOFF.x, 0, PICKUP_BACKOFF.z)) return false;
  if (!moveToPosition(PICKUP_POS.x, 0, PICKUP_POS.z)) return false;

  // Move into pickup.
  if (!moveToPosition(PICKUP_POS.x, PICKUP_POS.y, PICKUP_POS.z)) return false;

  if (!closeGripper()) return false;

  return true;
}

bool leavePickupWithWheel() {
  Serial.println();
  Serial.println("----- LEAVING PICKUP WITH WHEEL -----");
  Serial.println("Breaking normal safety rule here intentionally:");
  Serial.println("  1. Back off on Z first while Y is still extended");
  Serial.println("  2. Then retract Y to 0");

  // First: back off/down on Z while staying at pickup X and pickup Y.
  if (!moveToPosition(PICKUP_POS.x, PICKUP_POS.y, PICKUP_BACKOFF.z)) return false;

  // Second: now retract Y back to 0 at the backed-off Z height.
  if (!moveToPosition(PICKUP_BACKOFF.x, 0, PICKUP_BACKOFF.z)) return false;

  return true;
}

bool installWheel(Point wheel, const char* label) {
  Serial.println();
  Serial.print("----- INSTALL ");
  Serial.print(label);
  Serial.println(" -----");

  // Travel at safe backoff Z.
  if (!moveToPosition(wheel.x, 0, PICKUP_BACKOFF.z)) return false;

  // Drop to wheel install height.
  if (!moveToPosition(wheel.x, 0, wheel.z)) return false;

  // Push onto axle.
  if (!moveToPosition(wheel.x, wheel.y, wheel.z)) return false;

  if (!openGripper()) return false;

  // Pull back.
  if (!moveToPosition(wheel.x, 0, wheel.z)) return false;

  return true;
}

bool moveToY0() {
  return moveToPosition(xPos, 0, zPos);
}

// ======================================================
// MANUAL JOGGING
// ======================================================

void moveFromCommand(String cmd, char axis) {
  char sign = cmd.charAt(1);
  long steps = cmd.substring(2).toInt();

  if (steps <= 0) {
    Serial.println("Invalid step amount.");
    return;
  }

  if (steps > MAX_SINGLE_JOG_STEPS) {
    Serial.print("Move rejected. Max single jog is ");
    Serial.print(MAX_SINGLE_JOG_STEPS);
    Serial.println(" steps.");
    return;
  }

  bool plusMove = (sign == '+');

  if (axis == 'x') {
    long targetPos = xPos + (plusMove ? steps : -steps);

    if (targetPos < X_MIN_POS || targetPos > X_MAX_POS) {
      Serial.println("X jog rejected. Outside software limits.");
      Serial.print("Requested X = ");
      Serial.println(targetPos);
      return;
    }

    moveAxisTo("X", STEP_X_PIN, DIR_X_PIN, EN_X_PIN, X_PLUS_DIR, xPos, targetPos);
  }

  else if (axis == 'y') {
    long targetPos = yPos + (plusMove ? steps : -steps);

    if (targetPos < Y_MIN_POS || targetPos > Y_MAX_POS) {
      Serial.println("Y jog rejected. Outside software limits.");
      Serial.print("Requested Y = ");
      Serial.println(targetPos);
      return;
    }

    moveAxisTo("Y", STEP_Y_PIN, DIR_Y_PIN, EN_Y_PIN, Y_PLUS_DIR, yPos, targetPos);
  }

  else if (axis == 'z') {
    long targetPos = zPos + (plusMove ? steps : -steps);

    if (targetPos < Z_MIN_POS || targetPos > Z_MAX_POS) {
      Serial.println("Z jog rejected. Outside software limits.");
      Serial.print("Requested Z = ");
      Serial.println(targetPos);
      return;
    }

    moveAxisTo("Z", STEP_Z_PIN, DIR_Z_PIN, EN_Z_PIN, Z_PLUS_DIR, zPos, targetPos);
  }

  printPosition();
}

// ======================================================
// MANUAL SAFE GOTO
// ======================================================

void handleGotoCommand(String input) {
  // Accepts:
  // goto 615 2075 150
  // goto 615,2075,150

  input.replace(",", " ");

  int firstSpace = input.indexOf(' ');
  if (firstSpace < 0) {
    Serial.println("Invalid goto command. Use: goto X Y Z");
    return;
  }

  String data = input.substring(firstSpace + 1);
  data.trim();

  int space1 = data.indexOf(' ');
  int space2 = data.indexOf(' ', space1 + 1);

  if (space1 < 0 || space2 < 0) {
    Serial.println("Invalid goto command. Use: goto X Y Z");
    Serial.println("Example: goto 615 2075 150");
    return;
  }

  long targetX = data.substring(0, space1).toInt();
  long targetY = data.substring(space1 + 1, space2).toInt();
  long targetZ = data.substring(space2 + 1).toInt();

  Serial.println();
  Serial.println("Manual safe coordinate move requested:");
  Serial.print("Target X=");
  Serial.print(targetX);
  Serial.print(" Y=");
  Serial.print(targetY);
  Serial.print(" Z=");
  Serial.println(targetZ);

  moveToPositionManualSafe(targetX, targetY, targetZ);
}

bool moveToPositionManualSafe(long targetX, long targetY, long targetZ) {
  if (estopActive()) {
    return abortCycle("Manual goto cancelled. ESTOP active.");
  }

  if (!positionIsSafe(targetX, targetY, targetZ)) {
    Serial.println("Manual goto rejected. Target outside software limits.");
    Serial.print("Requested X=");
    Serial.print(targetX);
    Serial.print(" Y=");
    Serial.print(targetY);
    Serial.print(" Z=");
    Serial.println(targetZ);
    return false;
  }

  Serial.println("Manual safe path:");
  Serial.println("  1. Y to 0");
  Serial.println("  2. Z to target Z");
  Serial.println("  3. X to target X");
  Serial.println("  4. Y to final target Y");

  if (!moveAxisTo("Y", STEP_Y_PIN, DIR_Y_PIN, EN_Y_PIN, Y_PLUS_DIR, yPos, 0)) return false;
  if (!moveAxisTo("Z", STEP_Z_PIN, DIR_Z_PIN, EN_Z_PIN, Z_PLUS_DIR, zPos, targetZ)) return false;
  if (!moveAxisTo("X", STEP_X_PIN, DIR_X_PIN, EN_X_PIN, X_PLUS_DIR, xPos, targetX)) return false;
  if (!moveAxisTo("Y", STEP_Y_PIN, DIR_Y_PIN, EN_Y_PIN, Y_PLUS_DIR, yPos, targetY)) return false;

  Serial.println("Manual goto complete.");
  printPosition();

  return true;
}

// ======================================================
// MOTION HELPERS
// ======================================================

bool moveToPosition(long targetX, long targetY, long targetZ) {
  if (estopActive()) {
    return abortCycle("MOVE CANCELLED: ESTOP active.");
  }

  Serial.println();
  Serial.print("Moving to X=");
  Serial.print(targetX);
  Serial.print(" Y=");
  Serial.print(targetY);
  Serial.print(" Z=");
  Serial.println(targetZ);

  if (!positionIsSafe(targetX, targetY, targetZ)) {
    Serial.println("MOVE REJECTED: target outside software limits.");
    return false;
  }

  if (!moveAxisTo("Z", STEP_Z_PIN, DIR_Z_PIN, EN_Z_PIN, Z_PLUS_DIR, zPos, targetZ)) return false;
  if (!moveAxisTo("X", STEP_X_PIN, DIR_X_PIN, EN_X_PIN, X_PLUS_DIR, xPos, targetX)) return false;
  if (!moveAxisTo("Y", STEP_Y_PIN, DIR_Y_PIN, EN_Y_PIN, Y_PLUS_DIR, yPos, targetY)) return false;

  printPosition();
  return true;
}

bool positionIsSafe(long targetX, long targetY, long targetZ) {
  if (targetX < X_MIN_POS || targetX > X_MAX_POS) return false;
  if (targetY < Y_MIN_POS || targetY > Y_MAX_POS) return false;
  if (targetZ < Z_MIN_POS || targetZ > Z_MAX_POS) return false;
  return true;
}

bool moveAxisTo(
  const char* axisName,
  int stepPin,
  int dirPin,
  int enPin,
  int plusDir,
  long &currentPos,
  long targetPos
) {
  if (estopActive()) {
    return abortCycle("Axis move cancelled. ESTOP active.");
  }

  long delta = targetPos - currentPos;

  if (delta == 0) {
    Serial.print(axisName);
    Serial.println(" already at target.");
    return true;
  }

  bool plusMove = delta > 0;
  long steps = plusMove ? delta : -delta;
  int moveDir = plusMove ? plusDir : !plusDir;

  Serial.print(axisName);
  Serial.print(" moving from ");
  Serial.print(currentPos);
  Serial.print(" to ");
  Serial.print(targetPos);
  Serial.print(" steps: ");
  Serial.println(steps);

  digitalWrite(enPin, DRIVER_ENABLED);
  delay(200);

  digitalWrite(dirPin, moveDir);

  for (long i = 0; i < steps; i++) {
    if (estopActive()) {
      digitalWrite(stepPin, LOW);
      return abortCycle("Axis stopped by ESTOP.");
    }

    stepOnce(stepPin);
  }

  digitalWrite(stepPin, LOW);

  // X stays enabled during cycle or manual X hold.
  // Y and Z always disable after moves.
  if (strcmp(axisName, "X") == 0 && (manualXHold || cycleRunning)) {
    Serial.println("X kept enabled.");
  }
  else {
    digitalWrite(enPin, DRIVER_DISABLED);
  }

  currentPos = targetPos;

  Serial.print(axisName);
  Serial.println(" move complete.");

  return true;
}

// ======================================================
// HOMING
// ======================================================

bool homeAll() {
  Serial.println();
  Serial.println("Homing all axes...");
  Serial.println("Homing order: Y first, then X, then Z.");

  if (!homeAxis("Y", STEP_Y_PIN, DIR_Y_PIN, EN_Y_PIN, HOME_Y_PIN, Y_HOME_DIR, Y_AWAY_DIR, Y_BACKOFF_STEPS, yPos)) return false;
  if (!homeAxis("X", STEP_X_PIN, DIR_X_PIN, EN_X_PIN, HOME_X_PIN, X_HOME_DIR, X_AWAY_DIR, X_BACKOFF_STEPS, xPos)) return false;
  if (!homeAxis("Z", STEP_Z_PIN, DIR_Z_PIN, EN_Z_PIN, HOME_Z_PIN, Z_HOME_DIR, Z_AWAY_DIR, Z_BACKOFF_STEPS, zPos)) return false;

  Serial.println("Homing complete.");
  printPosition();

  return true;
}

bool homeAxis(
  const char* axisName,
  int stepPin,
  int dirPin,
  int enPin,
  int homePin,
  int homeDir,
  int awayDir,
  long backoffSteps,
  long &posCounter
) {
  if (estopActive()) {
    return abortCycle("Homing cancelled. ESTOP active.");
  }

  Serial.print("Homing ");
  Serial.println(axisName);

  digitalWrite(enPin, DRIVER_ENABLED);
  delay(300);

  digitalWrite(dirPin, homeDir);

  while (digitalRead(homePin) == HIGH) {
    if (estopActive()) {
      digitalWrite(stepPin, LOW);
      return abortCycle("Homing stopped by ESTOP.");
    }

    stepOnce(stepPin);
  }

  digitalWrite(stepPin, LOW);

  Serial.print(axisName);
  Serial.println(" home switch hit.");

  if (!delayWithEstopCheck(200)) return false;

  Serial.print(axisName);
  Serial.println(" backing off switch...");

  digitalWrite(dirPin, awayDir);

  for (long i = 0; i < backoffSteps; i++) {
    if (estopActive()) {
      digitalWrite(stepPin, LOW);
      return abortCycle("Backoff stopped by ESTOP.");
    }

    stepOnce(stepPin);
  }

  digitalWrite(stepPin, LOW);

  // X stays enabled during cycle or manual X hold.
  // Y and Z always disable after homing.
  if (strcmp(axisName, "X") == 0 && (manualXHold || cycleRunning)) {
    Serial.println("X kept enabled after homing.");
  }
  else {
    digitalWrite(enPin, DRIVER_DISABLED);
  }

  posCounter = 0;

  Serial.print(axisName);
  Serial.println(" homed. Position set to 0.");

  return true;
}

// ======================================================
// GRIPPER / PLC / ESTOP
// ======================================================

bool openGripper() {
  return setGripperAngle(GRIPPER_OPEN_ANGLE);
}

bool closeGripper() {
  return setGripperAngle(GRIPPER_CLOSE_ANGLE);
}

bool setGripperAngle(int angle) {
  if (estopActive()) {
    return abortCycle("Gripper move cancelled. ESTOP active.");
  }

  if (angle < GRIPPER_MIN_ANGLE || angle > GRIPPER_MAX_ANGLE) {
    Serial.print("Gripper angle rejected. Use ");
    Serial.print(GRIPPER_MIN_ANGLE);
    Serial.print(" to ");
    Serial.println(GRIPPER_MAX_ANGLE);
    return false;
  }

  Serial.print("Moving gripper to ");
  Serial.print(angle);
  Serial.println(" degrees...");

  gripperServo.write(angle);
  currentGripperAngle = angle;

  if (!delayWithEstopCheck(500)) return false;

  Serial.print("Gripper angle = ");
  Serial.println(currentGripperAngle);

  return true;
}

bool pulseDone() {
  if (estopActive()) {
    return abortCycle("DONE pulse cancelled. ESTOP active.");
  }

  Serial.println("Sending DONE pulse to PLC...");
  digitalWrite(DONE_PIN, HIGH);

  if (!delayWithEstopCheck(DONE_PULSE_MS)) {
    digitalWrite(DONE_PIN, LOW);
    return false;
  }

  digitalWrite(DONE_PIN, LOW);
  Serial.println("DONE pulse complete.");

  return true;
}

bool delayWithEstopCheck(unsigned long waitTime) {
  unsigned long startTime = millis();

  while (millis() - startTime < waitTime) {
    if (estopActive()) {
      return abortCycle("Delay stopped by ESTOP.");
    }

    delay(10);
  }

  return true;
}

bool abortCycle(const char* message) {
  Serial.println();
  Serial.println("!!!!! ESTOP / ABORT !!!!!");
  Serial.println(message);

  digitalWrite(DONE_PIN, LOW);
  disableAllDrivers();

  cycleRunning = false;
  abortRequested = true;
  cycleStage = 0;

  // Prevent duplicate ESTOP messages after returning to loop.
  lastEstopState = digitalRead(ESTOP_PIN);

  Serial.println("Cycle aborted. Stage reset to 0.");
  Serial.println("Re-home before running again.");
  printPosition();

  return false;
}

void waitForStartRelease() {
  while (digitalRead(START_PIN) == LOW) {
    checkEstopAlways();

    if (estopActive()) {
      return;
    }

    delay(10);
  }

  lastStartState = HIGH;
}

bool estopActive() {
  return digitalRead(ESTOP_PIN) == LOW;
}

// ======================================================
// BASIC HELPERS
// ======================================================

void stepOnce(int stepPin) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(STEP_DELAY_US);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(STEP_DELAY_US);
}

void disableAllDrivers() {
  digitalWrite(EN_X_PIN, DRIVER_DISABLED);
  digitalWrite(EN_Y_PIN, DRIVER_DISABLED);
  digitalWrite(EN_Z_PIN, DRIVER_DISABLED);
}

void printPosition() {
  Serial.println("Current estimated positions:");
  Serial.print("X = ");
  Serial.println(xPos);
  Serial.print("Y = ");
  Serial.println(yPos);
  Serial.print("Z = ");
  Serial.println(zPos);
  Serial.print("Gripper angle = ");
  Serial.println(currentGripperAngle);
  Serial.print("Cycle stage = ");
  Serial.println(cycleStage);
  printXHoldStatus();
}

void printXHoldStatus() {
  Serial.print("Manual X hold = ");
  Serial.println(manualXHold ? "ON" : "OFF");

  Serial.print("Cycle running = ");
  Serial.println(cycleRunning ? "YES" : "NO");
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  run          = start/continue cycle based on stage");
  Serial.println("  next         = same as run");
  Serial.println("  start        = same as run");
  Serial.println("  resetstage   = reset cycle stage back to 0");
  Serial.println("  home         = home all axes, Y first");
  Serial.println("  homex        = home X only");
  Serial.println("  homey        = home Y only");
  Serial.println("  homez        = home Z only");
  Serial.println("  x+100        = jog X positive 100 steps");
  Serial.println("  x-100        = jog X negative 100 steps");
  Serial.println("  y+100        = jog Y positive 100 steps");
  Serial.println("  y-100        = jog Y negative 100 steps");
  Serial.println("  z+100        = jog Z positive 100 steps");
  Serial.println("  z-100        = jog Z negative 100 steps");
  Serial.println("  goto X Y Z   = safe manual move to coordinates");
  Serial.println("  goto X,Y,Z   = same as goto X Y Z");
  Serial.println("  open         = open gripper");
  Serial.println("  close        = close gripper");
  Serial.println("  grip 75      = manually set gripper angle");
  Serial.println("  gripper 75   = same as grip 75");
  Serial.println("  done         = manual DONE pulse");
  Serial.println("  pos          = show current position");
  Serial.println();
  Serial.println("X hold commands:");
  Serial.println("  xholdon      = manually keep X axis enabled");
  Serial.println("  xholdoff     = let X axis disable again");
  Serial.println("  xhold        = show X hold status");
  Serial.println("  driversoff   = force all drivers off");
  Serial.println();
  Serial.println("PLC:");
  Serial.println("  START_PIN LOW acts like run/next depending on stage");
  Serial.println("  DONE_PIN pulses HIGH for 500 ms");
  Serial.println("  ESTOP_PIN LOW aborts cycle and resets stage to 0");
  Serial.println("  ESTOP is checked even while idle/waiting");
  Serial.println();
  Serial.println("Main editable points:");
  Serial.println("  WHEEL1_POS");
  Serial.println("  WHEEL2_POS");
  Serial.println("  DROPOFF_POS");
  Serial.println("  PICKUP_POS");
  Serial.println("  PICKUP_BACKOFF");
}

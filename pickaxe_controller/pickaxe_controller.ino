#include <I2Cdev.h>
#include <MPU6050.h>
#include <BleMouse.h>

MPU6050 mpu;
BleMouse mouse;

// GPIO Pin Defs
#define BUTTON_L  5  // left button
#define BUTTON_B  18  // back button
#define BUTTON_R  19  // right button
/* GPIO 21 (SDA), 22 (SCL) for MPU6050 */

// Timing constants
const int SWING_HOLD_MS = 125; // Button debounce
const unsigned long HOLD_COUNTDOWN_TIMER = 350; // If no swing after 500ms, release

// Sensor constants
const int AX_THRESHOLD = 1000; // min acceleration value to trigger the button

/* ======================= Global variables ======================= */
int16_t ax, ay, az, gx, gy, gz; // collect the accelerometer/gyroscope data
int old_ax = 0; // accelerometer data from the previous loop
int delta_ax; // change in acceleration

unsigned int countdownStart = 0;
bool countdownRunning = false;

int swing_prev = 0;   // 0 = no swing, 1 = left swing, 2 = right swing
bool swing_now = false;

/* ======================= Function Declarations/Definitions ======================= */
// Inactivity countdown for release
void startCountdown();
int checkCountdown();
void refreshCountdown();
bool checkForSwing();

void setup() {
  Serial.begin(115200);

  // Configure GPIOs (Button press connects to ground)
  pinMode(BUTTON_L, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_R, INPUT_PULLUP);

  // Initialize MPU sensor
  mpu.initialize();
  if (!mpu.testConnection()) {
    while (1);
  }
  mouse.begin();
}

void loop() {
  // Check if mouse disconnected
   if (!mouse.isConnected()) {
    delay(100);
    return;
  }

  unsigned long startTime;

  swing_now = checkForSwing();

  if (swing_now && !swing_prev) {
    int L = !digitalRead(BUTTON_L);
    int R = !digitalRead(BUTTON_R);
    int B = !digitalRead(BUTTON_B);

    // 1) Left + Back --> Left hold for 1 swing
    if (L && B) {
      Serial.println("Left hold");
      mouse.press(MOUSE_LEFT);
      startCountdown(HOLD_COUNTDOWN_TIMER);
      // Each swing refreshes countdown. When countdown expires, release
      while (!checkCountdown(HOLD_COUNTDOWN_TIMER)) {
        if (checkForSwing()) {
          refreshCountdown(HOLD_COUNTDOWN_TIMER);
        }
        delay(25);
      }
      Serial.println("Left release");
      mouse.release(MOUSE_LEFT);
      swing_now = 0;
      swing_prev = swing_now;
    } 
    // 2) Right + Back --> Right hold for 1 swing
    else if (R && B) {
      Serial.println("Right hold");
      mouse.press(MOUSE_RIGHT);
      startCountdown(HOLD_COUNTDOWN_TIMER);
      // Each swing refreshes countdown. When countdown expires, release
      while (!checkCountdown(HOLD_COUNTDOWN_TIMER)) {
        if (checkForSwing()) {
          refreshCountdown(HOLD_COUNTDOWN_TIMER);
        }
        delay(25);
      }
      Serial.println("Right release");
      mouse.release(MOUSE_RIGHT);
      swing_now = false;
      swing_prev = swing_now;
    }
    // 3) Left --> left click
    else if (L) {
      Serial.println("Left click");
      mouse.click(MOUSE_LEFT);
    }
    // 4) Right --> right click
    else if (R) {
      Serial.println("Right click");
      mouse.click(MOUSE_RIGHT);
    }
    // 5) Back --> no click, no pan
    else if (B) {
      Serial.println("No pan");
    }
  }
  delay(SWING_HOLD_MS); // small debouncing
}

void startCountdown(const unsigned long duration) {
  countdownStart = millis();
  countdownRunning = true;
}

// Check remaining time and stop when hits zero. 
int checkCountdown(const unsigned long duration) {
  if (!countdownRunning) return 1;

  unsigned long elapsed = millis() - countdownStart;

  if (elapsed >= duration) {
    countdownRunning = false; // Countdown finished
  }

  return 0;
}

void refreshCountdown(const unsigned long duration) {
  countdownStart = millis();
}

bool checkForSwing() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // read current accelerometer data
  delta_ax = (old_ax - ax);
  old_ax = ax;

  if (abs(delta_ax) > AX_THRESHOLD) { // swing detected
    return true;
  }

  return false;
}

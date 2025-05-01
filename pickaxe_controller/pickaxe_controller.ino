#include <Wire.h>
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

/* ======================= Global variables ======================= */
// Timing constants
const int SWING_HOLD_MS = 25; // Small delay in between hold check cycles
const int LOOP_DELAY_MS = 20; // Small delay in between mouse movements/clicks (debouncing)
const unsigned long HOLD_COUNTDOWN_TIMER = 350; // If no swing after 500ms, release
const int CLICK_COOLDOWN = 150; // 250ms cooldown between clicks

// Sensor constants
const int AX_THRESHOLD = 20000; // min acceleration value to trigger the button
const int SPEED = 200;

int16_t ax, ay, az, gx, gy, gz; // collect the accelerometer/gyroscope data
int old_ax = 0; // accelerometer data from the previous loop
int delta_ax; // change in acceleration

// int old_gx, old_gy = 0, old_gz = 0;
// int delta_gx, delta_gy, delta_gz;

unsigned int countdownStart = 0;
bool countdownRunning = false;
unsigned long lastClickTime = 0; // last time a click was made

int swing_prev = 0;   // 0 = no swing, 1 = left swing, 2 = right swing
bool swing_now = false;

/* ======================= Function Declarations ======================= */
// Inactivity countdown for release
void startCountdown();
int checkCountdown();
void refreshCountdown();
bool checkForSwing();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Configure GPIOs (Button press connects to ground)
  pinMode(BUTTON_L, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_R, INPUT_PULLUP);

  // Initialize MPU sensor
  mpu.initialize();
  Serial.println("MPU6050 initializing...");
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  // Initialize bluetooth mouse
  mouse.begin();
  Serial.println("Mouse started");
}

void loop() {
  // Check if mouse disconnected
   if (!mouse.isConnected()) {
    delay(100);
    return;
  }

  /* =========== Handling Mouse Movement =========== */
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // Read the current accelerometer data
  //delta_gx = (old_gx - gx);
  //old_gx = gx;
  //delta_gy = (old_gy - gy);
  old_gy = gy;
  //delta_gz = (old_gz - gz);
  old_gz = gz;
  //if (abs(gx) < 30) gx = 0;
  if (abs(gy) < 30) gy = 0;
  gz += 130;  // Account for gravity
  if (abs(gz) < 30) gz = 0;
  //Serial.print("Move mouse: "); Serial.print(gy); Serial.print(", "); Serial.println(gz);
  if (digitalRead(BUTTON_L) && digitalRead(BUTTON_R) && digitalRead(BUTTON_B)) {
    // Only move mouse if no buttons are pressed
    mouse.move(gy*1.3 / SPEED, gz*0.9 / SPEED); // L/R more sensitive, U/D less sensitive
  }
  
  /* =========== Swing/Button Input Logic =========== */

  unsigned long startTime;

  swing_now = checkForSwing();
  
  if (swing_now && !swing_prev) {
    int L = !digitalRead(BUTTON_L);
    int R = !digitalRead(BUTTON_R);
    int B = !digitalRead(BUTTON_B);

    // If swing is detected but no buttons are pressed, break?

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
        delay(SWING_HOLD_MS);
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
        delay(SWING_HOLD_MS);
      }
      Serial.println("Right release");
      mouse.release(MOUSE_RIGHT);
      swing_now = false;
      swing_prev = swing_now;
    }
    // 3) Left --> left click
    else if (L) {
      unsigned long now = millis();
      if (now - lastClickTime >= CLICK_COOLDOWN) {
        //Serial.println("Left click!!!!!!!!!!!!!!");
        mouse.click(MOUSE_LEFT);
        lastClickTime = now;
      } else {
        Serial.println("Left click suppressed (cooldown)");
      }
    }
    // 4) Right --> right click
    else if (R) {
      unsigned long now = millis();
      if (now - lastClickTime >= CLICK_COOLDOWN) {
        //Serial.println("Right click!!!!!!!!!!!!!!!");
        mouse.click(MOUSE_RIGHT);
        lastClickTime = now;
      } else {
        Serial.println("Right click suppressed (cooldown)");
      }
    }
    // 5) Back --> no click, no pan
    else if (B) {
      Serial.println("No pan");
    }
  }

  delay(LOOP_DELAY_MS); // Small debouncing delay
}

/* ======================= Function Definitions ======================= */

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

char sensor_buffer[80];
bool checkForSwing() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // read current accelerometer data
  delta_ax = (old_ax - ax);
  old_ax = ax;

  
  /* sprintf(sensor_buffer, "%d, %d, %d, %d, %d, %d", ax, ay, az, gx, gy, gz);
  Serial.println(sensor_buffer); */
  /* sprintf(sensor_buffer, "%d, %d", ax, delta_ax);
  Serial.println(sensor_buffer); */

  if (delta_ax > AX_THRESHOLD) { // swing detected
    //Serial.print("Swing detected!!!!!!!!!!!!");
    return true;
  }
  //Serial.print("No swing");
  return false;
}


// gy for left/right panning
// gz for up/down panning

// Detect movement: if change in gy > threshold, then move mouse left/right
// Detect movement: if change in gz > threshold, then move mouse up/down

// Or we try to find velocity of the mouse and move it accordingly

// Or even position?

#include <BleMouse.h>
BleMouse mouse;

// GPIO Pin Defs
#define BUTTON_L  18  // left button
#define BUTTON_B  19  // back button
#define BUTTON_R  21  // right button
#define SWING     5   // FOR TESTING: pickaxe is swung

// Timing constants
const int SWING_HOLD_MS = 125;
const unsigned long HOLD_COUNTDOWN_TIMER = 500; // If no swing after 500ms, release

// Program global variables
int swing_prev = 0;   // 0 = no swing, 1 = left swing, 2 = right swing

// Inactivity countdown for release
void startCountdown();
int checkCountdown();
void refreshCountdown();
unsigned int countdownStart = 0;
int countdownRunning = 0;

void setup() {
  Serial.begin(115200);

  // Button press connects to ground
  pinMode(BUTTON_L, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_R, INPUT_PULLUP);
  pinMode(SWING, INPUT_PULLUP);

  mouse.begin();
  Serial.println("Mouse started");
}

void loop() {
  if (!mouse.isConnected()) {
    delay(100);
    return;
  }

  unsigned long startTime;

  /*
    Mouse Operations:
    Left click - mouse.click(MOUSE_LEFT);
    Right click - mouse.click(MOUSE_RIGHT);
    Move - startTime = millis(); while (millis() < startTime + 2000) {mouse.move(x, y);}  // origin is at top left
    Left hold - mouse.press(button); mouse.release(button); // defaults to left button
    More on Arduino Mouse Library: https://docs.arduino.cc/libraries/mouse/
  */

  // Read swing pin
  bool swing_now = !digitalRead(SWING);

  // On falling edge of swing (first swing in sequence): 
  if (swing_now && !swing_prev) {
    int L = !digitalRead(BUTTON_L);
    int R = !digitalRead(BUTTON_R);
    int B = !digitalRead(BUTTON_B);

    // 1) Left + Back --> Left hold for 1 swing
    if (B && L) {
      Serial.println("Left hold");
      mouse.press(MOUSE_LEFT);
      startCountdown(HOLD_COUNTDOWN_TIMER);
      // Each swing refreshes countdown. When countdown expires, release
      while (!checkCountdown(HOLD_COUNTDOWN_TIMER)) {
        if (!digitalRead(SWING)) {
          refreshCountdown(HOLD_COUNTDOWN_TIMER);
        }
        delay(25);
      }
      Serial.println("Left release");
      mouse.release(MOUSE_LEFT);
      swing_now = 0;
      swing_prev = swing_now;
      //mouse.release(MOUSE_LEFT);
    } 
    // 2) Right + Back --> Right hold for 1 swing
    else if (B && R) {
      Serial.println("Right hold");
      mouse.press(MOUSE_RIGHT);
      startCountdown(HOLD_COUNTDOWN_TIMER);
      // Each swing refreshes countdown. When countdown expires, release
      while (!checkCountdown(HOLD_COUNTDOWN_TIMER)) {
        if (!digitalRead(SWING)) {
          refreshCountdown(HOLD_COUNTDOWN_TIMER);
        }
        delay(25);
      }
      Serial.println("Right release");
      mouse.release(MOUSE_RIGHT);
      swing_now = 0;
      swing_prev = swing_now;
      //mouse.release(MOUSE_RIGHT);
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

  // If previous swing was high and current swing is also high, do nothing
  
  //swing_prev = swing_now;
  delay(SWING_HOLD_MS); // small debouncing
}


void startCountdown(const unsigned long duration) {
  countdownStart = millis();
  countdownRunning = 1;
}

// Check remaining time and stop when hits zero. 
int checkCountdown(const unsigned long duration) {
  if (!countdownRunning) return 1;

  unsigned long elapsed = millis() - countdownStart;
  if (elapsed >= duration) {
    countdownRunning = 0; // Countdown finished
  }
  return 0;
}

void refreshCountdown(const unsigned long duration) {
  countdownStart = millis();
}
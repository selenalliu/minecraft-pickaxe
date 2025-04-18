
#include <BleMouse.h>

BleMouse mouse;

void setup() {
  Serial.begin(115200);
  mouse.begin();
}

void loop() {

  if (mouse.isConnected()) {
    unsigned long startTime;

    /*
      Mouse Operations:
      Left click - mouse.click(MOUSE_LEFT);
      Right click - mouse.click(MOUSE_RIGHT);
      Move - startTime = millis(); while (millis() < startTime + 2000) {mouse.move(x, y);}  // origin is at top left
      Left hold - mouse.press(button); mouse.release(button); // defaults to left button
      More on Arduino Mouse Library: https://docs.arduino.cc/libraries/mouse/
    */

    
  }

}

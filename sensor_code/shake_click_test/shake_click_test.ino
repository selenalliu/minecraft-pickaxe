#include <I2Cdev.h>
#include <MPU6050.h>
#include <BleMouse.h>

MPU6050 mpu;
BleMouse mouse;

int16_t ax, ay, az, gx, gy, gz; // collect the accelerometer/gyroscope data
int old_ax = 0; // accelerometer data from the previous loop

int delta_ax; // change in acceleration

int ax_threshold = 1000; // min acceleration value to trigger the button

void setup() {
  Serial.begin(115200);
  mpu.initialize();
  if (!mpu.testConnection()) {
    while (1);
  }
  mouse.begin();
}

void loop() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // read current accelerometer data

  delta_ax = (old_ax - ax);
  old_ax = ax; 
  
  // Serial.print("delta_ax = "); 
  // Serial.println(abs(delta_ax));

  if (abs(delta_ax) > ax_threshold)
  {
    mouse.click(MOUSE_LEFT); 
  }
}

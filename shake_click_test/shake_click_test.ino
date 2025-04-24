#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include <BleMouse.h>

MPU6050 mpu; // Initialize MPU
BleMouse mouse;

int16_t ax, ay, az, gx, gy, gz; // Used to collect the accelerometer data - ax for x-axis, ay for y-axis, az for z-axis

int old_ax = 0; // Stores the accelerometer data from the previous loop

int delta_ax;  // Stores the change in acceleration

int ax_threshold = 1000; // The minimum acceleration value to trigger the button

void setup() {
  Serial.begin(115200); // Intialize I2C and the MPU connection
  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    while (1);
  }
  mouse.begin();
}

void loop() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // Read the current accelerometer data

  delta_ax = (old_ax - ax); // Calculate change in acceleration
  old_ax = ax; // Save the current acceleration reading to the old acceleration reading variable
  
  //Serial.print("delta_ax = ");  // Print the change in acceleration in the serial monitor - this is super useful for debugging
  //Serial.println(abs(delta_ax));

  if (abs(delta_ax) > ax_threshold) // Check if change in acceleration is higher than the threshold - use "delta_ax" for positive direction, "-(delta_ax)" for opposite direction, or "abs(delta_ax)" for the absolute acceleration to ignore direction
  {
    mouse.click(MOUSE_LEFT);
    Serial.println("click detected");
  }
}
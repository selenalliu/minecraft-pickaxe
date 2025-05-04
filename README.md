# Minecraft Pickaxe Controller
This game controller mimics the iconic in-game tool that doubles as a mouse and brings physical interactivity to Minecraft, 
allowing players to mine, craft, and control in-game movement by swinging a real, pickaxe-shaped device! ⛏️

## Demo

## Features
Swing to click/hold
- Single swing + left button = left click
- Swings + left button + middle button = left hold
- Same logic for right button
- Sensitivity calibration

Relative Mouse Panning
- Tilt/pan with gyroscope
- Panning disabled during swings or middle button press

## Hardware
- ESP32 Devkit V1
- MPU6050 IMU Sensor
- 3 push buttons
- Powered with 5-20 V (<12V suggested) battery/micro-USB

### Circuit:
![Circuit diagram for Minecraft Pickaxe](https://github.com/selenalliu/minecraft-pickaxe/blob/main/pickaxe_circuit.png?raw=true)
Made with https://www.cirkitstudio.com/

## Setup & Usage
### Clone the repo 
`git clone https://github.com/selenalliu/minecraft-pickaxe`

### Install Arduino Libraries:
1) MPU6050 (https://github.com/ElectronicCats/mpu6050)
2) I2Cdev (https://github.com/jrowberg/i2cdevlib/tree/master)
3) ESP32-BLE-Mouse (https://github.com/sirfragles/ESP32-BLE-Mouse)
4) Wire
### Open & Upload
- Open `pickaxe_controller/pickaxe_controller.ino` in Arduino IDE
- Select ESP32 Dev Module and the correct COM port
- Upload to board
### Bluetooth Pairing
- Controller appears as "ESP32 Bluetooth Mouse"

## Customization
- Movement Sensitivity: Adjust `SPEED` multiplier
- Swing Sensitivity: Adjust `AX_THRESHOLD`

## Future Improvements
- Use a higher-grade accelerometer for true position tracking
- Add a capacitive touch strip for scroll/zoom
- Integrate left-hand controller for dual wielding

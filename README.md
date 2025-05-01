# Minecraft Pickaxe Controller
This game controller mimics the iconic in-game tool and brings physical interactivity to Minecraft, 
allowing players to mine, craft, and control in-game movement by swinging a real, pickaxe-shaped device! ⛏️

## Table of Contents
* [General Info](#general-info)
* [Setup](#setup)
* [Demo](#demo)
  
## General Info
Using an ESP32 Devkit V1 and MPU6050, the controller is able to emulate the functionality of a Bluetooth mouse -- this functionality
is applied in-game with controls like left/right clicks/holds, which allow players to break blocks, interact with items, and more.

## Setup
### Libraries:
1) MPU6050 (https://github.com/ElectronicCats/mpu6050)
2) I2Cdev (https://github.com/jrowberg/i2cdevlib/tree/master)
3) ESP32-BLE-Mouse (https://github.com/sirfragles/ESP32-BLE-Mouse)
4) Wire

### Circuit:
![Circuit diagram for Minecraft Pickaxe](https://github.com/selenalliu/minecraft-pickaxe/blob/main/minecraft_pickaxe_circuit.png?raw=true)

## Demo

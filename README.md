# cubeSolver-2.0

This is an update to the Cube Solver 1.0. This project is still a work in progress

This project's goal is to improve upon the first design by dropping the STM32, using an RTOS, implementing an HMI, and being built on a custom PCB.

The PCB is a completely custom design with the schematic capture and PCB layout being done in KiCad. It is a double sided PCB. 

Schematic: 
<img width="2200" height="1700" alt="schematic-1" src="https://github.com/user-attachments/assets/3d33bd9d-94d3-4d88-b956-111dd891aa0c" />

PCB:
<img width="1933" height="1214" alt="robot" src="https://github.com/user-attachments/assets/665d0d61-13d8-48b3-b68b-7dbef9186e1a" />


This project is written in C/C++ and utilizes FreeRTOS on the ESP32. It was developed within PlatformIO and the ESP-IDF framework. The code is still a WIP
The project is planned to have an HMI. The HMI will allow the user to control the device via a touch screen. The HMI is a display from Nextion. The code to run on the display is complete. It will communicate with the ESP via serial communication. 

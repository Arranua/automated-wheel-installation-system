# Automated Wheel Installation and Removal Cell

> PLC-coordinated automation project for removing wheels from a small vehicle chassis, feeding replacement wheels from magazines, picking them up with a gripper, and installing them onto axles.
>
> Will be displayed at the Summer 2026 Seneca TechSparks: Applied Technology Design Showcase


[![Automated Wheel Installation System Demo](https://img.youtube.com/vi/_Y8QUHR4chs/0.jpg)](https://www.youtube.com/watch?v=_Y8QUHR4chs)

https://www.youtube.com/watch?v=_Y8QUHR4chs

<img width="1603" height="826" alt="image" src="https://github.com/user-attachments/assets/e15f44a0-fc02-49ba-91d8-36f4d688364f" />
<img width="1603" height="826" alt="image" src="https://github.com/user-attachments/assets/6612b8b7-e2bd-4775-8660-401c4d973110" />
<img width="1603" height="826" alt="image" src="https://github.com/user-attachments/assets/118bb256-2a8a-455c-beff-9e50559b305c" />

<img width="1255" height="730" alt="image" src="https://github.com/user-attachments/assets/b9a8cfd3-b4f0-4266-a194-5f596e3387df" />
<img width="784" height="695" alt="image" src="https://github.com/user-attachments/assets/2cfb06b4-a30e-48e7-963c-851ce5514a7e" />
<img width="903" height="543" alt="image" src="https://github.com/user-attachments/assets/1704f746-11ca-431f-b262-99f5945a4733" />


## System Summary

The machine is a small manufacturing-style automation cell. A PLC coordinates the main sequence and communicates with ESP32-based motion controllers. The system uses stepper-driven axes, servo grippers, wheel magazine servos, photoelectric sensors, a capacitive sensor, relay interfaces, and a solenoid-actuated wheel release mechanism.

## Main Features

- PLC-based start, stop, reset, and sequence control
- ESP32 motion control for X/Y/Z stepper axes
- Servo gripper for wheel pickup and placement
- Separate wheel magazine controller
- Sensor-based car and wheel presence detection
- Start/done handshaking between PLC and controllers
- Software travel limits and homing routines
- Emergency stop / abort input to motion controllers
- Relay-isolated outputs for PLC and actuator interfaces

## Repository Map

| File / Folder | Purpose |
|---|---|
| `01_Project_Overview.md` | quick explanation of the machine and goals |
| `02_BOM_and_Parts.md` | bill of materials overview and parts notes |
| `03_System_Architecture.md` | controller, sensor, and actuator layout |
| `04_Electrical_Wiring.md` | wiring tables, pinouts, and power notes |
| `05_PLC_Sequence.md` | PLC state sequence and handshake logic |
| `06_Motion_Control.md` | ESP32 stepper/servo control overview |
| `07_Testing_Log.md` | testing, calibration, and troubleshooting notes |
| `08_Future_Work.md` | future improvements and industrial upgrades |
| `code/` | PLC and ESP32 code folders |
| `images/` | photos, CAD screenshots, wiring images, diagrams |
| `videos/` | operation clips and demo videos |
| `docs/` | extra documents such as BOM files, PDFs, and reports |

## Technologies Used

- PLC control
- ESP32 microcontrollers
- Stepper motors and stepper drivers
- Servo motors
- Capacitive and photoelectric sensors
- Relay / optocoupler signal interfaces
- 24 VDC solenoid actuation
- 3D printed mechanical parts
- Siemens NX for digital twin / CAD assembly
- Node-RED or IoT interface planned for demonstration requirements

## High-Level Sequence

1. Operator resets machine and sends system to home position.
2. Car body is detected in the loading area.
3. PLC checks sensor conditions and starts the motion sequence.
4. Gantry removes existing wheels.
5. Magazine system releases replacement wheel.
6. Gripper picks up replacement wheel.
7. Gantry installs wheel onto axle.
8. Process repeats for second wheel.
9. PLC receives done signal and marks cycle complete.


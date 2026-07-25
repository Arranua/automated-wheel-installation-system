# 06 - Motion Control

## Gantry Controller

The gantry controller is based on an ESP32. It controls three stepper axes and one servo gripper. The controller receives a start signal from the PLC, performs the required motion stage, and sends a done pulse back to the PLC.

## Axes

| Axis | Function |
|---|---|
| X | left/right travel between wheel positions and pickup zone |
| Y | forward/back motion into the vehicle axle or pickup location |
| Z | vertical or height positioning |

## Motion Features

- homing switches for each axis
- software travel limits
- manual jog commands for calibration
- staged movement sequence
- gripper open/close commands
- ESTOP/abort input
- done pulse output to PLC
- driver hold logic for axes that need to maintain position

## Magazine Controller

A second ESP32 controls the wheel magazine servos. The PLC selects or starts the required magazine action, then the ESP32 moves the selected servo through a dispense cycle.

## Calibration Notes

Calibration is performed by homing the machine, jogging to test coordinates, recording step positions, and updating the motion sequence. Manual jog commands make it possible to test one axis at a time instead of running the full cycle every time.

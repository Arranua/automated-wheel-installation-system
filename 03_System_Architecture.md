# 03 - System Architecture

## Controller Layout

```text
PLC
├── Operator start / stop / reset inputs
├── Car presence sensor
├── Wheel ready / pickup sensors
├── ESP32 gantry controller
│   ├── X stepper driver
│   ├── Y stepper driver
│   ├── Z stepper driver
│   └── gripper servo
├── ESP32 magazine controller
│   ├── magazine servo A
│   └── magazine servo B
└── solenoid relay / trapdoor actuator
```

## Control Philosophy

The PLC is the main coordinator. It validates sensor conditions and starts each subsystem. The ESP32 controllers handle timing-sensitive motion tasks and return a done signal when their assigned action is complete.

## Subsystems

| Subsystem | Main Job |
|---|---|
| PLC | main logic, sequence, start/stop/reset, interlocks |
| Gantry ESP32 | homing, stepper motion, gripper open/close, done pulse |
| Magazine ESP32 | selected wheel magazine actuation, done pulse |
| Sensors | confirm car/wheel presence and pickup readiness |
| Solenoid/trapdoor | release wheel into pickup zone |
| Mechanical frame | align vehicle, magazines, gantry, gripper, and pickup area |

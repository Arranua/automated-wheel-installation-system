# 05 - PLC Sequence

## PLC Role

The PLC controls the main process sequence, operator inputs, sensor checks, start commands, done feedback, and stop/reset behavior.

## Basic State Flow

```text
Power On
  ↓
Reset pressed
  ↓
Home position routine
  ↓
Wait for Start
  ↓
Check car and wheel sensors
  ↓
Start gantry / magazine actions
  ↓
Wait for done feedback
  ↓
Advance to next stage
  ↓
Cycle complete
```

## Planned States

| State | Description |
|---|---|
| Idle | waiting for reset/start |
| Reset/Home | machine returns to known reference position |
| Car Present Check | verifies vehicle is loaded |
| Wheel Ready Check | verifies wheel is available for pickup |
| Remove Wheels | gantry removes old wheels |
| Dispense Wheel | magazine releases replacement wheel |
| Pickup Wheel | gripper closes on wheel |
| Install Wheel | gantry pushes wheel onto axle |
| Cycle Complete | PLC receives final done signal |
| Stop/Fault | sequence stops and waits for reset |

## Handshake Concept

```text
PLC output START -> ESP32 input START
ESP32 runs motion sequence
ESP32 output DONE -> PLC input DONE
PLC advances to next state
```

This prevents the PLC from advancing before the motion subsystem has completed its assigned action.

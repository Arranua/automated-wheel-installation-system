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
<img width="1600" height="859" alt="image" src="https://github.com/user-attachments/assets/33a4cd66-f3ce-464b-a344-9d7351b4d156" />
<img width="1600" height="859" alt="image" src="https://github.com/user-attachments/assets/9c5fc6d4-7b25-4cdf-8347-34b112d3afea" />
<img width="1600" height="859" alt="image" src="https://github.com/user-attachments/assets/afc2d173-c14b-4d6b-a793-2729354c305a" />
<img width="1600" height="859" alt="image" src="https://github.com/user-attachments/assets/4baafd37-0ba3-4516-8c31-80510e76281b" />
<img width="1600" height="859" alt="image" src="https://github.com/user-attachments/assets/2e1704cb-e874-42f1-889f-5ae0aa649b00" />
<img width="1600" height="859" alt="image" src="https://github.com/user-attachments/assets/39ad25d6-33c4-47e0-9a39-4ddd2bc8aeb6" />


##  States

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

## Handshaking

```text
PLC output START -> ESP32 input START
ESP32 runs motion sequence
ESP32 output DONE -> PLC input DONE
PLC advances to next state
```

This prevents the PLC from advancing before the motion subsystem has completed its assigned action.

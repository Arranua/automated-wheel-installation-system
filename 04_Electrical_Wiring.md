# 04 - Electrical and Wiring

This page documents the project wiring.

## Power Distribution

| Voltage | Used For | Notes |
|---|---|---|
| 24 VDC | PLC, sensors, solenoid load | industrial control voltage |
| 5 VDC | servos / selected modules | use adequate current supply |
| 3.3 VDC | ESP32 GPIO logic | do not connect 24 V directly to GPIO |
| GND / 0 V | common DC reference | common ground for DC supplies where required |

<img width="1352" height="795" alt="image" src="https://github.com/user-attachments/assets/37e2e4c7-45ca-4836-b9a2-a963935192e8" />


## PLC Inputs 

| PLC Input | Device | Signal Type | Purpose |
|---|---|---|---|
| 1 | Stage Finished | relay/opto contact | confirms subsystem step complete |
| 2 | Start button | pushbutton | start cycle |
| 3 | Reset / stop input | pushbutton / safety input | reset or stop sequence |
| 4 | Car sensor | PNP capacitive | confirms car present |
| 5 | Magazine ready sensor | photoelectric | confirms wheel available |
| 7 | Pickup sensor | retroreflective / photoelectric | confirms wheel at pickup zone |

## PLC Outputs Draft

| PLC Output | Connected Device | Purpose |
|---|---|---|
| 1 | ESP32 gantry start input | starts gantry motion stage |
| 3 | ESP32 magazine start input | starts wheel feed action |
| 4 | Wheel type selector | tells magazine controller which wheel to feed |
| 6 | shared ESTOP / abort relay | sends abort signal to controllers |
| 8 | solenoid relay driver | opens trapdoor/drop gate |

## ESP32 Gantry Pinout

| ESP32 Pin | Signal | Purpose |
|---|---|---|
| GPIO 18 | X STEP | X-axis step pulses |
| GPIO 19 | X DIR | X-axis direction |
| GPIO 21 | X EN | X-axis driver enable |
| GPIO 22 | X HOME | X-axis home switch |
| GPIO 23 | Y STEP | Y-axis step pulses |
| GPIO 14 | Y DIR | Y-axis direction |
| GPIO 13 | Y EN | Y-axis driver enable |
| GPIO 4 | Y HOME | Y-axis home switch |
| GPIO 25 | Z STEP | Z-axis step pulses |
| GPIO 26 | Z DIR | Z-axis direction |
| GPIO 27 | Z EN | Z-axis driver enable |
| GPIO 32 | Z HOME | Z-axis home switch |
| GPIO 16 | gripper servo | servo control signal |
| GPIO 33 | PLC START | active-low start input |
| GPIO 17 | PLC DONE | done pulse output |
| GPIO 5 | ESTOP | active-low abort input |

<img width="913" height="799" alt="image" src="https://github.com/user-attachments/assets/dcb3a521-9f91-4bbb-9f8c-e537f965411f" />
<img width="1038" height="813" alt="image" src="https://github.com/user-attachments/assets/d7f50a7e-4a04-42e6-864b-582d0aaebfab" />
<img width="628" height="774" alt="image" src="https://github.com/user-attachments/assets/a0a21682-771e-4467-a640-f144b2fd18c8" />


## Solenoid Switching Note

The solenoid is treated as an inductive load. It should be switched through a properly rated relay or MOSFET driver, not directly from a controller signal pin. A flyback diode should be installed across the solenoid coil.

```text
Diode stripe/cathode -> solenoid positive
Diode non-stripe/anode -> solenoid negative
```

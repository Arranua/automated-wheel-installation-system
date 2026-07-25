# 07 - Testing Log and Troubleshooting

This page records problems found during development and how they were corrected.

## Issue: Axis Position Drift

**Problem:** One stepper axis could lose position or shift when the driver disabled after a move.

**Fix:** Updated the motion logic so the required axis stays enabled during the automatic cycle. Manual hold commands were also added for calibration.

**Result:** Improved repeatability and reduced position loss during staged motion.

## Issue: Solenoid Output Protection

**Problem:** The solenoid load required proper switching and protection. Signal-level interface boards are not the same as power drivers.

**Fix:** Use a relay or MOSFET driver rated for the solenoid current, and add a flyback diode across the solenoid coil.

**Result:** Safer switching of the 24 V solenoid and better protection for control electronics.

## Issue: Wheel Pickup Alignment

**Problem:** Free rolling or uncontrolled wheel pickup was unreliable.

**Fix:** The pickup concept was changed toward a guided drop/trapdoor approach, allowing the wheel to drop into a controlled gripper position.

**Result:** More repeatable wheel positioning for pickup.

## Issue: Sensor Type Selection

**Problem:** PNP/NPN sensor wiring had to match the PLC input style.

**Fix:** Sensor types and input common wiring were checked before finalizing the wiring plan.

**Result:** Reduced risk of incorrect sensor wiring and input logic problems.

## Testing To Add

- homing test video
- manual jog test video
- gripper open/close test
- wheel magazine dispense test
- solenoid trapdoor test
- full automatic cycle video

ADAS Level-1 Simulation (Win32 – C)

A Level-1 Advanced Driver Assistance Systems (ADAS) simulator built using C and the Win32 API, designed to demonstrate core vehicle safety and driver-assistance features through an interactive graphical interface.

This project simulates real-world ADAS behavior such as Forward Collision Warning, TPMS, door safety logic, lane change validation, and audible alerts, making it suitable for academic mini-projects, demonstrations, and learning Win32 GUI programming.

🚗 Project Overview

The ADAS Level-1 Simulation provides a dashboard-style interface where users can control vehicle parameters (speed, distance, tyre pressure, doors, indicators, etc.) and observe how ADAS features react in real time.

The focus is on:
Safety logic
Driver awareness
Human–Machine Interface (HMI) behavior
Event-based warnings

✨ Features Implemented
1. Forward Collision Warning (FCW):
Speed slider and front-distance slider
Adaptive FCW logic based on speed vs distance
Visual warnings on MID
High-priority audible alerts when collision risk is detected

2. Tyre Pressure Monitoring System (TPMS):
Base tyre pressure reference
Individual sliders for all four tyres
Automatic detection of under-inflated tyres
Warning indication on MID
Medium-priority warning beep

3. Vehicle State Controls (Toggle buttons):
Headlights ON/OFF
Day / Night mode
Hands-on steering detection
Left & Right indicators
Lane change request
Door obstacle detection
Each toggle updates the MID in real time.

4. Door Safety System
Individual door open/close buttons
Safety checks:
Doors cannot open while vehicle is moving
Doors blocked if obstacle is detected
Clear warning messages displayed on MID
Audible alerts for unsafe actions

5. Lane Change Assist Logic
Lane change allowed only if indicator is active
Warning shown if lane change is requested without indication
Mimics real ADAS driver-intent validation

6. Custom MID (Multi-Information Display)
Centralized display for:
Speed
Distance
TPMS status
Door states
ADAS warnings
Optimized size for clarity (not oversized)
Clean and readable layout

7. Audible Warning System
Different beep patterns for different priorities:
🔴 High priority: Collision warnings, unsafe actions
🟡 Medium priority: TPMS alerts
🔵 Low priority: Informational alerts
Implemented using Win32 Beep / sound logic.

🛠️ Technology Stack:
Language: C
Framework: Win32 API
Compiler: MinGW / MSVC
Platform: Windows

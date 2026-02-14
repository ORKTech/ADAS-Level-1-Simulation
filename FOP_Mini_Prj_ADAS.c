/* Title: FOP Mini Project - ADAS Level-1 Simulator
   Description: A Windows desktop application simulating a basic ADAS system with a custom MID display.
   Features:
   - Speed and front distance sliders with adaptive Forward Collision Warning (FCW).
   - Tyre Pressure Monitoring System (TPMS) with a base pressure and individual tyre sliders.
   - Toggle buttons for headlights, day/night mode, hands on steering, indicators, lane change request, and door obstacle.
   - Door open/close buttons with safety checks against obstacles and vehicle speed.
   - Custom MID display showing all relevant information and warnings in a clear format.
   - Audible beep patterns for different warning priorities.
   File Owner: Rahul Krishna
   Created: 2026-02-07
   File: FOP_Mini_Prj_ADAS.c
*/

// ---------------- INCLUDES AND DEFINES ----------------
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <math.h>

#pragma comment(lib, "comctl32.lib")

// ---------------- GLOBAL STATE & Variables ----------------
int speed = 0;
int frontDist = 50;
int basePressure = 32;
int tp[4] = { 32,32,32,32 };

BOOL headlights = FALSE;
BOOL nightMode = FALSE;
BOOL handsOn = TRUE;
BOOL leftInd = FALSE, rightInd = FALSE;
BOOL doorObstacle = FALSE;
BOOL laneChangeReq = FALSE;

wchar_t midWarnings[512];

// door state: 0=FL,1=FR,2=RL,3=RR
BOOL doorOpen[4] = { FALSE, FALSE, FALSE, FALSE };

// blinking state for indicators
BOOL blinkOn = FALSE;

// door open attempt blocked temporary warning (ms tick)
DWORD doorBlockWarnUntil = 0;

// lane message expiry tick
DWORD laneMsgUntil = 0;

// last beep time to avoid continuous beeps
DWORD lastBeepTime = 0;

// ---------------- CONTROL IDs ----------------
#define ID_SPEED      101
#define ID_FRONT      102
#define ID_BASETP     104
#define ID_TP1        105
#define ID_TP2        106
#define ID_TP3        107
#define ID_TP4        108

#define ID_HEADLIGHT  201
#define ID_DAYNIGHT   202
#define ID_HANDS      203
#define ID_LEFT       204
#define ID_RIGHT      205
#define ID_OBST       206
#define ID_LANE       207

// door buttons
#define ID_DOOR_FL    301
#define ID_DOOR_FR    302
#define ID_DOOR_RL    303
#define ID_DOOR_RR    304

// timers
#define IDT_BLINK     1001
#define IDT_LANE      1002

// button sizing (consistent)
#define BUTTON_W 140
#define BUTTON_H 40

// FCW cap (meters)
#define MAX_COLLISION_THRESHOLD 50

// vehicle physical length (m)
#define VEHICLE_LENGTH_M 5.0

// ---------------- FUNCTION DECLARATIONS ----------------
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawMID(HDC, RECT);
void AddWarning(const wchar_t*);
double StoppingDistance_m(int speed_kmh);
DWORD WINAPI BeepThreadProc(LPVOID lpParam);
void TriggerBeepForPriority(int priority);

// ---------------- ENTRY POINT ----------------
int WINAPI WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    LPSTR lpCmd,
    int nShow
) {
    INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_BAR_CLASSES };
    InitCommonControlsEx(&ic);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"ADAS";

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        L"ADAS", L"ADAS Level-1 Simulator",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1150, 760,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hwnd, nShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// ---------------- STOPPING DISTANCE ----------------
double StoppingDistance_m(int speed_kmh) {
    // Reaction time = 1.8s, braking distance estimate (mu ~0.7)
    double v = speed_kmh * 1000.0 / 3600.0; // m/s
    double reaction = 1.8; // seconds
    double mu = 0.7;
    double g = 9.81;
    double reaction_distance = v * reaction;
    double braking_distance = (v * v) / (2.0 * mu * g);
    return reaction_distance + braking_distance;
}

// ---------------- BEEP THREAD ----------------
DWORD WINAPI BeepThreadProc(LPVOID lpParam) {
    int priority = (int)(intptr_t)lpParam;
    // Patterns (non-blocking main thread). Use Beep + MessageBeep fallback for laptops.
    if (priority >= 3) {
        // High priority: three short high beeps; fallback to MessageBeep(MB_ICONHAND)
        Beep(1200, 150); Sleep(100);
        Beep(1200, 150); Sleep(100);
        Beep(1200, 150);
        MessageBeep(MB_ICONHAND);
    } else if (priority == 2) {
        // Medium: two medium beeps; fallback to MessageBeep(MB_ICONEXCLAMATION)
        Beep(900, 200); Sleep(150);
        Beep(900, 200);
        MessageBeep(MB_ICONEXCLAMATION);
    } else if (priority == 1) {
        // Low: single low beep; fallback to MessageBeep(MB_ICONASTERISK)
        Beep(700, 200);
        MessageBeep(MB_ICONASTERISK);
    }
    return 0;
}

void TriggerBeepForPriority(int priority) {
    // avoid continuous repetition: at least 800ms between beep bursts
    DWORD now = GetTickCount();
    if (priority <= 0) return;
    if (now - lastBeepTime < 800) return;
    lastBeepTime = now;
    // spawn thread to play beeps
    HANDLE h = CreateThread(NULL, 0, BeepThreadProc, (LPVOID)(intptr_t)priority, 0, NULL);
    if (h) CloseHandle(h);
}

// ---------------- WINDOW PROCEDURE ----------------
LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
) {
    static HWND hSpeed, hFront, hBase, hTP[4];
    static HWND hLeftBtn, hRightBtn;
    static HWND hDoorBtn[4];
    static HWND hHeadlightBtn, hDayNightBtn, hHandsBtn, hLaneBtn, hObstBtn;

    switch (msg) {
    case WM_CREATE:

        // SPEED
        CreateWindow(L"STATIC", L"Speed (km/h)",
            WS_CHILD | WS_VISIBLE, 20, 20, 120, 20,
            hwnd, NULL, NULL, NULL);

        hSpeed = CreateWindow(TRACKBAR_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
            20, 40, 260, 30,
            hwnd, (HMENU)ID_SPEED, NULL, NULL);
        SendMessage(hSpeed, TBM_SETRANGE, TRUE, MAKELONG(0, 180));
        SendMessage(hSpeed, TBM_SETTICFREQ, 5, 0);
        SendMessage(hSpeed, TBM_SETPOS, TRUE, 0);

        // FRONT Distance 
        CreateWindow(L"STATIC", L"Front Distance (m)",
            WS_CHILD | WS_VISIBLE, 20, 80, 160, 20,
            hwnd, NULL, NULL, NULL);
        hFront = CreateWindow(TRACKBAR_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
            20, 100, 260, 30,
            hwnd, (HMENU)ID_FRONT, NULL, NULL);
        SendMessage(hFront, TBM_SETRANGE, TRUE, MAKELONG(0, 50));
        SendMessage(hFront, TBM_SETTICFREQ, 5, 0);
        SendMessage(hFront, TBM_SETPOS, TRUE, frontDist);

        // TPMS
        CreateWindow(L"STATIC", L"Base Tyre Pressure (PSI)",
            WS_CHILD | WS_VISIBLE, 20, 160, 200, 20,
            hwnd, NULL, NULL, NULL);
        hBase = CreateWindow(TRACKBAR_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
            20, 180, 260, 30,
            hwnd, (HMENU)ID_BASETP, NULL, NULL);
        SendMessage(hBase, TBM_SETRANGE, TRUE, MAKELONG(20, 40));
        SendMessage(hBase, TBM_SETTICFREQ, 1, 0);
        SendMessage(hBase, TBM_SETPOS, TRUE, basePressure);

        for (int i = 0;i < 4;i++) {
            wchar_t lbl[20];
            wsprintf(lbl, L"Tyre %d", i + 1);
            CreateWindow(L"STATIC", lbl,
                WS_CHILD | WS_VISIBLE,
                20, 220 + i * 60, 100, 20,
                hwnd, NULL, NULL, NULL);

            hTP[i] = CreateWindow(TRACKBAR_CLASS, NULL,
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                20, 240 + i * 60, 260, 30,
                hwnd, (HMENU)(ID_TP1 + i), NULL, NULL);
            SendMessage(hTP[i], TBM_SETRANGE, TRUE, MAKELONG(20, 40));
            SendMessage(hTP[i], TBM_SETTICFREQ, 1, 0);
            SendMessage(hTP[i], TBM_SETPOS, TRUE, tp[i]);
        }

        // BUTTONS (consistent size + border)
        hHeadlightBtn = CreateWindow(L"BUTTON", L"Headlights",
            WS_CHILD | WS_VISIBLE | WS_BORDER, 320, 40, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_HEADLIGHT, NULL, NULL);

        hDayNightBtn = CreateWindow(L"BUTTON", L"Day / Night",
            WS_CHILD | WS_VISIBLE | WS_BORDER, 320, 90, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_DAYNIGHT, NULL, NULL);

        hHandsBtn = CreateWindow(L"BUTTON", L"Hands On Steering",
            WS_CHILD | WS_VISIBLE | WS_BORDER, 320, 140, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_HANDS, NULL, NULL);

        hLeftBtn = CreateWindow(L"BUTTON", L"Left Indicator",
            WS_CHILD | WS_VISIBLE | WS_BORDER, 320, 190, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_LEFT, NULL, NULL);

        hRightBtn = CreateWindow(L"BUTTON", L"Right Indicator",
            WS_CHILD | WS_VISIBLE | WS_BORDER, 320, 240, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_RIGHT, NULL, NULL);

        hLaneBtn = CreateWindow(L"BUTTON", L"Lane Change",
            WS_CHILD | WS_VISIBLE | WS_BORDER, 320, 290, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_LANE, NULL, NULL);

        hObstBtn = CreateWindow(L"BUTTON", L"Door Obstacle",
            WS_CHILD | WS_VISIBLE | WS_BORDER, 320, 340, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_OBST, NULL, NULL);

		// Door buttons (4) — placed under tyre sliders as 2x2 grid for better UI organization
        int doorBaseX = 20;
        int doorBaseY = 240 + 4 * 60 + 10; // adjusted for TPMS layout
        int doorGapX = BUTTON_W + 10;
        int doorGapY = BUTTON_H + 10;

        hDoorBtn[0] = CreateWindow(L"BUTTON", L"Front Left",
            WS_CHILD | WS_VISIBLE | WS_BORDER, doorBaseX, doorBaseY, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_DOOR_FL, NULL, NULL);
        hDoorBtn[1] = CreateWindow(L"BUTTON", L"Front Right",
            WS_CHILD | WS_VISIBLE | WS_BORDER, doorBaseX + doorGapX, doorBaseY, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_DOOR_FR, NULL, NULL);
        hDoorBtn[2] = CreateWindow(L"BUTTON", L"Rear Left",
            WS_CHILD | WS_VISIBLE | WS_BORDER, doorBaseX, doorBaseY + doorGapY, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_DOOR_RL, NULL, NULL);
        hDoorBtn[3] = CreateWindow(L"BUTTON", L"Rear Right",
            WS_CHILD | WS_VISIBLE | WS_BORDER, doorBaseX + doorGapX, doorBaseY + doorGapY, BUTTON_W, BUTTON_H,
            hwnd, (HMENU)ID_DOOR_RR, NULL, NULL);

        // start blink timer for indicators
        //SetTimer(hwnd, IDT_BLINK, 500, NULL);

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_HEADLIGHT: headlights = !headlights; InvalidateRect(hwnd, NULL, TRUE); break;
        case ID_DAYNIGHT: nightMode = !nightMode; InvalidateRect(hwnd, NULL, TRUE); break;
        case ID_HANDS: handsOn = !handsOn; InvalidateRect(hwnd, NULL, TRUE); break;

        case ID_LEFT:
            leftInd = !leftInd;
            if (leftInd) rightInd = FALSE;
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case ID_RIGHT:
            rightInd = !rightInd;
            if (rightInd) leftInd = FALSE;
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case ID_OBST: doorObstacle = !doorObstacle; InvalidateRect(hwnd, NULL, TRUE); break;

        case ID_LANE:
            // ensure message stays visible at least 1 second
            laneMsgUntil = GetTickCount() + 1000;
            laneChangeReq = TRUE;
            // start a short timer to drive updates while message is active
            SetTimer(hwnd, IDT_LANE, 200, NULL);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        // Door button clicks — block opening if obstacle present or vehicle moving
        case ID_DOOR_FL:
        case ID_DOOR_FR:
        case ID_DOOR_RL:
        case ID_DOOR_RR: {
            int doorIndex = -1;
            if (LOWORD(wParam) == ID_DOOR_FL) doorIndex = 0;
            if (LOWORD(wParam) == ID_DOOR_FR) doorIndex = 1;
            if (LOWORD(wParam) == ID_DOOR_RL) doorIndex = 2;
            if (LOWORD(wParam) == ID_DOOR_RR) doorIndex = 3;

            if (doorIndex >= 0) {
                // trying to open?
                BOOL tryingToOpen = !doorOpen[doorIndex];
                if (tryingToOpen && (doorObstacle || speed > 0)) {
                    // block the open and show temporary warning
                    doorBlockWarnUntil = GetTickCount() + 2000; // show for 2s
                    // do NOT change doorOpen[doorIndex]
                } else {
                    // allowed to toggle
                    doorOpen[doorIndex] = !doorOpen[doorIndex];
                }
            }
            InvalidateRect(hwnd, NULL, TRUE);
        } break;

        }
        break;

    case WM_HSCROLL: {
        HWND src = (HWND)lParam;
        // update basic values
        speed = SendMessage(hSpeed, TBM_GETPOS, 0, 0);
        frontDist = SendMessage(hFront, TBM_GETPOS, 0, 0);

        // if base TPMS changed -> sync all tyre sliders
        if (src == hBase) {
            basePressure = SendMessage(hBase, TBM_GETPOS, 0, 0);
            // sync other tyre sliders immediately
            for (int i = 0; i < 4; ++i) {
                SendMessage(hTP[i], TBM_SETPOS, TRUE, basePressure);
                tp[i] = basePressure;
            }
        } else {
            // otherwise read base and individual tyres
            basePressure = SendMessage(hBase, TBM_GETPOS, 0, 0);
            for (int i = 0; i < 4; ++i) {
                tp[i] = SendMessage(hTP[i], TBM_GETPOS, 0, 0);
            }
        }
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    case WM_TIMER:
        if (wParam == IDT_BLINK) {
            blinkOn = !blinkOn;
            InvalidateRect(hwnd, NULL, TRUE);
        } else if (wParam == IDT_LANE) {
            // expire lane message after laneMsgUntil
            if (GetTickCount() >= laneMsgUntil) {
                laneChangeReq = FALSE;
                KillTimer(hwnd, IDT_LANE);
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // increase MID size
        RECT mid = { 560,60,1140,680 };
        DrawMID(hdc, mid);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        KillTimer(hwnd, IDT_BLINK);
        KillTimer(hwnd, IDT_LANE);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ---------------- WARNING HANDLER ----------------
void AddWarning(const wchar_t* w) {
    wcscat_s(midWarnings, 512, w);
}

// ---------------- MID DRAW ----------------
void DrawMID(HDC hdc, RECT r) {
    midWarnings[0] = L'\0';

    int highestPriority = 0; // 0 none, 1 low, 2 medium, 3 high

    // HEADLIGHT warnings: use the day/night switch (nightMode) rather than local time
    if (nightMode && !headlights) {
        AddWarning(L"⚠ Headlights OFF (night)\n");
        if (highestPriority < 2) highestPriority = 2;
    } else if (!nightMode && headlights) {
        AddWarning(L"⚠ Headlights ON (day)\n");
        if (highestPriority < 1) highestPriority = 1;
    }

    // FCW adaptive threshold based on current speed (reaction 2.5s) + vehicle length
    double stopping = StoppingDistance_m(speed);
    double threshold_d = stopping + VEHICLE_LENGTH_M; // include vehicle length
    int adaptiveThreshold = (int)ceil(threshold_d + 0.5); // round up with small margin
    if (adaptiveThreshold > MAX_COLLISION_THRESHOLD)
        adaptiveThreshold = MAX_COLLISION_THRESHOLD;

    // Forward Collision Warning (adaptive) — high priority
    if (frontDist < adaptiveThreshold) {
        wchar_t tmp[128];
        wsprintf(tmp, L"⚠ Forward Collision Warning (threshold %d m)\n", adaptiveThreshold);
        AddWarning(tmp);
        highestPriority = 3;
    }

    // Tyre and other medium/low warnings
    for (int i = 0;i < 4;i++) {
        if (tp[i] < basePressure - 4) {
            AddWarning(L"⚠ Low Tyre Pressure\n");
            if (highestPriority < 2) highestPriority = 2;
        }
    }

    if (!handsOn) {
        AddWarning(L"⚠ Hands Off Steering\n");
        if (highestPriority < 2) highestPriority = 2;
    }

    // door-related warnings
    BOOL anyDoorOpen = FALSE;
    for (int i = 0; i < 4; ++i) if (doorOpen[i]) anyDoorOpen = TRUE;

    if (anyDoorOpen) {
        if (speed > 0) {
            AddWarning(L"⚠ Door Open While Moving\n");
            if (highestPriority < 3) highestPriority = 3;
        }
        if (doorObstacle) {
            AddWarning(L"⚠ Exit Warning: Obstacle Detected - Close Door\n");
            if (highestPriority < 3) highestPriority = 3;
        }
    }

    // door open attempts blocked -> show temporary warning
    if (doorBlockWarnUntil > GetTickCount()) {
        AddWarning(L"⚠ Door opening blocked: obstacle or vehicle moving\n");
        if (highestPriority < 2) highestPriority = 2;
    }

    // lane change: only show warning when unsafe (do not display safe-to-change)
    if (laneChangeReq) {
        if (!(leftInd || rightInd)) {
            AddWarning(L"⚠ Lane Change! Please Use indicator\n");
            if (highestPriority < 1) highestPriority = 1;
        }
    }

    // If there is a warning, trigger an audible beep pattern based on highestPriority
    if (highestPriority > 0) {
        TriggerBeepForPriority(highestPriority);
    }

    // Draw MID: header in green, warnings in orange-red
    HBRUSH bg = CreateSolidBrush(RGB(10, 10, 10));
    FillRect(hdc, &r, bg);
    DeleteObject(bg);

    SetBkMode(hdc, TRANSPARENT);

    HFONT f = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Consolas");
    SelectObject(hdc, f);

    // header (everything except midWarnings)
    wchar_t doorState[256];
    wsprintf(doorState,
        L"Doors: FL:%s FR:%s RL:%s RR:%s\nIndicators: %s %s\n\n",
        doorOpen[0] ? L"OPEN" : L"CLOSED",
        doorOpen[1] ? L"OPEN" : L"CLOSED",
        doorOpen[2] ? L"OPEN" : L"CLOSED",
        doorOpen[3] ? L"OPEN" : L"CLOSED",
        leftInd ? L"LEFT" : L"-",
        rightInd ? L"RIGHT" : L"-"
    );

    wchar_t header[1536];
    wsprintf(header,
        L"           RK\n"
        L"----------------------------------------\n\n"
        L"Speed: %d km/h\n"
        L"Front: %d m\n\n"
        L"TPMS Base: %d PSI\n"
        L"T1:%d  T2:%d  T3:%d  T4:%d\n\n"
        L"Headlights: %s | Mode: %s\n"
        L"Hands On Steering: %s\n\n"
        L"FCW Threshold: %d m (capped at %d m)\n\n"
        L"Obstacles near Door: %s\n"
        L"%s",
        speed, frontDist,
        basePressure, tp[0], tp[1], tp[2], tp[3],
        headlights ? L"ON" : L"OFF",
        nightMode ? L"NIGHT" : L"DAY",
        handsOn ? L"YES" : L"NO",
        adaptiveThreshold, MAX_COLLISION_THRESHOLD,
        doorObstacle ? L"ON" : L"OFF",
        doorState
    );

    // Calculate header size
    RECT calcHdr = r;
    DrawText(hdc, header, -1, &calcHdr, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_CALCRECT);
    int headerHeight = calcHdr.bottom - calcHdr.top;
    if (headerHeight < 20) headerHeight = 20;

    // Draw header in green using the calculated height
    SetTextColor(hdc, RGB(0, 255, 0));
    RECT drawHdr = r;
    drawHdr.bottom = drawHdr.top + headerHeight;
    DrawText(hdc, header, -1, &drawHdr, DT_LEFT | DT_TOP | DT_WORDBREAK);

    // now draw warnings below header in orange-red
    RECT warnRect;
    warnRect.left = r.left + 4;
    warnRect.right = r.right - 4;
    warnRect.top = drawHdr.bottom + 8; // small gap
    warnRect.bottom = r.bottom - 8;

    SetTextColor(hdc, RGB(255, 80, 0)); // orange-red for warnings

    // Draw WARNINGS header (measure then draw)
    RECT warnHeaderCalc = warnRect;
    DrawText(hdc, L"--- WARNINGS ---\n", -1, &warnHeaderCalc, DT_LEFT | DT_TOP | DT_CALCRECT);
    DrawText(hdc, L"--- WARNINGS ---\n", -1, &warnRect, DT_LEFT | DT_TOP);
    // advance top by calculated size of the WARNINGS header
    warnRect.top += (warnHeaderCalc.bottom - warnHeaderCalc.top);

    // draw the collected midWarnings (wordwrap as some warning are execeding the limit of mid)
    DrawText(hdc, midWarnings, -1, &warnRect, DT_LEFT | DT_TOP | DT_WORDBREAK);

    DeleteObject(f);
}
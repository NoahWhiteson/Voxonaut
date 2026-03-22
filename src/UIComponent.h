#ifndef UI_COMPONENT_H
#define UI_COMPONENT_H

#include "raylib.h"
#include <string>
#include <vector>
#include <cmath>

class UIComponent {
public:
    enum WindowMode { MODE_WINDOWED, MODE_BORDERLESS, MODE_FULLSCREEN };
    enum Screen { MENU, SETTINGS, EXPLORE, BUILD };

    Screen currentScreen = MENU;
    WindowMode currentWinMode = MODE_WINDOWED;
    float fadeAlpha = 1.0f;
    float menuTime = 0.0f;

    void Draw(float dt, int screenWidth, int screenHeight, float orbitAlt) {
        menuTime += dt;
        float scale = (float)screenHeight / 600.0f;
        for (int i = 0; i < screenHeight; i += (int)(4 * scale))
            DrawRectangle(0, i, screenWidth, (int)(1 * scale), Fade(SKYBLUE, 0.03f));
        switch (currentScreen) {
            case MENU:     DrawMainMenu(screenWidth, screenHeight, scale); break;
            case SETTINGS: DrawSettings(screenWidth, screenHeight, scale); break;
            case EXPLORE:  DrawHUD(screenWidth, screenHeight, scale, orbitAlt, false); break;
            case BUILD:    DrawHUD(screenWidth, screenHeight, scale, orbitAlt, true); break;
        }
    }

private:
    void DrawMainMenu(int sw, int sh, float scale) {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.4f));
        Color titleCol = (currentWinMode == MODE_FULLSCREEN) ? GOLD : (currentWinMode == MODE_BORDERLESS ? SKYBLUE : LIGHTGRAY);
        float floatAnim = sinf(menuTime * 1.5f) * (15.0f * scale);
        int titleSize = (int)(70 * scale);
        DrawText("VOXONAUT", sw/2 - MeasureText("VOXONAUT", titleSize)/2 + (int)(4*scale), (int)(120*scale) + (int)floatAnim + (int)(4*scale), titleSize, Fade(DARKBLUE, 0.5f));
        DrawText("VOXONAUT", sw/2 - MeasureText("VOXONAUT", titleSize)/2, (int)(120*scale) + (int)floatAnim, titleSize, titleCol);
        DrawRectangle(sw/2 - (int)(120*scale), (int)(205*scale), (int)(240*scale), (int)(2*scale), titleCol);
        DrawText("PLANETARY ENGINEERING INITIATIVE", sw/2 - MeasureText("PLANETARY ENGINEERING INITIATIVE", (int)(16*scale))/2, (int)(215*scale), (int)(16*scale), GRAY);
        const char* modeText = (currentWinMode == MODE_FULLSCREEN) ? "PROTOCOL: HIGH-PERFORMANCE FULLSCREEN" : (currentWinMode == MODE_BORDERLESS ? "PROTOCOL: IMMERSIVE BORDERLESS" : "PROTOCOL: STANDARD WINDOWED");
        DrawText(modeText, sw/2 - MeasureText(modeText, (int)(12*scale))/2, (int)(240*scale), (int)(12*scale), Fade(titleCol, 0.6f));
        if (Button(sw/2 - (int)(110*scale), (int)(300*scale), (int)(220*scale), (int)(50*scale), "INITIALIZE", scale)) currentScreen = EXPLORE;
        if (Button(sw/2 - (int)(110*scale), (int)(365*scale), (int)(220*scale), (int)(50*scale), "CONFIG", scale)) currentScreen = SETTINGS;
        if (Button(sw/2 - (int)(110*scale), (int)(430*scale), (int)(220*scale), (int)(50*scale), "TERMINATE", scale)) CloseWindow();
    }

    void DrawSettings(int sw, int sh, float scale) {
        DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.8f));
        int titleSize = (int)(30 * scale);
        DrawText("SYSTEM CONFIGURATION", sw/2 - MeasureText("SYSTEM CONFIGURATION", titleSize)/2, (int)(100*scale), titleSize, SKYBLUE);
        if (Button(sw/2 - (int)(120*scale), (int)(200*scale), (int)(240*scale), (int)(45*scale), "WINDOWED",   scale, currentWinMode == MODE_WINDOWED))   SetMode(MODE_WINDOWED);
        if (Button(sw/2 - (int)(120*scale), (int)(260*scale), (int)(240*scale), (int)(45*scale), "BORDERLESS", scale, currentWinMode == MODE_BORDERLESS)) SetMode(MODE_BORDERLESS);
        if (Button(sw/2 - (int)(120*scale), (int)(320*scale), (int)(240*scale), (int)(45*scale), "FULLSCREEN", scale, currentWinMode == MODE_FULLSCREEN)) SetMode(MODE_FULLSCREEN);
        if (Button(sw/2 - (int)(110*scale), (int)(450*scale), (int)(220*scale), (int)(50*scale), "BACK", scale)) currentScreen = MENU;
    }

    void DrawHUD(int sw, int sh, float scale, float alt, bool isBuild) {
        Rectangle panel = { 20*scale, 20*scale, 240*scale, 60*scale };
        DrawRectangleRounded(panel, 0.2f, 8, Fade(DARKBLUE, 0.3f));
        DrawRectangleRoundedLines(panel, 0.2f, 8, Fade(SKYBLUE, 0.5f));
        DrawText(isBuild ? "ENGINEERING DECK" : "FLIGHT BRIDGE", (int)(40*scale), (int)(33*scale), (int)(18*scale), WHITE);
        DrawText(isBuild ? "ASSEMBLING CLASS-C" : "ORBITAL VELOCITY: STABLE", (int)(40*scale), (int)(55*scale), (int)(12*scale), GRAY);
        Rectangle tele = { 20*scale, (float)sh - 80*scale, 240*scale, 60*scale };
        DrawRectangleRounded(tele, 0.2f, 8, Fade(BLACK, 0.4f));
        DrawText(TextFormat("ALTITUDE: %0.f KM", alt/100.0f), (int)(40*scale), (int)(sh - 65*scale), (int)(16*scale), WHITE);
        DrawRectangle((int)(40*scale), (int)(sh - 40*scale), (int)(200*scale), (int)(4*scale), DARKGRAY);
        DrawRectangle((int)(40*scale), (int)(sh - 40*scale), (int)(fminf(alt/2000.0f, 1.0f)*200*scale), (int)(4*scale), SKYBLUE);
    }

    bool Button(int x, int y, int w, int h, const char* label, float scale, bool active = false) {
        Rectangle r = { (float)x, (float)y, (float)w, (float)h };
        bool over = CheckCollisionPointRec(GetMousePosition(), r);
        DrawRectangleRounded(r, 0.15f, 6, Fade(active ? SKYBLUE : (over ? BLUE : DARKGRAY), 0.4f));
        DrawRectangleRoundedLines(r, 0.15f, 6, Fade(WHITE, over ? 0.8f : 0.3f));
        int fontSize = (int)(20 * scale);
        DrawText(label, x + w/2 - MeasureText(label, fontSize)/2, y + h/2 - fontSize/2, fontSize, WHITE);
        return (over && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
    }

    void SetMode(WindowMode mode) {
        currentWinMode = mode;
        int monitor = GetCurrentMonitor();
        int mw = GetMonitorWidth(monitor);
        int mh = GetMonitorHeight(monitor);
        if (mode == MODE_WINDOWED) {
            if (IsWindowFullscreen()) ToggleFullscreen();
            if (IsWindowState(FLAG_WINDOW_UNDECORATED)) ClearWindowState(FLAG_WINDOW_UNDECORATED);
            SetWindowSize(800, 600); SetWindowPosition(100, 100);
        } else if (mode == MODE_BORDERLESS) {
            SetWindowSize(mw, mh); SetWindowPosition(0, 0);
            if (!IsWindowState(FLAG_WINDOW_UNDECORATED)) SetWindowState(FLAG_WINDOW_UNDECORATED);
        } else if (mode == MODE_FULLSCREEN) {
            if (IsWindowState(FLAG_WINDOW_UNDECORATED)) ClearWindowState(FLAG_WINDOW_UNDECORATED);
            if (!IsWindowFullscreen()) ToggleFullscreen();
            SetWindowSize(mw, mh);
        }
    }
};

#endif

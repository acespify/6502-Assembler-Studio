// ============================================================================
// Copyright (c) 2026 Andrew Young
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
// ============================================================================

#pragma once
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"
#include <string>
#include <vector>
#include <cstdint>

struct GLFWwindow;

class MainWindow {
public:
    MainWindow(int width, int height, const char* title);
    ~MainWindow();
    bool Init();
    void Run();

private:
    void Shutdown();
    void RenderUI();

    // --- File Operations ---
    void SaveSource(const char* filepath);
    void LoadSource(const char* filepath);

    // --- Window State ---
    GLFWwindow* m_Window;
    int m_Width;
    int m_Height;
    const char* m_Title;

    // --- Editor & File State ---
    char m_EditorBuffer[1024 * 64] = "";       // 64KB Buffer for code
    char m_FilenameBuffer[256] = "main.s";     // Default assembly filename
    char m_RomFilenameBuffer[256] = "rom.bin"; // Default output ROM filename
    char m_StatusMessage[256] = "";            // UI Status bar text
    float m_StatusTimer = 0.0f;                // Timer to fade out status message

    // --- Hex Dump State ---
    char m_JumpAddressBuffer[8] = ""; // Buffer for the jump input
    bool m_DoJump = false;            // Flag to trigger the scroll

    // --- Build Results ---
    std::string m_LastLog = "";                // Output log from the Assembler
    std::vector<uint8_t> m_LastRom;            // Binary data for the Hex Dump view

    // --- Splash Screen State ---
    bool m_ShowSplash = true;                  // Show on startup
    float m_SplashTimer = 3.0f;                // Duration in seconds
    unsigned int m_SplashTexture = 0;          // OpenGL Texture ID for splash
    int m_SplashWidth = 0;
    int m_SplashHeight = 0;
};
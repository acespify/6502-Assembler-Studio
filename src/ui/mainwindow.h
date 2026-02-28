// ============================================================================
// Copyright (c) 2026 Andrew Young
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
// ============================================================================

#pragma once
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileDialogConfig.h"
#include "TextEditor.h"
#include <string>
#include <vector>
#include <cstdint>

struct GLFWwindow;

// ============================================================================
// CLASS: MainWindow
// PURPOSE: Acts as the primary frontend controller for the 6502 Assembler Studio.
//          Manages the application lifecycle, OpenGL/GLFW context, ImGui rendering,
//          file I/O operations, and user configuration persistence.
// ============================================================================
class MainWindow {
public:
    // Lifecycle Methods
    MainWindow(int width, int height, const char* title);
    ~MainWindow();

    // Initializes windowing, graphics contexts, and the text editor syntax rules.
    bool Init();

    // The main application loop. Handles frame timing, polling, and rendering.
    void Run();

private:
    // Core Sub-Systems

    // Safely tears down ImGui, OpenGL, and GLFW before the application exits.
    void Shutdown();

    // Safely tears down ImGui, OpenGL, and GLFW before the application exits.
    void RenderUI();

    // UI Component Functions
    void DrawMenuBar();               // Top menu (File, View, Version info)
    void DrawToolbar();               // Quick-access inputs (Filename, Save/Load buttons, Status)
    void DrawEditorPane(float width, float height); // The 6502 Syntax Text Editor
    void DrawHexDumpPane(float height);             // The live RAM/ROM memory monitor
    void DrawLogAndControlPane();                   // Target selector, Build button, and Compiler Logs
    void DrawColorSettingsWindow();                 // Floating modal for tweaking editor theme colors
    void DrawDialogs();                             // Handles asynchronous File Browser popups and Unsaved warnings
    void DrawSplashOverlay();                       // Startup logo drawn to the foreground

    // --- File Operations ---
    // Saves the current text editor buffer to the disk.
    void SaveSource(const char* filepath);

    // Reads a file from the disk and populates the text editor buffer.
    void LoadSource(const char* filepath);

    // Safely clears the editor buffer and resets the current active filename.
    void CloseFile();

    // --- Editor Settings & JSON Config ---
    void LoadEditorSettings();
    void SaveEditorSettings();
    void ApplyEditorColors();

    // --- Window State ---
    GLFWwindow* m_Window;
    int m_Width;
    int m_Height;
    const char* m_Title;

    // --- Active Color Theme Data ---
    float m_ColText[4]    = { 0.9f, 0.9f, 0.9f, 1.0f }; // White/Grey
    float m_ColKeyword[4] = { 0.3f, 0.6f, 0.9f, 1.0f }; // Blue
    float m_ColNumber[4]  = { 0.5f, 0.9f, 0.5f, 1.0f }; // Green
    float m_ColComment[4] = { 0.4f, 0.4f, 0.4f, 1.0f }; // Dark Grey
    float m_ColBg[4]      = { 0.1f, 0.1f, 0.1f, 1.0f }; // Dark Background
    float m_ColVariable[4]  = { 0.9f, 0.9f, 0.5f, 1.0f }; // Yellow
    float m_ColLabel[4]     = { 0.8f, 0.4f, 0.8f, 1.0f }; // Purple/Pink
    float m_ColLineNumber[4] = { 0.4f, 0.4f, 0.4f, 1.0f }; // Grey for Line Numbers
    bool m_ShowWhitespaces = false;

    // --- Editor & File State ---
    TextEditor m_TextEditor;                    // The Pro ImGui Code Editor instance
    TextEditor::Palette m_EditorPalette;        // Stores the active color theme
    std::string m_LoadedText  ="";              // Tracks the baseline text to detect unsaved changes
    bool m_WantsToClose = false;                // Trigger for the ImGui modal popup
    bool m_CloseAfterSave = false;              // Flag to close the file after the async Save Dialog finishes
    bool m_ShowColorSettings = false;           // Toggles the settings window

    // --- UI Strings & Status ---
    char m_FilenameBuffer[256] = "main.s";      // Default assembly filename
    char m_RomFilenameBuffer[256] = "rom.bin";  // Default output ROM filename
    char m_StatusMessage[256] = "";             // UI Status bar text
    float m_StatusTimer = 0.0f;                 // Timer to fade out status message

    // --- Hex Dump State ---
    char m_JumpAddressBuffer[8] = "";           // Buffer for the jump input
    bool m_DoJump = false;                      // Flag to trigger the scroll

    // --- Build Results ---
    std::string m_LastLog = "";                 // Output log from the Assembler
    std::vector<uint8_t> m_LastRom;             // Binary data for the Hex Dump view
    int m_RomSizeIdx = 1;                       // Target hardware size dropdown state
    
    // --- Splash Screen State ---
    bool m_ShowSplash = true;                   // Show on startup
    float m_SplashTimer = 3.0f;                 // Duration in seconds
    unsigned int m_SplashTexture = 0;           // OpenGL Texture ID for splash
    int m_SplashWidth = 0;
    int m_SplashHeight = 0;
    
};
// ============================================================================
// Copyright (c) 2026 Andrew Young
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
// ============================================================================

#include "MainWindow.h"
#include "assembler.h" // Your custom assembler class
#include "version.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "../vendor/GLFW/include/GLFW/glfw3.h"

#include <iostream>
#include <fstream>
#include <sstream>

// Include stb_image for loading PNGs
#include "../../vendor/stb_image/stb_image.h"

// ============================================================================
// FUNCTION: LoadTextureFromFile
// PURPOSE: 
//   A helper function to load an image file (like a PNG) from the hard drive 
//   and convert it into a format that OpenGL and ImGui can draw to the screen.
//
// HOW IT WORKS:
//   - Uses the `stb_image` library to read the image pixels and dimensions.
//   - Generates an OpenGL Texture ID (`glGenTextures`) and uploads the pixel 
//     data to the GPU (`glTexImage2D`).
//   - Frees the RAM used by the raw image once it's safely on the graphics card.
// ============================================================================
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) return false;

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;
    return true;
}

// ============================================================================
// CONSTRUCTOR / INIT / SHUTDOWN
// ============================================================================

MainWindow::MainWindow(int width, int height, const char* title)
    : m_Width(width), m_Height(height), m_Title(title) {
    //Init();
}

MainWindow::~MainWindow() {
    Shutdown();
}


// ============================================================================
// FUNCTION: MainWindow::Init
// PURPOSE: 
//   Sets up the underlying windowing system (GLFW), the graphics context 
//   (OpenGL), and the user interface library (ImGui).
//
// HOW IT WORKS:
//   - Initializes GLFW and creates the main application window.
//   - Loads the custom window icon and splash screen assets from the `assets/` folder.
//   - Initializes the ImGui context, applies the dark theme, and connects 
//     ImGui to the GLFW window and OpenGL renderer.
// ============================================================================
bool MainWindow::Init() {
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(1); // Enable VSync

    // --- 1. Load Window Icon (Taskbar/Titlebar) ---
    GLFWimage images[1];
    images[0].pixels = stbi_load("assets/icon6502.png", &images[0].width, &images[0].height, 0, 4);
    if (images[0].pixels) {
        glfwSetWindowIcon(m_Window, 1, images);
        stbi_image_free(images[0].pixels);
    } else {
        std::cerr << "Warning: Could not load assets/icon6502.png" << std::endl;
    }

    // --- 2. Load Splash Image Texture ---
    // Adjust this to "assets/Splashimage.png" if that's what your file is named
    if (!LoadTextureFromFile("assets/Splashimage.png", &m_SplashTexture, &m_SplashWidth, &m_SplashHeight)) {
        std::cerr << "Warning: Could not load assets/Splashimage.png" << std::endl;
        m_ShowSplash = false; // Skip splash if missing
    }

    // --- ImGui Setup ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2; // Makes the font render smoother
    fontConfig.OversampleV = 2;
    // Load directly from the Windows font directory at size 16
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 16.0f, &fontConfig);

    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
}

void MainWindow::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyPlatformWindows();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

// ============================================================================
// FILE IO
// ============================================================================

void MainWindow::SaveSource(const char* filepath) {
    std::ofstream out(filepath);
    if (out.is_open()) {
        out << m_EditorBuffer;
        out.close();
        snprintf(m_StatusMessage, sizeof(m_StatusMessage), "Saved %s", filepath);
        strncpy(m_FilenameBuffer, filepath, sizeof(m_FilenameBuffer) - 1);
    } else {
        snprintf(m_StatusMessage, sizeof(m_StatusMessage), "Error saving %s", filepath);
    }
    m_StatusTimer = 3.0f;
}

void MainWindow::LoadSource(const char* filepath) {
    std::ifstream in(filepath);
    if (in.is_open()) {
        std::stringstream buffer;
        buffer << in.rdbuf();
        std::string content = buffer.str();
        if (content.length() < sizeof(m_EditorBuffer)) {
            strcpy(m_EditorBuffer, content.c_str());
            snprintf(m_StatusMessage, sizeof(m_StatusMessage), "Loaded %s", filepath);
            strncpy(m_FilenameBuffer, filepath, sizeof(m_FilenameBuffer) - 1);
        } else {
            snprintf(m_StatusMessage, sizeof(m_StatusMessage), "File too large!");
        }
    } else {
        snprintf(m_StatusMessage, sizeof(m_StatusMessage), "Error loading %s", filepath);
    }
    m_StatusTimer = 3.0f;
}

// ============================================================================
// MAIN LOOP & RENDER
// ============================================================================

void MainWindow::Run() {
    while (!glfwWindowShouldClose(m_Window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(m_Window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        // Standard App Background Color (Dark Grey)
        glClearColor(0.10f, 0.10f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_Window);
    }
}


// ============================================================================
// FUNCTION: MainWindow::RenderUI
// PURPOSE: 
//   The heart of the frontend. This function dictates exactly what buttons, 
//   text boxes, and windows appear on the screen every single frame.
//
// HOW IT WORKS:
//   - Top Menu & Toolbar: Draws the File menu and the quick-save/load buttons.
//   - Editor Pane: Renders the multi-line text input for writing assembly code.
//   - Hex Dump Pane: Calculates scrolling offsets and draws the compiled ROM 
//     bytes in a 16-column grid.
//   - Build Log Pane: Displays the output messages from the Assembler.
//   - Splash Screen: If the splash timer is active, it draws the splash image 
//     in the foreground, bypassing standard window layouts.
// ============================================================================
void MainWindow::RenderUI() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    // ========================================================================
    // 1. MAIN APPLICATION INTERFACE (Always renders in the background)
    // ========================================================================
    
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("Assembler Studio", nullptr, 
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
    );

    // --- A. MENU BAR ---
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            if (ImGui::MenuItem("Save", "Ctrl+S")) { ImGuiFileDialog::Instance()->OpenDialog("SaveSourceDlg", "Save Source File", ".s,.asm,.txt,.*", config); }
            if (ImGui::MenuItem("Load", "Ctrl+O")) { ImGuiFileDialog::Instance()->OpenDialog("ChooseSourceDlg", "Choose Source File", ".s,.asm,.txt,.*", config); }
            if (ImGui::MenuItem("Exit")) { glfwSetWindowShouldClose(glfwGetCurrentContext(), true); }
            ImGui::EndMenu();
        }

        // --- Version Display ---
        ImGui::SameLine(ImGui::GetWindowWidth() - 250);
        ImGui::TextDisabled("Version %s (Build %s)", APP_VERSION, APP_BUILD_NUMBER);
        
        ImGui::EndMenuBar();
    }

    // --- B. TOOLBAR ---
    ImGui::AlignTextToFramePadding();
    ImGui::Text("File:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("##filename", m_FilenameBuffer, sizeof(m_FilenameBuffer));
    ImGui::SameLine();
    IGFD::FileDialogConfig config;
    config.path = ".";

    if (ImGui::Button("Save")) { 
        ImGuiFileDialog::Instance()->OpenDialog("SaveSourceDlg", "Save Source File", ".s,.asm,.txt,.*", config);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) { 
        ImGuiFileDialog::Instance()->OpenDialog("ChooseSourceDlg", "Choose Source File", ".s,.asm,.txt,.*", config);
    }
    
    if (m_StatusTimer > 0.0f) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), " [%s]", m_StatusMessage);
        m_StatusTimer -= ImGui::GetIO().DeltaTime;
    }
    ImGui::Separator();

    // --- C. PANES (Layout Math) ---
    float footerHeight = 150.0f;
    float contentHeight = ImGui::GetContentRegionAvail().y - footerHeight - 10.0f;
    float halfWidth = ImGui::GetContentRegionAvail().x * 0.6f;

    // --- Pane 1: Editor ---
    ImGui::BeginChild("EditorPane", ImVec2(halfWidth, contentHeight), true);
    ImGui::TextDisabled("Assembly Source");
    // AllowTabInput lets you use the Tab key to format code
    ImGui::InputTextMultiline("##source", m_EditorBuffer, sizeof(m_EditorBuffer), 
        ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput);
    ImGui::EndChild();

    ImGui::SameLine();

    // --- Pane 2: Hex Dump ---
    ImGui::BeginChild("HexPane", ImVec2(0, contentHeight), true);
    ImGui::TextDisabled("Hex Dump (ROM)");
    ImGui::SameLine();
    ImGui::Text("   Jump to $");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    // Flag to ensure user has only typed valid hex characters
    ImGui::InputText("##jump", m_JumpAddressBuffer, sizeof(m_JumpAddressBuffer), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
    ImGui::SameLine();

    if (ImGui::Button("Go") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))){
        m_DoJump = true;
    }
    ImGui::Separator();

    if (m_LastRom.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No binary generated.");
    } else {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));

        if (m_DoJump) {
            if (m_JumpAddressBuffer[0] != '\0') {
                // Convert the hex string to an integer
                int targetAddr = (int)std::strtol(m_JumpAddressBuffer, nullptr, 16);
                
                // Keep it safely inside the bounds of the ROM size
                if (targetAddr < 0) targetAddr = 0;
                if (targetAddr >= (int)m_LastRom.size()) targetAddr = (int)m_LastRom.size() - 1;

                // Calculate which row this address lives on (16 bytes per row)
                int lineIndex = targetAddr / 16;
                
                // Calculate the exact pixel height to scroll to
                float scrollY = lineIndex * ImGui::GetTextLineHeightWithSpacing();
                ImGui::SetScrollY(scrollY);
            }
            m_DoJump = false; // Reset the trigger
        }
        int totalLines = (m_LastRom.size() + 15) / 16; 
        ImGuiListClipper clipper;
        clipper.Begin(totalLines);
        
        while (clipper.Step()) {
            for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; line++) {
                int addr = line * 16;
                
                // Print the memory address in Yellow
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%04X: ", addr);
                ImGui::SameLine();
                
                // Format up to 16 bytes for this specific row
                for (int j = 0; j < 16; ++j) {
                    if (addr + j < (int)m_LastRom.size()) {
                        ImGui::Text("%02X ", m_LastRom[addr + j]);
                        ImGui::SameLine();
                    }
                }
                ImGui::NewLine();
            }
        }
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    // --- Pane 3: Log & Build Controls ---
    ImGui::BeginChild("LogPane", ImVec2(0, 0), true);
    
    // Output ROM Filename Input
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Output ROM:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("##romfile", m_RomFilenameBuffer, sizeof(m_RomFilenameBuffer));
    ImGui::SameLine();

    if (ImGui::Button("Browse To Save Location")) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("SaveBinDlg", "Choose ROM Save Location", ".bin,.rom,.*", config);
    }
    ImGui::SameLine();
    // The Action Button
    if (ImGui::Button("ASSEMBLE & BUILD", ImVec2(150, 30))) {
        Assembler asmTool;
        // 1. Call Assemble (It returns void, so we remove the 'if' statement)
        asmTool.Assemble(m_EditorBuffer); 
        
        // 2. Fetch the data using the CORRECT method names for your class
        m_LastRom = asmTool.GetBuffer(); // <-- Changed to GetBuffer()
        m_LastLog = asmTool.GetLog();
        
        // 3. Save the binary using the CORRECT method name
        asmTool.SaveBinary(m_RomFilenameBuffer); // <-- Changed to SaveBinary()
        snprintf(m_StatusMessage, sizeof(m_StatusMessage), "Built and saved to %s", m_RomFilenameBuffer);
        m_StatusTimer = 5.0f;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("  Logs:");
    ImGui::Separator();
    
    // Log Output Text
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::TextUnformatted(m_LastLog.c_str());
    ImGui::PopStyleColor();
    
    // Auto-scroll logic
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End(); // END OF MAIN WINDOW

    // ========================================================================
    // 2. SPLASH OVERLAY (Foreground Draw List)
    // ========================================================================
    // By using the Foreground Draw List, we bypass window calculations entirely.
    // The image gets stamped directly onto the screen, ensuring 100% transparency
    // around the PNG and ignoring the z-order of windows below it.
    
    if (m_ShowSplash && m_SplashTexture != 0) {
        float sWidth = (float)m_SplashWidth;
        float sHeight = (float)m_SplashHeight;

        // Calculate Top-Left corner to perfectly center the image
        ImVec2 pos = ImVec2(
            viewport->WorkPos.x + (viewport->WorkSize.x - sWidth) * 0.5f,
            viewport->WorkPos.y + (viewport->WorkSize.y - sHeight) * 0.5f
        );

        // Stamp the texture
        ImGui::GetForegroundDrawList()->AddImage(
            (void*)(intptr_t)m_SplashTexture, 
            pos, 
            ImVec2(pos.x + sWidth, pos.y + sHeight)
        );

        // Timer Logic
        m_SplashTimer -= ImGui::GetIO().DeltaTime;
        
        // Dismiss if time runs out, or if user clicks anywhere
        if (m_SplashTimer <= 0.0f || (ImGui::IsMouseClicked(0) && m_SplashTimer < 2.5f)) {
            m_ShowSplash = false;
        }
    }

    // ========================================================================
    // FILE DIALOG LOGIC
    // ========================================================================

    // 1. Load Source File Dialog
    if (ImGuiFileDialog::Instance()->Display("ChooseSourceDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            LoadSource(filePathName.c_str());
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // 2. Save Source File Dialog
    if (ImGuiFileDialog::Instance()->Display("SaveSourceDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            SaveSource(filePathName.c_str());
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // 3. Output ROM File Dialog
    if (ImGuiFileDialog::Instance()->Display("SaveBinDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            
            // Update the text box buffer with the path you chose
            strncpy(m_RomFilenameBuffer, filePathName.c_str(), sizeof(m_RomFilenameBuffer) - 1);
            m_RomFilenameBuffer[sizeof(m_RomFilenameBuffer) - 1] = '\0'; // Ensure null-termination
        }
        ImGuiFileDialog::Instance()->Close();
    }

}
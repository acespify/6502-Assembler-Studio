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

    LoadEditorSettings();

    // --- 6502 Assembly Syntax Highlighting Setup ---
    TextEditor::LanguageDefinition langDef = TextEditor::LanguageDefinition::CPlusPlus();;
    langDef.mName = "6502 Assembly";
    langDef.mKeywords.clear();            // Wipe C++ rules
    langDef.mIdentifiers.clear();
    langDef.mPreprocIdentifiers.clear();
    langDef.mSingleLineComment = ";"; 
    langDef.mCaseSensitive = false;
    
    const char* opcodes[] = {
        "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRK", "BVC", "BVS",
        "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", "CPY", "DEC", "DEX", "DEY", "EOR", "INC", "INX",
        "INY", "JMP", "JSR", "LDA", "LDX", "LDY", "LSR", "NOP", "ORA", "PHA", "PHP", "PLA", "PLP",
        "ROL", "ROR", "RTI", "RTS", "SBC", "SEC", "SED", "SEI", "STA", "STX", "STY", "TAX", "TAY",
        "TSX", "TXA", "TXS", "TYA",
        // --- W65C02 Extensions ---
        "BRA", "PHX", "PHY", "PLX", "PLY", "STZ", "TRB", "TSB", "WAI", "STP",
        // Lowercase versions
        "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl", "brk", "bvc", "bvs",
        "clc", "cld", "cli", "clv", "cmp", "cpx", "cpy", "dec", "dex", "dey", "eor", "inc", "inx",
        "iny", "jmp", "jsr", "lda", "ldx", "ldy", "lsr", "nop", "ora", "pha", "php", "pla", "plp",
        "rol", "ror", "rti", "rts", "sbc", "sec", "sed", "sei", "sta", "stx", "sty", "tax", "tay",
        "tsx", "txa", "txs", "tya",
        // --- W65C02 Extensions (Lowercase) ---
        "bra", "phx", "phy", "plx", "ply", "stz", "trb", "tsb", "wai", "stp",
        ".ORG", ".WORD", ".BYTE", ".ASCIIZ", ".org", ".word", ".byte", ".asciiz"
    };
    for (const char* op : opcodes) { langDef.mKeywords.insert(op); }

    // INJECT CUSTOM REGEX: If a word ends in a colon, force it to be a Label!
    langDef.mTokenRegexStrings.push_back(std::make_pair(std::string("[a-zA-Z_][a-zA-Z0-9_]*\\s*:"), TextEditor::PaletteIndex::KnownIdentifier));

    m_TextEditor.SetLanguageDefinition(langDef);
    ApplyEditorColors();

    return true;
}

// ============================================================================
// FUNCTION: MainWindow::Shutdown
// PURPOSE: Safely cleans up rendering contexts to prevent memory leaks.
// ============================================================================
void MainWindow::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyPlatformWindows();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

// ============================================================================
// FILE IO AND STATE MANAGEMENT
// ============================================================================

// Grabs text from the editor, writes to file, and updates baseline states
void MainWindow::SaveSource(const char* filepath) {
    std::ofstream out(filepath);
    if (out.is_open()) {
        out << m_TextEditor.GetText(); // Grab text directly form the Pro Editor
        out.close();

        // Baseline text to match the newly saved state
        m_LoadedText = m_TextEditor.GetText();
        snprintf(m_StatusMessage, sizeof(m_StatusMessage), "Saved %s", filepath);
        strncpy(m_FilenameBuffer, filepath, sizeof(m_FilenameBuffer) - 1);

        // Check to see if the user was waiting to close the file after saving, so do it now
        if (m_CloseAfterSave) {
            CloseFile();
            m_CloseAfterSave = false;
        }
    } else {
        snprintf(m_StatusMessage, sizeof(m_StatusMessage), "Error saving %s", filepath);
    }
    m_StatusTimer = 3.0f;
}

// Loads a file into a buffer and pushes it to the text editor
void MainWindow::LoadSource(const char* filepath) {
    std::ifstream in(filepath);
    if (in.is_open()) {
        std::stringstream buffer;
        buffer << in.rdbuf();
        
        m_TextEditor.SetText(buffer.str()); // Feed the loaded file into the Pro Editor

        m_LoadedText = m_TextEditor.GetText();
        snprintf(m_StatusMessage, sizeof(m_StatusMessage), "Loaded %s", filepath);
        strncpy(m_FilenameBuffer, filepath, sizeof(m_FilenameBuffer) - 1);
    } else {
        snprintf(m_StatusMessage, sizeof(m_StatusMessage), "Error loading %s", filepath);
    }
    m_StatusTimer = 3.0f;
}

// Empties the text editor and resets the filename to untitled
void MainWindow::CloseFile() {
    m_TextEditor.SetText("");
    m_LoadedText = ""; // Reset baseline
    strncpy(m_FilenameBuffer, "untitled.s", sizeof(m_FilenameBuffer));
    snprintf(m_StatusMessage, sizeof(m_StatusMessage), "File closed.");
    m_StatusTimer = 3.0f;
}


// ============================================================================
// JSON SETTINGS CONFIGURATION
// ============================================================================

// Translates the UI floats into ImU32 formats and maps them to Editor syntax tokens
void MainWindow::ApplyEditorColors() {
    ImU32 u_text    = ImGui::ColorConvertFloat4ToU32(ImVec4(m_ColText[0], m_ColText[1], m_ColText[2], m_ColText[3]));
    ImU32 u_keyword = ImGui::ColorConvertFloat4ToU32(ImVec4(m_ColKeyword[0], m_ColKeyword[1], m_ColKeyword[2], m_ColKeyword[3]));
    ImU32 u_number  = ImGui::ColorConvertFloat4ToU32(ImVec4(m_ColNumber[0], m_ColNumber[1], m_ColNumber[2], m_ColNumber[3]));
    ImU32 u_comment = ImGui::ColorConvertFloat4ToU32(ImVec4(m_ColComment[0], m_ColComment[1], m_ColComment[2], m_ColComment[3]));
    ImU32 u_bg      = ImGui::ColorConvertFloat4ToU32(ImVec4(m_ColBg[0], m_ColBg[1], m_ColBg[2], m_ColBg[3]));

    // Convert our two new colors
    ImU32 u_variable = ImGui::ColorConvertFloat4ToU32(ImVec4(m_ColVariable[0], m_ColVariable[1], m_ColVariable[2], m_ColVariable[3]));
    ImU32 u_label    = ImGui::ColorConvertFloat4ToU32(ImVec4(m_ColLabel[0], m_ColLabel[1], m_ColLabel[2], m_ColLabel[3]));
    ImU32 u_linenumber = ImGui::ColorConvertFloat4ToU32(ImVec4(m_ColLineNumber[0], m_ColLineNumber[1], m_ColLineNumber[2], m_ColLineNumber[3]));

    // Start with default dark theme to preserve UI cursors
    m_EditorPalette = TextEditor::GetDarkPalette();

    // Map the colors
    m_EditorPalette[(int)TextEditor::PaletteIndex::Default]           = u_text;
    m_EditorPalette[(int)TextEditor::PaletteIndex::Identifier]        = u_text;
    m_EditorPalette[(int)TextEditor::PaletteIndex::Punctuation]       = u_text;

    // THE SPLIT: Variables get 'Identifier', Labels get 'KnownIdentifier'
    m_EditorPalette[(int)TextEditor::PaletteIndex::Identifier]        = u_variable;
    m_EditorPalette[(int)TextEditor::PaletteIndex::KnownIdentifier]   = u_label;
    
    m_EditorPalette[(int)TextEditor::PaletteIndex::Keyword]           = u_keyword;
    m_EditorPalette[(int)TextEditor::PaletteIndex::KnownIdentifier]   = u_keyword;
    
    m_EditorPalette[(int)TextEditor::PaletteIndex::Number]            = u_number;
    m_EditorPalette[(int)TextEditor::PaletteIndex::PreprocIdentifier] = u_number; // Make '#' use number color
    
    m_EditorPalette[(int)TextEditor::PaletteIndex::Comment]           = u_comment;
    m_EditorPalette[(int)TextEditor::PaletteIndex::MultiLineComment]  = u_comment;
    m_EditorPalette[(int)TextEditor::PaletteIndex::Background]        = u_bg;
    m_EditorPalette[(int)TextEditor::PaletteIndex::LineNumber]        = u_linenumber;

    m_TextEditor.SetPalette(m_EditorPalette);
    m_TextEditor.SetShowWhitespaces(m_ShowWhitespaces);
}

// Writes the color float values into a local editor_settings.json file
void MainWindow::SaveEditorSettings() {
    std::ofstream out("editor_settings.json");
    if (!out.is_open()) return;
    
    // Write out a simple JSON structure
    out << "{\n";
    out << "  \"text\": [" << m_ColText[0] << ", " << m_ColText[1] << ", " << m_ColText[2] << "],\n";
    out << "  \"keyword\": [" << m_ColKeyword[0] << ", " << m_ColKeyword[1] << ", " << m_ColKeyword[2] << "],\n";
    out << "  \"number\": [" << m_ColNumber[0] << ", " << m_ColNumber[1] << ", " << m_ColNumber[2] << "],\n";
    out << "  \"comment\": [" << m_ColComment[0] << ", " << m_ColComment[1] << ", " << m_ColComment[2] << "],\n";
    out << "  \"background\": [" << m_ColBg[0] << ", " << m_ColBg[1] << ", " << m_ColBg[2] << "],\n";
    out << "  \"variable\": [" << m_ColVariable[0] << ", " << m_ColVariable[1] << ", " << m_ColVariable[2] << "],\n";
    out << "  \"label\": [" << m_ColLabel[0] << ", " << m_ColLabel[1] << ", " << m_ColLabel[2] << "],\n";
    out << "  \"line_number\": [" << m_ColLineNumber[0] << ", " << m_ColLineNumber[1] << ", " << m_ColLineNumber[2] << "],\n";
    out << "  \"show_whitespace\": " << (m_ShowWhitespaces ? "true" : "false") << "\n";
    out << "}\n";
    out.close();
}

// Reads the JSON file and overwrites the application's default colors
void MainWindow::LoadEditorSettings() {
    std::ifstream in("editor_settings.json");
    if (!in.is_open()) return; // If file doesn't exist, it keeps the defaults

    std::string line;
    while (std::getline(in, line)) {
        // Simple manual parsing to avoid heavy JSON library dependencies
        if (line.find("\"text\"") != std::string::npos) sscanf(line.c_str(), "  \"text\": [%f, %f, %f],", &m_ColText[0], &m_ColText[1], &m_ColText[2]);
        if (line.find("\"keyword\"") != std::string::npos) sscanf(line.c_str(), "  \"keyword\": [%f, %f, %f],", &m_ColKeyword[0], &m_ColKeyword[1], &m_ColKeyword[2]);
        if (line.find("\"number\"") != std::string::npos) sscanf(line.c_str(), "  \"number\": [%f, %f, %f],", &m_ColNumber[0], &m_ColNumber[1], &m_ColNumber[2]);
        if (line.find("\"comment\"") != std::string::npos) sscanf(line.c_str(), "  \"comment\": [%f, %f, %f],", &m_ColComment[0], &m_ColComment[1], &m_ColComment[2]);
        if (line.find("\"background\"") != std::string::npos) sscanf(line.c_str(), "  \"background\": [%f, %f, %f],", &m_ColBg[0], &m_ColBg[1], &m_ColBg[2]);
        if (line.find("\"variable\"") != std::string::npos) sscanf(line.c_str(), "  \"variable\": [%f, %f, %f],", &m_ColVariable[0], &m_ColVariable[1], &m_ColVariable[2]);
        if (line.find("\"label\"") != std::string::npos) sscanf(line.c_str(), "  \"label\": [%f, %f, %f],", &m_ColLabel[0], &m_ColLabel[1], &m_ColLabel[2]);
        if (line.find("\"line_number\"") != std::string::npos) sscanf(line.c_str(), "  \"line_number\": [%f, %f, %f],", &m_ColLineNumber[0], &m_ColLineNumber[1], &m_ColLineNumber[2]);
        if (line.find("\"show_whitespace\"") != std::string::npos) m_ShowWhitespaces = (line.find("true") != std::string::npos);
    }
    in.close();
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
//   The heart of the frontend IDE. This function dictates the layout, state, 
//   and interactions of the entire application interface every single frame 
//   using ImGui's immediate-mode paradigm.
//
// HOW IT WORKS:
//   - Global Sizing: Determines the target hardware memory bounds (64KB, 
//     32KB, or 16KB) to dynamically scale the rest of the UI panes.
//   - Top Menu & Toolbar: Draws the File menu, Quick-Save/Load buttons, 
//     status messages, and the current App Version / Build Number.
//   - Editor Pane: Renders the multi-line text input with tab support for 
//     writing assembly source code.
//   - Hex Dump Pane: A dynamically scaled memory monitor using ImGuiListClipper.
//     Features bounded "Jump To" navigation, absolute 6502 address translation,
//     dimmed NOP/Empty bytes for readability, and an ASCII character column.
//   - Build & Log Pane: Contains the target size selector, ROM export path, 
//     and the "Assemble & Build" execution button. Displays auto-scrolling 
//     compiler outputs and errors.
//   - Splash Screen: Renders a temporary startup image directly to the 
//     Foreground Draw List, bypassing standard window z-ordering.
//   - File Dialogs: Handles the asynchronous state of ImGuiFileDialog for 
//     safely loading/saving source scripts and compiled .bin files.
// ============================================================================
void MainWindow::RenderUI() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("Assembler Studio", nullptr, 
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus
    );

    // 1. Top Level UI
    DrawMenuBar();
    DrawToolbar();

    // 2. Layout Math
    float footerHeight = 150.0f;
    float contentHeight = ImGui::GetContentRegionAvail().y - footerHeight - 10.0f;
    float halfWidth = ImGui::GetContentRegionAvail().x * 0.6f;

    // 3. The Panes
    DrawEditorPane(halfWidth, contentHeight);
    ImGui::SameLine();
    DrawHexDumpPane(contentHeight);
    DrawLogAndControlPane();

    ImGui::End(); // End Main Window

    // 4. Overlays & Dialogs
    if (m_ShowColorSettings) DrawColorSettingsWindow();
    DrawSplashOverlay();
    DrawDialogs();

}

// ============================================================================
// UI SUB-COMPONENTS
// ============================================================================

// Top application menu for File commands and View toggles
void MainWindow::DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            IGFD::FileDialogConfig config; config.path = ".";
            if (ImGui::MenuItem("Save", "Ctrl+S")) { ImGuiFileDialog::Instance()->OpenDialog("SaveSourceDlg", "Save Source File", ".s,.asm,.txt,.*", config); }
            if (ImGui::MenuItem("Load", "Ctrl+O")) { ImGuiFileDialog::Instance()->OpenDialog("ChooseSourceDlg", "Choose Source File", ".s,.asm,.txt,.*", config); }
            if (ImGui::MenuItem("Close")){ if (m_TextEditor.GetText() != m_LoadedText){ m_WantsToClose = true;}else{ CloseFile(); } }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) { glfwSetWindowShouldClose(m_Window, true); }
            ImGui::EndMenu();
        }
        
        // --- NEW: View Menu for Settings ---
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Editor Colors", nullptr, &m_ShowColorSettings);
            ImGui::EndMenu();
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 250);
        ImGui::TextDisabled("Version %s (Build %s)", APP_VERSION, APP_BUILD_NUMBER);
        ImGui::EndMenuBar();
    }
}

// Quick action toolbar housing the current file path and status messages
void MainWindow::DrawToolbar() {
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
}

// The Text Editor wrapper container
void MainWindow::DrawEditorPane(float width, float height){
    ImGui::BeginChild("EditorPane", ImVec2(width, height), true);
    ImGui::TextDisabled("Assembly Source");
    m_TextEditor.Render("CodeEditor");
    ImGui::EndChild();
}

// Renders the virtual 6502 memory mapping using an efficient ListClipper
void MainWindow::DrawHexDumpPane(float height) {
    ImGui::BeginChild("HexPane", ImVec2(0, height), true);
    ImGui::TextDisabled("Hex Dump (ROM)");
    ImGui::SameLine();
    ImGui::Text("   Jump to $");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);

    // Recalculate layout sizes based on the class member m_RomSizeIdx
    uint16_t start_address = 0x0000;
    int display_size = 65536;
    if (m_RomSizeIdx == 1)      { start_address = 0x8000; display_size = 32768; }
    else if (m_RomSizeIdx == 2) { start_address = 0xC000; display_size = 16384; }

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
                int targetAddr = (int)std::strtol(m_JumpAddressBuffer, nullptr, 16);
                if (targetAddr < start_address) targetAddr = start_address;
                if (targetAddr >= start_address + display_size) targetAddr = start_address + display_size - 1;

                int lineIndex = (targetAddr - start_address) / 16;
                float scrollY = lineIndex * ImGui::GetTextLineHeightWithSpacing();
                ImGui::SetScrollY(scrollY);
            }
            m_DoJump = false; 
        }

        int totalLines = display_size / 16; 
        ImGuiListClipper clipper;
        clipper.Begin(totalLines);
        
        while (clipper.Step()) {
            for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; line++) {
                int base_addr = start_address + (line * 16);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%04X: ", base_addr);
                ImGui::SameLine();
                
                for (int j = 0; j < 16; ++j) {
                    int mem_addr = base_addr + j;
                    if (mem_addr < (int)m_LastRom.size()) {
                        uint8_t val = m_LastRom[mem_addr];
                        if (val == 0xEA) {
                            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "EA ");
                        } else if (val == 0x00) {
                            ImGui::TextDisabled("00 ");
                        } else {
                            ImGui::Text("%02X ", val);
                        }
                        ImGui::SameLine();
                    }
                }

                ImGui::TextDisabled(" | ");
                ImGui::SameLine(); 
                for (int j = 0; j < 16; ++j) {
                    int mem_addr = base_addr + j;
                    if (mem_addr < (int)m_LastRom.size()){
                        uint8_t val = m_LastRom[mem_addr];
                        if (val >= 32 && val <= 126) ImGui::Text("%c", val);
                        else ImGui::TextDisabled(".");
                        ImGui::SameLine(0, 0); 
                    }
                }
                ImGui::NewLine();
            }
        }
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
}

// Target selector, Assembly execution button, and Auto-scrolling compiler logs
void MainWindow::DrawLogAndControlPane() {
    ImGui::BeginChild("LogPane", ImVec2(0, 0), true);
    
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

    // Re-declare the labels array here
    const char* rom_sizes[] = {"Full 64KB ($0000-$FFFF)", "32KB EEPROM ($8000-$FFFF)", "16KB ROM ($C000-$FFFF)"};
    
    // Notice we use m_RomSizeIdx (capital 'R') to match the header
    ImGui::Combo("Output Target", &m_RomSizeIdx, rom_sizes, IM_ARRAYSIZE(rom_sizes));

    if (ImGui::Button("ASSEMBLE & BUILD", ImVec2(150, 30))) {
        Assembler asmTool;
        std::string sourceCode = m_TextEditor.GetText();
        asmTool.Assemble(sourceCode); 
        
        m_LastRom = asmTool.GetBuffer(); 
        m_LastLog = asmTool.GetLog();
        
        switch (m_RomSizeIdx){
            case 0: asmTool.SaveBinary(m_RomFilenameBuffer, 0x0000, 65536); break;
            case 1: asmTool.SaveBinary(m_RomFilenameBuffer, 0x8000, 32768); break;
            case 2: asmTool.SaveBinary(m_RomFilenameBuffer, 0xC000, 16384); break;
        }
        
        snprintf(m_StatusMessage, sizeof(m_StatusMessage), "Built and saved to %s", m_RomFilenameBuffer);
        m_StatusTimer = 5.0f;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("  Logs:");
    ImGui::Separator();
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::TextUnformatted(m_LastLog.c_str());
    ImGui::PopStyleColor();
    
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
}

// Manages all asynchronous modal popups (File Browsers and "Unsaved Changes" warnings)
void MainWindow::DrawDialogs() {
    // 1. Open the popup if the trigger was set in the menu
    if (m_WantsToClose) {
        ImGui::OpenPopup("Unsaved Changes");
        m_WantsToClose = false; // Reset trigger
    }

    // Center the modal perfectly on the screen
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // 2. Render the Modal
    if (ImGui::BeginPopupModal("Unsaved Changes", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("You have unsaved changes. Do you want to save before closing?");
        ImGui::Separator();

        if (ImGui::Button("Save", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            
            // If the file is untitled, open the Save As dialog
            if (strcmp(m_FilenameBuffer, "untitled.s") == 0 || strcmp(m_FilenameBuffer, "main.s") == 0) {
                m_CloseAfterSave = true; // Tell the system to close it AFTER the dialog finishes
                IGFD::FileDialogConfig config; config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("SaveSourceDlg", "Save Source File", ".s,.asm,.txt,.*", config);
            } else {
                // If it already has a real filename, just quick-save and close instantly
                SaveSource(m_FilenameBuffer);
                CloseFile();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Don't Save", ImVec2(120, 0))) {
            CloseFile(); // Nuke the buffer without saving
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup(); // Abort the close operation entirely
        }
        
        ImGui::EndPopup();
    }

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

// ========================================================================
// 2. SPLASH OVERLAY (Foreground Draw List)
// ========================================================================
// By using the Foreground Draw List, we bypass window calculations entirely.
// The image gets stamped directly onto the screen, ensuring 100% transparency
// around the PNG and ignoring the z-order of windows below it.
void MainWindow::DrawSplashOverlay() {
    if (m_ShowSplash && m_SplashTexture != 0) {
        
        // ADD THIS: Fetch the viewport here!
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        
        float sWidth = (float)m_SplashWidth;
        float sHeight = (float)m_SplashHeight;

        ImVec2 pos = ImVec2(
            viewport->WorkPos.x + (viewport->WorkSize.x - sWidth) * 0.5f,
            viewport->WorkPos.y + (viewport->WorkSize.y - sHeight) * 0.5f
        );

        ImGui::GetForegroundDrawList()->AddImage(
            (void*)(intptr_t)m_SplashTexture, pos, ImVec2(pos.x + sWidth, pos.y + sHeight)
        );

        m_SplashTimer -= ImGui::GetIO().DeltaTime;
        if (m_SplashTimer <= 0.0f || (ImGui::IsMouseClicked(0) && m_SplashTimer < 2.5f)) {
            m_ShowSplash = false;
        }
    }
}

// Floating tool window that binds user interactions directly to the Editor's token palette
void MainWindow::DrawColorSettingsWindow() {
    ImGui::Begin("Editor Color Settings", &m_ShowColorSettings, ImGuiWindowFlags_AlwaysAutoResize);

    bool changed = false;
    if (ImGui::ColorEdit3("Standard Text", m_ColText)) changed = true;
    if (ImGui::ColorEdit3("Variables", m_ColVariable)) changed = true;
    if (ImGui::ColorEdit3("Labels", m_ColLabel)) changed = true;
    if (ImGui::ColorEdit3("Opcodes", m_ColKeyword)) changed = true;
    if (ImGui::ColorEdit3("Numbers", m_ColNumber)) changed = true;
    if (ImGui::ColorEdit3("Comments", m_ColComment)) changed = true;
    if (ImGui::ColorEdit3("Line Numbers", m_ColLineNumber)) changed = true;
    if (ImGui::ColorEdit3("Background", m_ColBg)) changed = true;

    ImGui::Separator();
    if (ImGui::Checkbox("Show Whitespace", &m_ShowWhitespaces)) changed = true;

    if (changed) ApplyEditorColors();

    ImGui::Separator();

    // Write out the JSON file when clicked
    if (ImGui::Button("Save Settings", ImVec2(120, 0))) {
        SaveEditorSettings();
        m_ShowColorSettings = false; // Close window on save
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        // Revert to saved file if they cancel
        LoadEditorSettings();
        ApplyEditorColors();
        m_ShowColorSettings = false;
    }

    ImGui::End();
}
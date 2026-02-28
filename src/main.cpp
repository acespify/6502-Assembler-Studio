// ============================================================================
// Copyright (c) 2026 Andrew Young
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
// ============================================================================
#include "ui/mainwindow.h"
#include <iostream>

int main() {
    MainWindow app(1920, 1080, "6502 Pro Assembler");

    // Initialize the window with a resolution and title
    if (!app.Init()) {
        std::cerr << "Failed to initialize GUI." << std::endl;
        return -1;
    }

    // Run the main application loop
    app.Run();

    return 0;
}

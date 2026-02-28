// ============================================================================
// Copyright (c) 2026 Andrew Young
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
// 
// ============================================================================
// FILE: assembler.h
// PURPOSE: 
//   Declares the core Assembler class. The Assembler holds the 64KB virtual 
//   RAM buffer, the symbol table (for storing variable/label addresses), and 
//   the build log. It defines the core Two-Pass compilation 
//   process required to safely resolve forward references and build the ROM.
// ============================================================================

#pragma once
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include "lexer.h"

class Assembler {
public:
    // Initialize the RAM buffer to represent the 64KB address space
    Assembler() { 
        ram.resize(65536, 0); 
    }

    // Main entry point for the UI to compile the source text
    void Assemble(const std::string& source);
    
    // File output
    void SaveBinary(const std::string& filepath, uint16_t start_address = 0x0000, size_t size = 65536);
    
    // Getters for the ImGui frontend
    const std::vector<uint8_t>& GetBuffer() const { return ram; }
    const std::string& GetLog() const { return log; }

private:
    std::vector<uint8_t> ram;
    std::map<std::string, uint16_t> symbolTable;
    std::string log;
    
    Lexer lexer;

    // The two phases of assembly
    void PassOne(const std::vector<Token>& tokens);
    void PassTwo(const std::vector<Token>& tokens);
};
// ============================================================================
// Copyright (c) 2026 Andrew Young
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
// 
// ============================================================================
// FILE: opcodetable.h
// PURPOSE: 
//   Defines the core data structures needed to represent 6502/65C02 instructions. 
//   It includes the AddrMode enum, which lists all possible ways the CPU can 
//   fetch data (like Immediate, ZeroPage, Absolute). It 
//   also defines the Instruction struct (opcode, addressing mode, and byte size) 
//   and declares the global OpcodeMap.
// ============================================================================

#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

// Define all possible 6502/65C02 addressing modes
enum class AddrMode {
    Implied,    // CLC
    Accumulator,// ROL A
    Immediate,  // LDA #$10
    ZeroPage,   // LDA $10
    ZeroPageX,  // LDA $10,X
    ZeroPageY,  // LDX $10,Y
    Relative,   // BNE label (used for branching)
    Absolute,   // LDA $1234
    AbsoluteX,  // LDA $1234,X
    AbsoluteY,  // LDA $1234,Y
    Indirect,   // JMP ($1234)
    IndexedInd, // LDA ($10,X)
    IndIndexed, // LDA ($10),Y
    IndZP,      // LDA ($10) - 65C02 specific
};

struct Instruction {
    uint8_t opcode;
    AddrMode mode;
    int size;       // Total bytes (1, 2, or 3)
};

// The Master Map: Mnemonic -> List of possible versions of that instruction
extern std::map<std::string, std::vector<Instruction>> OpcodeMap;
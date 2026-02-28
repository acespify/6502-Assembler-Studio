// ============================================================================
// Copyright (c) 2026 Andrew Young
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
// 
// ============================================================================
// FILE: opcodetable.cpp
// PURPOSE: 
//   Acts as the master dictionary for the assembler. It maps human-readable 
//   mnemonics (like "LDA" or "JMP") to their corresponding machine code instructions. 
//   Because an instruction like "LDA" has multiple variations depending on 
//   how it addresses memory, this map groups all valid variations together.
// ============================================================================

#include "opcodetable.h"

std::map<std::string, std::vector<Instruction>> OpcodeMap = {
    // --- LOAD/STORE ---
    {"LDA", {{0xA9, AddrMode::Immediate, 2}, {0xA5, AddrMode::ZeroPage, 2}, {0xB5, AddrMode::ZeroPageX, 2}, {0xAD, AddrMode::Absolute, 3}, {0xBD, AddrMode::AbsoluteX, 3}, {0xB9, AddrMode::AbsoluteY, 3}, {0xA1, AddrMode::IndexedInd, 2}, {0xB1, AddrMode::IndIndexed, 2}, {0xB2, AddrMode::IndZP, 2}}},
    {"LDX", {{0xA2, AddrMode::Immediate, 2}, {0xA6, AddrMode::ZeroPage, 2}, {0xB6, AddrMode::ZeroPageY, 2}, {0xAE, AddrMode::Absolute, 3}, {0xBE, AddrMode::AbsoluteY, 3}}},
    {"LDY", {{0xA0, AddrMode::Immediate, 2}, {0xA4, AddrMode::ZeroPage, 2}, {0xB4, AddrMode::ZeroPageX, 2}, {0xAC, AddrMode::Absolute, 3}, {0xBC, AddrMode::AbsoluteX, 3}}},
    {"STA", {{0x85, AddrMode::ZeroPage, 2}, {0x95, AddrMode::ZeroPageX, 2}, {0x8D, AddrMode::Absolute, 3}, {0x9D, AddrMode::AbsoluteX, 3}, {0x99, AddrMode::AbsoluteY, 3}, {0x81, AddrMode::IndexedInd, 2}, {0x91, AddrMode::IndIndexed, 2}, {0x92, AddrMode::IndZP, 2}}},
    {"STX", {{0x86, AddrMode::ZeroPage, 2}, {0x96, AddrMode::ZeroPageY, 2}, {0x8E, AddrMode::Absolute, 3}}},
    {"STY", {{0x84, AddrMode::ZeroPage, 2}, {0x94, AddrMode::ZeroPageX, 2}, {0x8C, AddrMode::Absolute, 3}}},
    {"STZ", {{0x64, AddrMode::ZeroPage, 2}, {0x74, AddrMode::ZeroPageX, 2}, {0x9C, AddrMode::Absolute, 3}, {0x9E, AddrMode::AbsoluteX, 3}}}, // 65C02

    // --- ARITHMETIC ---
    {"ADC", {{0x69, AddrMode::Immediate, 2}, {0x65, AddrMode::ZeroPage, 2}, {0x75, AddrMode::ZeroPageX, 2}, {0x6D, AddrMode::Absolute, 3}, {0x7D, AddrMode::AbsoluteX, 3}, {0x79, AddrMode::AbsoluteY, 3}, {0x61, AddrMode::IndexedInd, 2}, {0x71, AddrMode::IndIndexed, 2}, {0x72, AddrMode::IndZP, 2}}},
    {"SBC", {{0xE9, AddrMode::Immediate, 2}, {0xE5, AddrMode::ZeroPage, 2}, {0xF5, AddrMode::ZeroPageX, 2}, {0xED, AddrMode::Absolute, 3}, {0xFD, AddrMode::AbsoluteX, 3}, {0xF9, AddrMode::AbsoluteY, 3}, {0xE1, AddrMode::IndexedInd, 2}, {0xF1, AddrMode::IndIndexed, 2}, {0xF2, AddrMode::IndZP, 2}}},

    // --- LOGICAL ---
    {"AND", {{0x29, AddrMode::Immediate, 2}, {0x25, AddrMode::ZeroPage, 2}, {0x35, AddrMode::ZeroPageX, 2}, {0x2D, AddrMode::Absolute, 3}, {0x3D, AddrMode::AbsoluteX, 3}, {0x39, AddrMode::AbsoluteY, 3}, {0x21, AddrMode::IndexedInd, 2}, {0x31, AddrMode::IndIndexed, 2}, {0x32, AddrMode::IndZP, 2}}},
    {"ORA", {{0x09, AddrMode::Immediate, 2}, {0x05, AddrMode::ZeroPage, 2}, {0x15, AddrMode::ZeroPageX, 2}, {0x0D, AddrMode::Absolute, 3}, {0x1D, AddrMode::AbsoluteX, 3}, {0x19, AddrMode::AbsoluteY, 3}, {0x01, AddrMode::IndexedInd, 2}, {0x11, AddrMode::IndIndexed, 2}, {0x12, AddrMode::IndZP, 2}}},
    {"EOR", {{0x49, AddrMode::Immediate, 2}, {0x45, AddrMode::ZeroPage, 2}, {0x55, AddrMode::ZeroPageX, 2}, {0x4D, AddrMode::Absolute, 3}, {0x5D, AddrMode::AbsoluteX, 3}, {0x59, AddrMode::AbsoluteY, 3}, {0x41, AddrMode::IndexedInd, 2}, {0x51, AddrMode::IndIndexed, 2}, {0x52, AddrMode::IndZP, 2}}},
    {"BIT", {{0x24, AddrMode::ZeroPage, 2}, {0x2C, AddrMode::Absolute, 3}, {0x34, AddrMode::ZeroPageX, 2}, {0x3C, AddrMode::AbsoluteX, 3}, {0x89, AddrMode::Immediate, 2}}},

    // --- BRANCHING ---
    {"BCC", {{0x90, AddrMode::Relative, 2}}},
    {"BCS", {{0xB0, AddrMode::Relative, 2}}},
    {"BEQ", {{0xF0, AddrMode::Relative, 2}}},
    {"BNE", {{0xD0, AddrMode::Relative, 2}}},
    {"BMI", {{0x30, AddrMode::Relative, 2}}},
    {"BPL", {{0x10, AddrMode::Relative, 2}}},
    {"BVC", {{0x50, AddrMode::Relative, 2}}},
    {"BVS", {{0x70, AddrMode::Relative, 2}}},
    {"BRA", {{0x80, AddrMode::Relative, 2}}}, // 65C02

    // --- JUMP / CALL ---
    {"JMP", {{0x4C, AddrMode::Absolute, 3}, {0x6C, AddrMode::Indirect, 3}, {0x7C, AddrMode::AbsoluteX, 3}}}, // 65C02 adds AbsX to JMP
    {"JSR", {{0x20, AddrMode::Absolute, 3}}},
    {"RTS", {{0x60, AddrMode::Implied, 1}}},
    {"RTI", {{0x40, AddrMode::Implied, 1}}},

    // --- INCREMENT / DECREMENT ---
    {"INC", {{0xE6, AddrMode::ZeroPage, 2}, {0xF6, AddrMode::ZeroPageX, 2}, {0xEE, AddrMode::Absolute, 3}, {0xFE, AddrMode::AbsoluteX, 3}, {0x1A, AddrMode::Accumulator, 1}}},
    {"DEC", {{0xC6, AddrMode::ZeroPage, 2}, {0xD6, AddrMode::ZeroPageX, 2}, {0xCE, AddrMode::Absolute, 3}, {0xDE, AddrMode::AbsoluteX, 3}, {0x3A, AddrMode::Accumulator, 1}}},
    {"INX", {{0xE8, AddrMode::Implied, 1}}},
    {"INY", {{0xC8, AddrMode::Implied, 1}}},
    {"DEX", {{0xCA, AddrMode::Implied, 1}}},
    {"DEY", {{0x88, AddrMode::Implied, 1}}},

    // --- REGISTER TRANSFERS / STACK ---
    {"TAX", {{0xAA, AddrMode::Implied, 1}}}, {"TAY", {{0xA8, AddrMode::Implied, 1}}},
    {"TXA", {{0x8A, AddrMode::Implied, 1}}}, {"TYA", {{0x98, AddrMode::Implied, 1}}},
    {"TSX", {{0xBA, AddrMode::Implied, 1}}}, {"TXS", {{0x9A, AddrMode::Implied, 1}}},
    {"PHA", {{0x48, AddrMode::Implied, 1}}}, {"PHP", {{0x08, AddrMode::Implied, 1}}},
    {"PLA", {{0x68, AddrMode::Implied, 1}}}, {"PLP", {{0x28, AddrMode::Implied, 1}}},
    {"PHX", {{0xDA, AddrMode::Implied, 1}}}, {"PHY", {{0x5A, AddrMode::Implied, 1}}}, // 65C02
    {"PLX", {{0xFA, AddrMode::Implied, 1}}}, {"PLY", {{0x7A, AddrMode::Implied, 1}}}, // 65C02

    // --- SHIFT / ROTATE ---
    {"ASL", {{0x0A, AddrMode::Accumulator, 1}, {0x06, AddrMode::ZeroPage, 2}, {0x16, AddrMode::ZeroPageX, 2}, {0x0E, AddrMode::Absolute, 3}, {0x1E, AddrMode::AbsoluteX, 3}}},
    {"LSR", {{0x4A, AddrMode::Accumulator, 1}, {0x46, AddrMode::ZeroPage, 2}, {0x56, AddrMode::ZeroPageX, 2}, {0x4E, AddrMode::Absolute, 3}, {0x5E, AddrMode::AbsoluteX, 3}}},
    {"ROL", {{0x2A, AddrMode::Accumulator, 1}, {0x26, AddrMode::ZeroPage, 2}, {0x36, AddrMode::ZeroPageX, 2}, {0x2E, AddrMode::Absolute, 3}, {0x3E, AddrMode::AbsoluteX, 3}}},
    {"ROR", {{0x6A, AddrMode::Accumulator, 1}, {0x66, AddrMode::ZeroPage, 2}, {0x76, AddrMode::ZeroPageX, 2}, {0x6E, AddrMode::Absolute, 3}, {0x7E, AddrMode::AbsoluteX, 3}}},

    // --- FLAG SET/CLEAR ---
    {"CLC", {{0x18, AddrMode::Implied, 1}}}, {"SEC", {{0x38, AddrMode::Implied, 1}}},
    {"CLI", {{0x58, AddrMode::Implied, 1}}}, {"SEI", {{0x78, AddrMode::Implied, 1}}},
    {"CLV", {{0xB8, AddrMode::Implied, 1}}}, {"CLD", {{0xD8, AddrMode::Implied, 1}}},
    {"SED", {{0xF8, AddrMode::Implied, 1}}},

    // --- MISC ---
    {"BRK", {{0x00, AddrMode::Implied, 1}}},
    {"NOP", {{0xEA, AddrMode::Implied, 1}}},
    {"TRB", {{0x14, AddrMode::ZeroPage, 2}, {0x1C, AddrMode::Absolute, 3}}}, // 65C02
    {"TSB", {{0x04, AddrMode::ZeroPage, 2}, {0x0C, AddrMode::Absolute, 3}}},  // 65C02

    // --- COMPARES ---
    {"CMP", {{0xC9, AddrMode::Immediate, 2}, {0xC5, AddrMode::ZeroPage, 2}, {0xD5, AddrMode::ZeroPageX, 2}, {0xCD, AddrMode::Absolute, 3}, {0xDD, AddrMode::AbsoluteX, 3}, {0xD9, AddrMode::AbsoluteY, 3}, {0xC1, AddrMode::IndexedInd, 2}, {0xD1, AddrMode::IndIndexed, 2}, {0xD2, AddrMode::IndZP, 2}}},
    {"CPX", {{0xE0, AddrMode::Immediate, 2}, {0xE4, AddrMode::ZeroPage, 2}, {0xEC, AddrMode::Absolute, 3}}},
    {"CPY", {{0xC0, AddrMode::Immediate, 2}, {0xC4, AddrMode::ZeroPage, 2}, {0xCC, AddrMode::Absolute, 3}}},

    // --- 65C02 HARDWARE CONTROL ---
    {"WAI", {{0xCB, AddrMode::Implied, 1}}}, // Wait for Interrupt
    {"STP", {{0xDB, AddrMode::Implied, 1}}} // Stop Processor
};
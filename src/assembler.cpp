// ============================================================================
// Copyright (c) 2026 Andrew Young
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
// 
// ============================================================================
// FILE: assembler.cpp
// PURPOSE: 
//   The brain of the compiler. It processes tokens through a Two-Pass system:
//   - Pass 1: Scans tokens to track the Program Counter (PC) and records 
//             the exact addresses of all labels and variables.
//   - Pass 2: Uses those recorded label addresses to generate the final 
//             machine code bytes and inserts them into the RAM buffer.
//   It also handles parsing complex mathematical expressions (+, |) within operands.
// ============================================================================

#include "Assembler.h"
#include "OpcodeTable.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>


// ============================================================================
// FUNCTION: EvaluateOperand
// PURPOSE: 
//   This function takes a raw operand string (like "#$FF", "label+1", or "10") 
//   and converts it into a usable 16-bit integer.
// 
// HOW IT WORKS:
//   1. Cleanup: It strips away structural characters like '#', '(', and ')' 
//      so it can focus purely on the math or the label name.
//   2. Recursion: It searches for '+' or '|' operators. If it finds them, it 
//      splits the string and calls itself recursively to calculate both sides.
//   3. Conversion: It checks the first character to determine the number base:
//      '$' for Hexadecimal, '%' for Binary, or standard digits for Decimal.
//   4. Symbols: If it's a word instead of a number, it looks up the value in 
//      the symbolTable. If it doesn't exist yet, it returns 0xFFFF (assuming 
//      it is a forward reference that Pass 2 will resolve later).
// ============================================================================
uint16_t EvaluateOperand(std::string op, const std::map<std::string, uint16_t>& symbolTable) {
    if (op.empty()) return 0;
    
    // Clean up addressing mode symbols before doing math
    if (op.front() == '#') op.erase(0, 1); 
    if (op.front() == '(') op.erase(0, 1);
    if (op.back() == ')') op.pop_back();
    
    // ---> NEW FIX: Recursively evaluate Bitwise OR (|)
    size_t orPos = op.find('|');
    if (orPos != std::string::npos) {
        std::string left = op.substr(0, orPos);
        std::string right = op.substr(orPos + 1);
        return EvaluateOperand(left, symbolTable) | EvaluateOperand(right, symbolTable);
    }

    // ---> NEW FIX: Recursively evaluate Addition (+)
    size_t addPos = op.find('+');
    if (addPos != std::string::npos) {
        std::string left = op.substr(0, addPos);
        std::string right = op.substr(addPos + 1);
        return EvaluateOperand(left, symbolTable) + EvaluateOperand(right, symbolTable);
    }
    
    if (op[0] == '$') { 
        uint16_t val;
        std::stringstream ss;
        ss << std::hex << op.substr(1);
        ss >> val;
        return val;
    }
    if (op[0] == '%') {
        try { return std::stoi(op.substr(1), nullptr, 2); } catch(...) { return 0; }
    }
    if (isdigit(op[0])) {
        try { return std::stoi(op); } catch(...) { return 0; }
    }
    if (symbolTable.find(op) != symbolTable.end()) return symbolTable.at(op);
    
    return 0xFFFF; // Forward Reference
}


// ============================================================================
// FUNCTION: DetermineAddressingMode
// PURPOSE: 
//   This function plays detective. It looks at the syntax of an instruction 
//   and figures out exactly which 6502 addressing mode the programmer 
//   intended to use. 
//
// HOW IT WORKS:
//   - Implied/Relative: Checks if there is no operand or if the mnemonic 
//     starts with 'B' (Branch instructions).
//   - Immediate: Looks for the '#' prefix.
//   - Indirect: Looks for the '(' prefix and checks if X or Y registers 
//     are used after a comma to determine Indexed Indirect or Indirect Indexed.
//   - Zero Page vs Absolute: Uses the calculated 16-bit 'val'. If the value 
//     is less than 256 (0x0100), it fits in the Zero Page to save memory and 
//     cycles. Otherwise, it defaults to Absolute addressing.
//   - It also checks for the ",X" or ",Y" syntax to append the correct index.
// ============================================================================
AddrMode DetermineAddressingMode(const std::string& mnemonic, std::string op, bool hasComma, std::string reg, uint16_t val) {
    if (op.empty() || op == "A") return AddrMode::Implied; 
    if (mnemonic[0] == 'B' && mnemonic != "BIT" && mnemonic != "BRK") return AddrMode::Relative;
    bool isIndirect = (op.front() == '(');
    if (op.front() == '#') return AddrMode::Immediate;

    if (isIndirect) {
        if (hasComma && reg == "X") return AddrMode::IndexedInd; 
        if (hasComma && reg == "Y") return AddrMode::IndIndexed; 
        if (val < 256) return AddrMode::IndZP; 
        return AddrMode::Indirect; 
    }

    if (val < 256) {
        if (hasComma && reg == "X") return AddrMode::ZeroPageX;
        if (hasComma && reg == "Y") return AddrMode::ZeroPageY;
        return AddrMode::ZeroPage;
    } else {
        if (hasComma && reg == "X") return AddrMode::AbsoluteX;
        if (hasComma && reg == "Y") return AddrMode::AbsoluteY;
        return AddrMode::Absolute;
    }
}


// ============================================================================
// FUNCTION: Assembler::Assemble
// PURPOSE: 
//   This is the main entry point for the compiler. When you click "Build" in 
//   the UI, this function orchestrates the entire assembly process.
//
// HOW IT WORKS:
//   1. Initialization: It clears the build log and fills the 64KB RAM buffer 
//      with zeros to ensure a clean slate.
//   2. Tokenization: It passes the raw text to the Lexer, which chops the 
//      code into manageable Token objects.
//   3. Compilation: It runs PassOne to map out all the memory addresses, and 
//      then PassTwo to actually generate the machine code bytes.
// ============================================================================
void Assembler::Assemble(const std::string& source) {
    log = "Starting Assembly...\n";
    std::fill(ram.begin(), ram.end(), 0xEA); 
    std::vector<Token> tokens = lexer.Tokenize(source);
    PassOne(tokens);
    PassTwo(tokens);
}


// ============================================================================
// FUNCTION: Assembler::PassOne
// PURPOSE: 
//   The goal of the first pass is strictly to calculate memory addresses and 
//   build the Symbol Table. It does NOT write any machine code to RAM.
//
// HOW IT WORKS:
//   - It tracks a virtual Program Counter (PC) starting at 0x0000.
//   - Directives: If it sees `.ORG`, it changes the PC to the new address. If 
//     it sees `.BYTE` or `.WORD`, it advances the PC by 1 or 2 bytes respectively.
//   - Labels: When it encounters a label (like "Loop:"), it saves the label's 
//     name and the current PC into the symbolTable so Pass Two knows exactly 
//     where "Loop" is located in memory.
//   - Instructions: It calculates the size of the instruction (1, 2, or 3 bytes) 
//     and advances the PC accordingly.
// ============================================================================
void Assembler::PassOne(const std::vector<Token>& tokens) {
    uint16_t PC = 0x0000;
    symbolTable.clear();

    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& t = tokens[i];

        if (t.type == TokenType::Directive) {
            if (t.value == ".ORG" && i + 1 < tokens.size()) PC = EvaluateOperand(tokens[++i].value, symbolTable);
            else if (t.value == ".BYTE" || t.value == ".DB") {
                int currentLine = t.line;
                while (i + 1 < tokens.size() && tokens[i+1].line == currentLine) {
                    if (tokens[++i].type != TokenType::Comma) PC += 1;
                }
            }
            else if (t.value == ".WORD" || t.value == ".DW") {
                int currentLine = t.line;
                while (i + 1 < tokens.size() && tokens[i+1].line == currentLine) {
                    if (tokens[++i].type != TokenType::Comma) PC += 2;
                }
            }
        }
        else if (t.type == TokenType::Label || 
                (t.type == TokenType::Instruction && i + 1 < tokens.size() && (tokens[i+1].value == "EQU" || tokens[i+1].value == "="))) {
            std::string labelName = t.value;
            if (!labelName.empty() && labelName.back() == ':') labelName.pop_back();

            if (i + 1 < tokens.size() && (tokens[i+1].value == "EQU" || tokens[i+1].value == "=")) {
                symbolTable[labelName] = EvaluateOperand(tokens[i+2].value, symbolTable);
                i += 2; 
            } else {
                symbolTable[labelName] = PC;
            }
        } 
        else if (t.type == TokenType::Instruction) {
            std::string mnemonic = t.value;
            std::string op = "";
            bool hasComma = false;
            std::string reg = "";

            if (i + 1 < tokens.size() && tokens[i+1].line == t.line && 
               (tokens[i+1].type == TokenType::Operand || tokens[i+1].type == TokenType::Label || tokens[i+1].type == TokenType::Instruction)) {
                
                // ---> NEW FIX: Keep eating tokens on this line until we hit a comma to build math expressions
                while (i + 1 < tokens.size() && tokens[i+1].line == t.line && tokens[i+1].type != TokenType::Comma) {
                    op += tokens[++i].value;
                }

                if (i + 1 < tokens.size() && tokens[i+1].type == TokenType::Comma) {
                    hasComma = true;
                    i++;
                    if (i + 1 < tokens.size() && tokens[i+1].line == t.line) reg = tokens[++i].value;
                }
            }

            uint16_t val = EvaluateOperand(op, symbolTable);
            AddrMode mode = DetermineAddressingMode(mnemonic, op, hasComma, reg, val);
            
            if (OpcodeMap.find(mnemonic) != OpcodeMap.end()) {
                for (const auto& inst : OpcodeMap[mnemonic]) {
                    if (inst.mode == mode || (mode == AddrMode::Implied && inst.mode == AddrMode::Accumulator)) {
                        PC += inst.size;
                        break;
                    }
                }
            } else log += "Error: Unknown instruction " + mnemonic + "\n";
        }
    }
}


// ============================================================================
// FUNCTION: Assembler::PassTwo
// PURPOSE: 
//   Now that all labels and variables have known memory addresses, Pass Two 
//   generates the actual 6502 machine code and writes it to the RAM buffer.
//
// HOW IT WORKS:
//   - It resets the PC to 0x0000 and loops through the tokens again.
//   - Directives: It writes raw data bytes (`.BYTE`) or words (`.WORD`) directly 
//     into the RAM buffer at the current PC.
//   - Instructions: It matches the mnemonic and addressing mode to the Master 
//     OpcodeMap to get the correct hex opcode, and writes it to RAM.
//   - Operands: It evaluates the operands using the fully-populated Symbol Table.
//     If it is a relative branch (like BNE), it calculates the 8-bit jump offset 
//     and throws an error if the destination is too far away (>127 bytes).
// ============================================================================
void Assembler::PassTwo(const std::vector<Token>& tokens) {
    uint16_t PC = 0x0000;
    log += "Starting Pass 2...\n";

    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& t = tokens[i];

        if (t.type == TokenType::Directive) {
            if (t.value == ".ORG" && i + 1 < tokens.size()) PC = EvaluateOperand(tokens[++i].value, symbolTable);
            else if (t.value == ".BYTE" || t.value == ".DB") {
                int currentLine = t.line;
                while (i + 1 < tokens.size() && tokens[i+1].line == currentLine) {
                    i++;
                    if (tokens[i].type != TokenType::Comma) ram[PC++] = EvaluateOperand(tokens[i].value, symbolTable) & 0xFF;
                }
            }
            else if (t.value == ".WORD" || t.value == ".DW") {
                int currentLine = t.line;
                while (i + 1 < tokens.size() && tokens[i+1].line == currentLine) {
                    i++;
                    if (tokens[i].type != TokenType::Comma) {
                        uint16_t val = EvaluateOperand(tokens[i].value, symbolTable);
                        ram[PC++] = val & 0xFF;         
                        ram[PC++] = (val >> 8) & 0xFF;  
                    }
                }
            }
        }
        else if (t.type == TokenType::Label || 
                (t.type == TokenType::Instruction && i + 1 < tokens.size() && (tokens[i+1].value == "EQU" || tokens[i+1].value == "="))) {
            if (i + 1 < tokens.size() && (tokens[i+1].value == "EQU" || tokens[i+1].value == "=")) i += 2; 
        }
        else if (t.type == TokenType::Instruction) {
            std::string mnemonic = t.value;
            std::string op = "";
            bool hasComma = false;
            std::string reg = "";

            if (i + 1 < tokens.size() && tokens[i+1].line == t.line && 
               (tokens[i+1].type == TokenType::Operand || tokens[i+1].type == TokenType::Label || tokens[i+1].type == TokenType::Instruction)) {
                
                // ---> NEW FIX: Same logic for Pass 2 math token eating
                while (i + 1 < tokens.size() && tokens[i+1].line == t.line && tokens[i+1].type != TokenType::Comma) {
                    op += tokens[++i].value;
                }

                if (i + 1 < tokens.size() && tokens[i+1].type == TokenType::Comma) {
                    hasComma = true;
                    i++;
                    if (i + 1 < tokens.size() && tokens[i+1].line == t.line) reg = tokens[++i].value;
                }
            }

            uint16_t val = EvaluateOperand(op, symbolTable);
            AddrMode mode = DetermineAddressingMode(mnemonic, op, hasComma, reg, val);

            bool found = false;
            if (OpcodeMap.find(mnemonic) != OpcodeMap.end()) {
                for (const auto& inst : OpcodeMap[mnemonic]) {
                    if (inst.mode == mode || (mode == AddrMode::Implied && inst.mode == AddrMode::Accumulator)) {
                        ram[PC++] = inst.opcode; 
                        if (inst.size == 2) {
                            if (mode == AddrMode::Relative) {
                                int offset = val - PC - 1;
                                if (offset < -128 || offset > 127) log += "Error: Branch out of range at line " + std::to_string(t.line) + "\n";
                                ram[PC++] = static_cast<uint8_t>(offset);
                            } else ram[PC++] = val & 0xFF; 
                        } 
                        else if (inst.size == 3) {
                            ram[PC++] = val & 0xFF;         
                            ram[PC++] = (val >> 8) & 0xFF;  
                        }
                        found = true;
                        break;
                    }
                }
            }
            if (!found) log += "Error: Could not assemble '" + mnemonic + " " + op + "' at line " + std::to_string(t.line) + "\n";
        }
    }
    log += "Assembly Successful!\n";
}


// ============================================================================
// FUNCTION: Assembler::SaveBinary
// PURPOSE: 
//   Outputs the compiled 64KB RAM buffer to a physical file on your hard drive 
//   so it can be loaded into an emulator or burned to a real ROM chip.
//
// HOW IT WORKS:
//   - It opens a standard C++ file stream (`std::ofstream`) in binary mode.
//   - It dumps the entire `ram` vector into the file in one continuous block.
//   - It updates the build log to notify the user if the save succeeded or failed.
// ============================================================================
void Assembler::SaveBinary(const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(ram.data()), ram.size());
        file.close();
        log += "Saved binary to " + filepath + "\n";
    } else {
        log += "Failed to save binary to " + filepath + "\n";
    }
}
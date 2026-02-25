// ============================================================================
// Copyright (c) 2026 Andrew Young
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
// 
// ============================================================================
// FILE: lexer.cpp
// PURPOSE: 
//   Implements the Lexer class. This file takes a raw string of assembly 
//   code, strips out the comments, and chops the text line-by-line into a 
//   clean list of Tokens. It also forces text to uppercase 
//   to ensure the assembler is completely case-insensitive.
// ============================================================================

#include "Lexer.h"
#include <sstream>
#include <cctype>
#include <algorithm>

// ============================================================================
// FUNCTION: ToUpper
// PURPOSE: 
//   A simple helper function that takes a string and converts every character 
//   to uppercase. 
//
// HOW IT WORKS:
//   - It uses the C++ Standard Library's `std::transform` to iterate through 
//     the string from beginning to end, applying the standard `::toupper` 
//     function to each character. 
//   - This is crucial for the assembler because 6502 assembly is typically 
//     case-insensitive (e.g., "lda #$ff" and "LDA #$FF" mean the same thing).
// ============================================================================
std::string ToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// ============================================================================
// FUNCTION: Lexer::Tokenize
// PURPOSE: 
//   This is the core of the Lexer. It takes the giant block of raw text from 
//   the editor and chops it up into a structured list of Tokens that the 
//   Assembler can actually understand.
//
// HOW IT WORKS:
//   1. Line by Line: It uses `std::istringstream` to read the source code one 
//      line at a time, keeping track of the line number for error reporting.
//   2. Stripping Comments: If it finds a semicolon (`;`), it deletes everything 
//      after it on that line so the assembler ignores comments.
//   3. Word Extraction: It reads each word separated by spaces. It specifically 
//      checks for commas (e.g., `LDA $10,X`) and splits them into separate 
//      tokens so the assembler can easily detect indexing.
//   4. Categorization: It looks at the first and last characters of the word:
//      - Ends with ':' -> It's a Label.
//      - Starts with '.' or is "EQU"/"=" -> It's a Directive.
//      - Starts with '#', '$', '%', or a digit -> It's an Operand.
//      - Is "A", "X", or "Y" -> It's a Register (Operand).
//      - Anything else -> Defaults to an Instruction (Mnemonic).
// ============================================================================
std::vector<Token> Lexer::Tokenize(const std::string& source) {
    std::vector<Token> tokens;
    std::istringstream stream(source);
    std::string line;
    int lineNumber = 1;

    while (std::getline(stream, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);

        std::istringstream lineStream(line);
        std::string word;
        
        while (lineStream >> word) {
            Token t;
            t.line = lineNumber;
            
            bool hasComma = false;
            if (word.back() == ',') {
                hasComma = true;
                word.pop_back(); 
            } else if (size_t pos = word.find(','); pos != std::string::npos) {
                std::string firstPart = word.substr(0, pos);
                std::string secondPart = word.substr(pos + 1);
                
                t.value = ToUpper(firstPart);
                t.type = TokenType::Operand; 
                tokens.push_back(t);
                
                tokens.push_back({TokenType::Comma, ",", lineNumber});
                word = secondPart; 
            }

            std::string upperWord = ToUpper(word);

            if (word.back() == ':') {
                t.type = TokenType::Label;
                t.value = upperWord; // FIX: Force all labels to uppercase
            } 
            else if (word[0] == '.') {
                t.type = TokenType::Directive;
                t.value = upperWord; 
            } 
            else if (upperWord == "EQU" || upperWord == "=") {
                t.type = TokenType::Directive;
                t.value = upperWord;
            }
            else if (word[0] == '#' || word[0] == '$' || word[0] == '%' || isdigit(word[0])) {
                t.type = TokenType::Operand;
                t.value = upperWord; // FIX: Force hex/binary to uppercase
            }
            else if (upperWord == "A" || upperWord == "X" || upperWord == "Y") {
                t.type = TokenType::Operand;
                t.value = upperWord;
            }
            else {
                t.type = TokenType::Instruction;
                t.value = upperWord;
            }

            tokens.push_back(t);
            if (hasComma) tokens.push_back({TokenType::Comma, ",", lineNumber});
        }
        lineNumber++;
    }
    return tokens;
}
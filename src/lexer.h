// ============================================================================
// Copyright (c) 2026 Andrew Young
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
// 
// ============================================================================
// FILE: lexer.h
// PURPOSE: 
//   Declares the Lexer, which is responsible for the very first step of 
//   compilation: Tokenization. It defines the TokenType enum and the Token 
//   struct. This allows the raw text to be categorized into 
//   meaningful pieces (like Labels, Instructions, or Operands) before passing 
//   them to the Assembler.
// ============================================================================

#pragma once
#include <vector>
#include <string>
#include <regex>


enum class TokenType { Instruction, Label, Operand, Directive, Comma, Error };

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class Lexer {
public:
    std::vector<Token> Tokenize(const std::string& source);
};
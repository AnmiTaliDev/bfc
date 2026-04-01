// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "lexer.hpp"
#include <optional>

namespace bf {

Lexer::Lexer(std::string src, std::string filename)
    : src_(std::move(src)), filename_(std::move(filename)) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(src_.size() / 2);

    for (char ch : src_) {
        std::optional<TokenKind> kind;
        switch (ch) {
            case '>': kind = TokenKind::MoveRight; break;
            case '<': kind = TokenKind::MoveLeft;  break;
            case '+': kind = TokenKind::Increment; break;
            case '-': kind = TokenKind::Decrement; break;
            case '.': kind = TokenKind::Output;    break;
            case ',': kind = TokenKind::Input;     break;
            case '[': kind = TokenKind::LoopBegin; break;
            case ']': kind = TokenKind::LoopEnd;   break;
            case '\n':
                ++line_;
                col_ = 1;
                continue;
            default:
                ++col_;
                continue;
        }
        tokens.push_back({*kind, line_, col_});
        ++col_;
    }
    tokens.push_back({TokenKind::Eof, line_, col_});
    return tokens;
}

} // namespace bf

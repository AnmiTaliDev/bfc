// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once
#include "bf_types.hpp"
#include <stdexcept>
#include <string>
#include <vector>

namespace bf {

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens, std::string filename = "<stdin>");

    std::vector<BFNode> parse();

private:
    std::vector<BFNode> parseBlock(bool nested);

    std::vector<Token> tokens_;
    std::string        filename_;
    size_t             pos_ = 0;

    const Token& current() const { return tokens_[pos_]; }
    const Token& consume()       { return tokens_[pos_++]; }
    bool         atEnd()   const { return tokens_[pos_].kind == TokenKind::Eof; }
};

} // namespace bf

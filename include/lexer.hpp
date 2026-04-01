// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once
#include "bf_types.hpp"
#include <string>
#include <vector>

namespace bf {

class Lexer {
public:
    explicit Lexer(std::string src, std::string filename = "<stdin>");

    std::vector<Token> tokenize();

    const std::string& filename() const { return filename_; }

private:
    std::string src_;
    std::string filename_;
    size_t      pos_  = 0;
    uint32_t    line_ = 1;
    uint32_t    col_  = 1;
};

} // namespace bf

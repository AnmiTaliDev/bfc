// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "lexer.hpp"
#include <cassert>
#include <iostream>

using namespace bf;

static void test_basic_tokens() {
    Lexer lex("><+-.,[]", "test");
    auto tokens = lex.tokenize();

    assert(tokens.size() == 9); // 8 ops + EOF
    assert(tokens[0].kind == TokenKind::MoveRight);
    assert(tokens[1].kind == TokenKind::MoveLeft);
    assert(tokens[2].kind == TokenKind::Increment);
    assert(tokens[3].kind == TokenKind::Decrement);
    assert(tokens[4].kind == TokenKind::Output);
    assert(tokens[5].kind == TokenKind::Input);
    assert(tokens[6].kind == TokenKind::LoopBegin);
    assert(tokens[7].kind == TokenKind::LoopEnd);
    assert(tokens[8].kind == TokenKind::Eof);
    std::cout << "test_basic_tokens: OK\n";
}

static void test_comments_ignored() {
    Lexer lex("+ this is a comment - more comment +", "test");
    auto tokens = lex.tokenize();

    // Should only see +, -, + and EOF
    assert(tokens.size() == 4);
    assert(tokens[0].kind == TokenKind::Increment);
    assert(tokens[1].kind == TokenKind::Decrement);
    assert(tokens[2].kind == TokenKind::Increment);
    assert(tokens[3].kind == TokenKind::Eof);
    std::cout << "test_comments_ignored: OK\n";
}

static void test_empty_input() {
    Lexer lex("", "test");
    auto tokens = lex.tokenize();
    assert(tokens.size() == 1);
    assert(tokens[0].kind == TokenKind::Eof);
    std::cout << "test_empty_input: OK\n";
}

static void test_line_tracking() {
    Lexer lex("+\n+\n+", "test");
    auto tokens = lex.tokenize();
    assert(tokens[0].line == 1);
    assert(tokens[1].line == 2);
    assert(tokens[2].line == 3);
    std::cout << "test_line_tracking: OK\n";
}

int main() {
    test_basic_tokens();
    test_comments_ignored();
    test_empty_input();
    test_line_tracking();
    std::cout << "All lexer tests passed.\n";
    return 0;
}

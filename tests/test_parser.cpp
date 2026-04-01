// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "lexer.hpp"
#include "parser.hpp"
#include <cassert>
#include <iostream>

using namespace bf;

static std::vector<BFNode> parse(const std::string& src) {
    Lexer  lex(src, "test");
    Parser p(lex.tokenize(), "test");
    return p.parse();
}

static void test_simple_ops() {
    auto ast = parse("+-><.,");
    assert(ast.size() == 6);
    assert(ast[0].kind == BFNode::Kind::Add  && ast[0].value ==  1);
    assert(ast[1].kind == BFNode::Kind::Add  && ast[1].value == -1);
    assert(ast[2].kind == BFNode::Kind::Move && ast[2].value ==  1);
    assert(ast[3].kind == BFNode::Kind::Move && ast[3].value == -1);
    assert(ast[4].kind == BFNode::Kind::Output);
    assert(ast[5].kind == BFNode::Kind::Input);
    std::cout << "test_simple_ops: OK\n";
}

static void test_loop() {
    auto ast = parse("[+]");
    assert(ast.size() == 1);
    assert(ast[0].kind == BFNode::Kind::Loop);
    assert(ast[0].children.size() == 1);
    assert(ast[0].children[0].kind == BFNode::Kind::Add);
    std::cout << "test_loop: OK\n";
}

static void test_nested_loops() {
    auto ast = parse("[[+]]");
    assert(ast.size() == 1);
    assert(ast[0].kind == BFNode::Kind::Loop);
    assert(ast[0].children[0].kind == BFNode::Kind::Loop);
    std::cout << "test_nested_loops: OK\n";
}

static void test_unmatched_close_throws() {
    Lexer  lex("]", "test");
    Parser p(lex.tokenize(), "test");
    bool threw = false;
    try { p.parse(); }
    catch (const ParseError&) { threw = true; }
    assert(threw);
    std::cout << "test_unmatched_close_throws: OK\n";
}

static void test_unmatched_open_throws() {
    Lexer  lex("[+", "test");
    Parser p(lex.tokenize(), "test");
    bool threw = false;
    try { p.parse(); }
    catch (const ParseError&) { threw = true; }
    assert(threw);
    std::cout << "test_unmatched_open_throws: OK\n";
}

int main() {
    test_simple_ops();
    test_loop();
    test_nested_loops();
    test_unmatched_close_throws();
    test_unmatched_open_throws();
    std::cout << "All parser tests passed.\n";
    return 0;
}

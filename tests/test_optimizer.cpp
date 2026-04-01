// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "lexer.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include <cassert>
#include <iostream>

using namespace bf;

static std::vector<BFNode> compile(const std::string& src, OptLevel lvl) {
    Lexer  lex(src, "test");
    Parser p(lex.tokenize(), "test");
    auto   ast = p.parse();
    return Optimizer(lvl).optimize(std::move(ast));
}

static void test_o0_no_change() {
    auto ast = compile("++++", OptLevel::O0);
    assert(ast.size() == 4);
    std::cout << "test_o0_no_change: OK\n";
}

static void test_contract_add() {
    auto ast = compile("++++", OptLevel::O1);
    assert(ast.size() == 1);
    assert(ast[0].kind == BFNode::Kind::Add && ast[0].value == 4);
    std::cout << "test_contract_add: OK\n";
}

static void test_contract_move() {
    auto ast = compile(">>>", OptLevel::O1);
    assert(ast.size() == 1);
    assert(ast[0].kind == BFNode::Kind::Move && ast[0].value == 3);
    std::cout << "test_contract_move: OK\n";
}

static void test_cancel_ops() {
    auto ast = compile("+-", OptLevel::O1);
    assert(ast.empty());
    std::cout << "test_cancel_ops: OK\n";
}

static void test_recognize_clear() {
    auto ast = compile("[-]", OptLevel::O1);
    assert(ast.size() == 1);
    assert(ast[0].kind == BFNode::Kind::Clear);
    std::cout << "test_recognize_clear: OK\n";
}

static void test_recognize_set() {
    auto ast = compile("[-]+++", OptLevel::O2);
    assert(ast.size() == 1);
    assert(ast[0].kind == BFNode::Kind::Set && ast[0].value == 3);
    std::cout << "test_recognize_set: OK\n";
}

static void test_nested_contract() {
    auto ast = compile("[+++]", OptLevel::O1);
    assert(ast.size() == 1);
    assert(ast[0].kind == BFNode::Kind::Loop);
    assert(ast[0].children.size() == 1);
    assert(ast[0].children[0].value == 3);
    std::cout << "test_nested_contract: OK\n";
}

int main() {
    test_o0_no_change();
    test_contract_add();
    test_contract_move();
    test_cancel_ops();
    test_recognize_clear();
    test_recognize_set();
    test_nested_contract();
    std::cout << "All optimizer tests passed.\n";
    return 0;
}

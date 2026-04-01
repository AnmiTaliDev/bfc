// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "parser.hpp"
#include <string>

namespace bf {

Parser::Parser(std::vector<Token> tokens, std::string filename)
    : tokens_(std::move(tokens)), filename_(std::move(filename)) {}

std::vector<BFNode> Parser::parse() {
    return parseBlock(false);
}

std::vector<BFNode> Parser::parseBlock(bool nested) {
    std::vector<BFNode> nodes;

    while (!atEnd()) {
        const Token& tok = current();

        if (tok.kind == TokenKind::LoopEnd) {
            if (!nested) {
                throw ParseError(filename_ + ":" + std::to_string(tok.line) +
                                 ":" + std::to_string(tok.col) + ": unmatched ']'");
            }
            consume();
            return nodes;
        }

        switch (tok.kind) {
            case TokenKind::MoveRight:
                nodes.push_back({BFNode::Kind::Move, +1, {}});
                break;
            case TokenKind::MoveLeft:
                nodes.push_back({BFNode::Kind::Move, -1, {}});
                break;
            case TokenKind::Increment:
                nodes.push_back({BFNode::Kind::Add, +1, {}});
                break;
            case TokenKind::Decrement:
                nodes.push_back({BFNode::Kind::Add, -1, {}});
                break;
            case TokenKind::Output:
                nodes.push_back({BFNode::Kind::Output, 0, {}});
                break;
            case TokenKind::Input:
                nodes.push_back({BFNode::Kind::Input, 0, {}});
                break;
            case TokenKind::LoopBegin: {
                consume();
                BFNode loop;
                loop.kind     = BFNode::Kind::Loop;
                loop.children = parseBlock(true);
                nodes.push_back(std::move(loop));
                continue;
            }
            default:
                break;
        }
        consume();
    }

    if (nested)
        throw ParseError(filename_ + ": unmatched '['");

    return nodes;
}

} // namespace bf

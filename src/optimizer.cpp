// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "optimizer.hpp"

namespace bf {

std::vector<BFNode> Optimizer::optimize(std::vector<BFNode> nodes) {
    if (level_ == OptLevel::O0)
        return nodes;

    auto contract = [this](auto n) { return contractOps(std::move(n)); };
    auto clear    = [this](auto n) { return recognizeClear(std::move(n)); };
    auto set      = [this](auto n) { return recognizeSet(std::move(n)); };
    auto dead     = [this](auto n) { return removeDeadOps(std::move(n)); };

    nodes = applyRecursive(std::move(nodes), contract);
    nodes = applyRecursive(std::move(nodes), clear);

    if (level_ >= OptLevel::O2)
        nodes = applyRecursive(std::move(nodes), set);

    if (level_ >= OptLevel::O3)
        nodes = applyRecursive(std::move(nodes), dead);

    return nodes;
}

std::vector<BFNode> Optimizer::applyRecursive(
    std::vector<BFNode> nodes,
    std::function<std::vector<BFNode>(std::vector<BFNode>)> pass)
{
    for (auto& n : nodes) {
        if (n.kind == BFNode::Kind::Loop)
            n.children = applyRecursive(std::move(n.children), pass);
    }
    return pass(std::move(nodes));
}

std::vector<BFNode> Optimizer::contractOps(std::vector<BFNode> nodes) {
    std::vector<BFNode> out;
    out.reserve(nodes.size());

    for (auto& n : nodes) {
        bool contractable = (n.kind == BFNode::Kind::Move || n.kind == BFNode::Kind::Add);
        if (contractable && !out.empty() && out.back().kind == n.kind) {
            out.back().value += n.value;
            if (out.back().value == 0)
                out.pop_back();
        } else {
            out.push_back(std::move(n));
        }
    }
    return out;
}

std::vector<BFNode> Optimizer::recognizeClear(std::vector<BFNode> nodes) {
    std::vector<BFNode> out;
    out.reserve(nodes.size());

    for (auto& n : nodes) {
        if (n.kind == BFNode::Kind::Loop && n.children.size() == 1 &&
            n.children[0].kind == BFNode::Kind::Add &&
            (n.children[0].value == 1 || n.children[0].value == -1))
        {
            out.push_back({BFNode::Kind::Clear, 0, {}});
        } else {
            out.push_back(std::move(n));
        }
    }
    return out;
}

std::vector<BFNode> Optimizer::recognizeSet(std::vector<BFNode> nodes) {
    std::vector<BFNode> out;
    out.reserve(nodes.size());

    for (auto& n : nodes) {
        if (!out.empty() && out.back().kind == BFNode::Kind::Clear &&
            n.kind == BFNode::Kind::Add)
        {
            out.back().kind  = BFNode::Kind::Set;
            out.back().value = n.value;
        } else {
            out.push_back(std::move(n));
        }
    }
    return out;
}

std::vector<BFNode> Optimizer::removeDeadOps(std::vector<BFNode> nodes) {
    std::vector<BFNode> out;
    out.reserve(nodes.size());

    for (auto& n : nodes) {
        bool dead = (n.kind == BFNode::Kind::Move || n.kind == BFNode::Kind::Add) &&
                    n.value == 0;
        if (!dead)
            out.push_back(std::move(n));
    }
    return out;
}

} // namespace bf

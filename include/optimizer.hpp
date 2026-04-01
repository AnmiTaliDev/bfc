// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once
#include "bf_types.hpp"
#include <functional>
#include <vector>

namespace bf {

class Optimizer {
public:
    explicit Optimizer(OptLevel level) : level_(level) {}

    std::vector<BFNode> optimize(std::vector<BFNode> nodes);

private:
    OptLevel level_;

    std::vector<BFNode> contractOps(std::vector<BFNode> nodes);
    std::vector<BFNode> recognizeClear(std::vector<BFNode> nodes);
    std::vector<BFNode> recognizeSet(std::vector<BFNode> nodes);
    std::vector<BFNode> removeDeadOps(std::vector<BFNode> nodes);

    std::vector<BFNode> applyRecursive(
        std::vector<BFNode> nodes,
        std::function<std::vector<BFNode>(std::vector<BFNode>)> pass);
};

} // namespace bf

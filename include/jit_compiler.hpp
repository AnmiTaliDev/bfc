// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once
#include "bf_types.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <memory>

namespace bf {

class JITCompiler {
public:
    explicit JITCompiler(const CompilerOptions& opts);

    // Takes ownership of result and executes main(). Returns exit code.
    int run(CodeGenResult result);

private:
    const CompilerOptions& opts_;
};

} // namespace bf

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once
#include "bf_types.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <memory>
#include <vector>

namespace bf {

class CodeGen {
public:
    explicit CodeGen(const CompilerOptions& opts);

    // Returns ownership of both ctx and module; caller is responsible for both.
    CodeGenResult generate(const std::vector<BFNode>& nodes);

private:
    void emitNodes(const std::vector<BFNode>& nodes);
    void emitMove(int value);
    void emitAdd(int value);
    void emitOutput();
    void emitInput();
    void emitLoop(const std::vector<BFNode>& body);
    void emitClear();
    void emitSet(int value);

    void runOptPasses();

    llvm::Value* loadCellPtr();
    llvm::Value* loadCellVal();

    const CompilerOptions& opts_;

    std::unique_ptr<llvm::LLVMContext> ctx_;
    std::unique_ptr<llvm::Module>      mod_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;

    llvm::Function* main_fn_    = nullptr;
    llvm::Value*    dp_alloca_  = nullptr;
    llvm::Value*    tape_base_  = nullptr;
    llvm::Function* putchar_fn_ = nullptr;
    llvm::Function* getchar_fn_ = nullptr;
    llvm::Function* calloc_fn_  = nullptr;
    llvm::Function* free_fn_    = nullptr;

    unsigned loop_id_ = 0;
};

} // namespace bf

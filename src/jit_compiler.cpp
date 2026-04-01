// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "jit_compiler.hpp"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

namespace bf {

JITCompiler::JITCompiler(const CompilerOptions& opts) : opts_(opts) {}

int JITCompiler::run(CodeGenResult result) {
    if (!result.mod || !result.ctx) {
        llvm::errs() << "bfc: jit: invalid module\n";
        return 1;
    }

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto jit_expected = llvm::orc::LLJITBuilder().create();
    if (!jit_expected) {
        llvm::errs() << "bfc: jit: failed to create LLJIT: "
                     << llvm::toString(jit_expected.takeError()) << "\n";
        return 1;
    }
    auto& jit = *jit_expected;

    llvm::orc::ThreadSafeModule tsm(std::move(result.mod), std::move(result.ctx));
    if (auto err = jit->addIRModule(std::move(tsm))) {
        llvm::errs() << "bfc: jit: failed to add module: "
                     << llvm::toString(std::move(err)) << "\n";
        return 1;
    }

    auto sym = jit->lookup("main");
    if (!sym) {
        llvm::errs() << "bfc: jit: symbol 'main' not found: "
                     << llvm::toString(sym.takeError()) << "\n";
        return 1;
    }

    auto* main_fn = sym->toPtr<int (*)()>();
    return main_fn();
}

} // namespace bf

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once
#include "bf_types.hpp"

#include <llvm/IR/Module.h>

#include <string>

namespace bf {

class AOTCompiler {
public:
    explicit AOTCompiler(const CompilerOptions& opts);

    // Compiles module according to output_kind in opts. Returns true on success.
    bool compile(llvm::Module& mod);

private:
    bool emitToFile(llvm::Module& mod, const std::string& path,
                    llvm::CodeGenFileType file_type);
    bool linkToExecutable(const std::string& obj_path, const std::string& exe_path);
    std::string deriveOutputPath() const;

    const CompilerOptions& opts_;
};

} // namespace bf

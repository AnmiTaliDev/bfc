// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "aot_compiler.hpp"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#include <filesystem>
#include <cstdlib>

namespace bf {

AOTCompiler::AOTCompiler(const CompilerOptions& opts) : opts_(opts) {}

std::string AOTCompiler::deriveOutputPath() const {
    if (!opts_.output_file.empty())
        return opts_.output_file;

    std::filesystem::path p(opts_.input_file);
    std::string stem = p.stem().string();
    if (stem.empty()) stem = "out";

    switch (opts_.output_kind) {
        case OutputKind::Object:    return stem + ".o";
        case OutputKind::Assembly:  return stem + ".ll";
        case OutputKind::Bitcode:   return stem + ".bc";
        default:                    return stem;
    }
}

bool AOTCompiler::compile(llvm::Module& mod) {
    if (opts_.output_kind == OutputKind::Bitcode) {
        std::string path = deriveOutputPath();
        std::error_code ec;
        llvm::raw_fd_ostream out(path, ec, llvm::sys::fs::OF_None);
        if (ec) {
            llvm::errs() << "bfc: cannot open '" << path << "': " << ec.message() << "\n";
            return false;
        }
        llvm::WriteBitcodeToFile(mod, out);
        if (opts_.verbose)
            llvm::errs() << "bfc: wrote bitcode to " << path << "\n";
        return true;
    }

    if (opts_.output_kind == OutputKind::Assembly) {
        std::string path = deriveOutputPath();
        std::error_code ec;
        llvm::raw_fd_ostream out(path, ec, llvm::sys::fs::OF_None);
        if (ec) {
            llvm::errs() << "bfc: cannot open '" << path << "': " << ec.message() << "\n";
            return false;
        }
        mod.print(out, nullptr);
        if (opts_.verbose)
            llvm::errs() << "bfc: wrote LLVM IR to " << path << "\n";
        return true;
    }

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    llvm::Triple triple(llvm::sys::getDefaultTargetTriple());
    mod.setTargetTriple(triple);

    std::string lookup_err;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple.str(), lookup_err);
    if (!target) {
        llvm::errs() << "bfc: target lookup failed: " << lookup_err << "\n";
        return false;
    }

    llvm::TargetOptions target_opts;
    auto reloc = llvm::Reloc::PIC_;
    auto tm = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(triple, "generic", "", target_opts, reloc));
    if (!tm) {
        llvm::errs() << "bfc: failed to create TargetMachine\n";
        return false;
    }

    mod.setDataLayout(tm->createDataLayout());

    std::string obj_path;
    if (opts_.output_kind == OutputKind::Object) {
        obj_path = deriveOutputPath();
    } else {
        // Temporary object file for linking
        obj_path = deriveOutputPath() + ".bfc.tmp.o";
    }

    if (!emitToFile(mod, obj_path, llvm::CodeGenFileType::ObjectFile))
        return false;

    if (opts_.output_kind == OutputKind::Object) {
        if (opts_.verbose)
            llvm::errs() << "bfc: wrote object file to " << obj_path << "\n";
        return true;
    }

    std::string exe_path = deriveOutputPath();
    bool ok = linkToExecutable(obj_path, exe_path);
    std::filesystem::remove(obj_path);

    if (ok && opts_.verbose)
        llvm::errs() << "bfc: wrote executable to " << exe_path << "\n";

    return ok;
}

bool AOTCompiler::emitToFile(llvm::Module& mod, const std::string& path,
                              llvm::CodeGenFileType file_type) {
    // Re-acquire TargetMachine (mod already has triple/datalayout set).
    llvm::Triple triple = mod.getTargetTriple();
    std::string lookup_err;
    const llvm::Target* target =
        llvm::TargetRegistry::lookupTarget(triple.str(), lookup_err);
    if (!target) {
        llvm::errs() << "bfc: " << lookup_err << "\n";
        return false;
    }

    llvm::TargetOptions target_opts;
    auto reloc = llvm::Reloc::PIC_;
    auto tm = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(triple, "generic", "", target_opts, reloc));

    std::error_code ec;
    llvm::raw_fd_ostream dest(path, ec, llvm::sys::fs::OF_None);
    if (ec) {
        llvm::errs() << "bfc: cannot open '" << path << "': " << ec.message() << "\n";
        return false;
    }

    llvm::legacy::PassManager pm;
    if (tm->addPassesToEmitFile(pm, dest, nullptr, file_type)) {
        llvm::errs() << "bfc: target does not support file emission\n";
        return false;
    }
    pm.run(mod);
    dest.flush();
    return true;
}

bool AOTCompiler::linkToExecutable(const std::string& obj_path,
                                    const std::string& exe_path) {
    std::string cmd = "cc " + obj_path + " -o " + exe_path;
    if (opts_.verbose)
        llvm::errs() << "bfc: link: " << cmd << "\n";

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        llvm::errs() << "bfc: linker exited with code " << ret << "\n";
        return false;
    }
    return true;
}

} // namespace bf

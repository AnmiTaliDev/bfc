// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace llvm {
class LLVMContext;
class Module;
}

namespace bf {

constexpr int         DEFAULT_TAPE_SIZE = 30000;
constexpr const char* BFC_VERSION       = "0.1.0";

enum class TokenKind : uint8_t {
    MoveRight,
    MoveLeft,
    Increment,
    Decrement,
    Output,
    Input,
    LoopBegin,
    LoopEnd,
    Eof
};

struct Token {
    TokenKind kind;
    uint32_t  line;
    uint32_t  col;
};

struct BFNode {
    enum class Kind : uint8_t {
        Move,
        Add,
        Output,
        Input,
        Loop,
        Clear,
        Set,
    };
    Kind             kind;
    int              value    = 0;
    std::vector<BFNode> children;
};

enum class OptLevel : uint8_t { O0, O1, O2, O3 };

enum class OutputKind : uint8_t {
    Executable,
    Object,
    Assembly,
    Bitcode,
    JIT,
};

struct CompilerOptions {
    std::string input_file;
    std::string output_file;
    OptLevel    opt_level   = OptLevel::O2;
    OutputKind  output_kind = OutputKind::Executable;
    int         tape_size   = DEFAULT_TAPE_SIZE;
    bool        verbose     = false;
    bool        dump_ast    = false;
    bool        dump_ir     = false;
};

struct CodeGenResult {
    std::unique_ptr<llvm::LLVMContext> ctx;
    std::unique_ptr<llvm::Module>      mod;
};

} // namespace bf

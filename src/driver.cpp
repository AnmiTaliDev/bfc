// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "driver.hpp"
#include "aot_compiler.hpp"
#include "codegen.hpp"
#include "jit_compiler.hpp"
#include "lexer.hpp"
#include "optimizer.hpp"
#include "parser.hpp"

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace bf {

static void dumpAst(const std::vector<BFNode>& nodes, int indent = 0) {
    auto pfx = std::string(indent * 2, ' ');
    for (const auto& n : nodes) {
        switch (n.kind) {
            case BFNode::Kind::Move:
                llvm::errs() << pfx << "Move(" << n.value << ")\n";
                break;
            case BFNode::Kind::Add:
                llvm::errs() << pfx << "Add(" << n.value << ")\n";
                break;
            case BFNode::Kind::Output:
                llvm::errs() << pfx << "Output\n";
                break;
            case BFNode::Kind::Input:
                llvm::errs() << pfx << "Input\n";
                break;
            case BFNode::Kind::Loop:
                llvm::errs() << pfx << "Loop {\n";
                dumpAst(n.children, indent + 1);
                llvm::errs() << pfx << "}\n";
                break;
            case BFNode::Kind::Clear:
                llvm::errs() << pfx << "Clear\n";
                break;
            case BFNode::Kind::Set:
                llvm::errs() << pfx << "Set(" << n.value << ")\n";
                break;
        }
    }
}

CompilerOptions Driver::parseArgs(int argc, char** argv) {
    CompilerOptions opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            std::exit(0);
        }
        if (arg == "--version") {
            printVersion();
            std::exit(0);
        }
        if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
            continue;
        }
        if (arg == "--jit") {
            opts.output_kind = OutputKind::JIT;
            continue;
        }
        if (arg == "-c") {
            opts.output_kind = OutputKind::Object;
            continue;
        }
        if (arg == "-S") {
            opts.output_kind = OutputKind::Assembly;
            continue;
        }
        if (arg == "--emit-llvm") {
            opts.output_kind = OutputKind::Bitcode;
            continue;
        }
        if (arg == "--dump-ast") {
            opts.dump_ast = true;
            continue;
        }
        if (arg == "--dump-ir") {
            opts.dump_ir = true;
            continue;
        }
        if (arg == "-O0") { opts.opt_level = OptLevel::O0; continue; }
        if (arg == "-O1") { opts.opt_level = OptLevel::O1; continue; }
        if (arg == "-O2") { opts.opt_level = OptLevel::O2; continue; }
        if (arg == "-O3") { opts.opt_level = OptLevel::O3; continue; }

        if (arg == "-o") {
            if (i + 1 >= argc)
                throw DriverError("bfc: -o requires an argument");
            opts.output_file = argv[++i];
            continue;
        }
        if (arg.substr(0, 2) == "-o") {
            opts.output_file = arg.substr(2);
            continue;
        }

        if (arg == "--tape-size") {
            if (i + 1 >= argc)
                throw DriverError("bfc: --tape-size requires an argument");
            opts.tape_size = std::stoi(argv[++i]);
            if (opts.tape_size <= 0)
                throw DriverError("bfc: --tape-size must be positive");
            continue;
        }
        if (arg.substr(0, 12) == "--tape-size=") {
            opts.tape_size = std::stoi(arg.substr(12));
            if (opts.tape_size <= 0)
                throw DriverError("bfc: --tape-size must be positive");
            continue;
        }

        if (!arg.empty() && arg[0] == '-')
            throw DriverError("bfc: unknown option: " + arg);

        if (!opts.input_file.empty())
            throw DriverError("bfc: multiple input files not supported");
        opts.input_file = arg;
    }

    if (opts.input_file.empty())
        throw DriverError("bfc: no input file");

    return opts;
}

void Driver::printHelp(const char* argv0) {
    llvm::outs()
        << "Usage: " << argv0 << " [options] <file>\n"
        << "\n"
        << "Options:\n"
        << "  -o <file>          Output file (default: derived from input)\n"
        << "  -O0                No optimization\n"
        << "  -O1                Basic optimization\n"
        << "  -O2                Standard optimization (default)\n"
        << "  -O3                Aggressive optimization\n"
        << "  --jit              Execute via JIT (no output file)\n"
        << "  -c                 Compile to object file\n"
        << "  -S                 Emit LLVM IR text\n"
        << "  --emit-llvm        Emit LLVM bitcode\n"
        << "  --tape-size <n>    Tape size in cells (default: 30000)\n"
        << "  --dump-ast         Print the AST to stderr\n"
        << "  --dump-ir          Print LLVM IR to stderr\n"
        << "  -v, --verbose      Verbose output\n"
        << "  --version          Print version and exit\n"
        << "  -h, --help         Print this help and exit\n";
}

void Driver::printVersion() {
    llvm::outs() << "bfc " << BFC_VERSION << "\n"
                 << "Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>\n"
                 << "License: GNU GPL version 3 or later\n";
}

int Driver::run(const CompilerOptions& opts) {
    // Read source
    std::ifstream file(opts.input_file);
    if (!file) {
        llvm::errs() << "bfc: cannot open '" << opts.input_file << "'\n";
        return 1;
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    std::string src = buf.str();

    if (opts.verbose)
        llvm::errs() << "bfc: compiling " << opts.input_file << "\n";

    // Lex
    Lexer lexer(src, opts.input_file);
    auto tokens = lexer.tokenize();

    // Parse
    Parser parser(std::move(tokens), opts.input_file);
    std::vector<BFNode> ast;
    try {
        ast = parser.parse();
    } catch (const ParseError& e) {
        llvm::errs() << e.what() << "\n";
        return 1;
    }

    if (opts.dump_ast)
        dumpAst(ast);

    // Optimize
    Optimizer optimizer(opts.opt_level);
    ast = optimizer.optimize(std::move(ast));

    // Generate LLVM IR
    CodeGen cg(opts);
    auto result = cg.generate(ast);
    if (!result.mod) {
        llvm::errs() << "bfc: code generation failed\n";
        return 1;
    }

    // JIT or AOT
    if (opts.output_kind == OutputKind::JIT) {
        JITCompiler jit(opts);
        return jit.run(std::move(result));
    }

    AOTCompiler aot(opts);
    return aot.compile(*result.mod) ? 0 : 1;
}

} // namespace bf

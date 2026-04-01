// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "driver.hpp"

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/raw_ostream.h>

int main(int argc, char** argv) {
    llvm::InitLLVM x(argc, argv);

    bf::CompilerOptions opts;
    try {
        opts = bf::Driver::parseArgs(argc, argv);
    } catch (const bf::DriverError& e) {
        llvm::errs() << e.what() << "\n"
                     << "Try '" << argv[0] << " --help' for usage.\n";
        return 1;
    }

    bf::Driver driver;
    return driver.run(opts);
}

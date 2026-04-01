// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#pragma once
#include "bf_types.hpp"
#include <stdexcept>

namespace bf {

struct DriverError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Driver {
public:
    static CompilerOptions parseArgs(int argc, char** argv);
    static void            printHelp(const char* argv0);
    static void            printVersion();

    // Run the full compilation pipeline. Returns process exit code.
    int run(const CompilerOptions& opts);
};

} // namespace bf

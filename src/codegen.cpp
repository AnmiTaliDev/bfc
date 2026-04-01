// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 AnmiTaliDev <anmitalidev@nuros.org>

#include "codegen.hpp"

#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/raw_ostream.h>

#include <string>

namespace bf {

CodeGen::CodeGen(const CompilerOptions& opts) : opts_(opts) {
    ctx_     = std::make_unique<llvm::LLVMContext>();
    mod_     = std::make_unique<llvm::Module>("bfmodule", *ctx_);
    builder_ = std::make_unique<llvm::IRBuilder<>>(*ctx_);
}

CodeGenResult CodeGen::generate(const std::vector<BFNode>& nodes) {
    auto* i32ty  = builder_->getInt32Ty();
    auto* i64ty  = builder_->getInt64Ty();
    auto* ptrty  = llvm::PointerType::getUnqual(*ctx_);
    auto* voidty = builder_->getVoidTy();

    auto declareFunc = [&](const char* name, llvm::Type* ret,
                           std::initializer_list<llvm::Type*> params) -> llvm::Function* {
        auto* fty = llvm::FunctionType::get(ret, llvm::ArrayRef<llvm::Type*>(params), false);
        return llvm::cast<llvm::Function>(mod_->getOrInsertFunction(name, fty).getCallee());
    };

    putchar_fn_ = declareFunc("putchar", i32ty, {i32ty});
    getchar_fn_ = declareFunc("getchar", i32ty, {});
    calloc_fn_  = declareFunc("calloc",  ptrty, {i64ty, i64ty});
    free_fn_    = declareFunc("free",    voidty, {ptrty});

    auto* main_ty = llvm::FunctionType::get(i32ty, {}, false);
    main_fn_ = llvm::Function::Create(main_ty, llvm::Function::ExternalLinkage,
                                      "main", *mod_);
    auto* entry = llvm::BasicBlock::Create(*ctx_, "entry", main_fn_);
    builder_->SetInsertPoint(entry);

    auto* tape = builder_->CreateCall(
        calloc_fn_,
        {llvm::ConstantInt::get(i64ty, opts_.tape_size),
         llvm::ConstantInt::get(i64ty, 1)},
        "tape");

    // Store tape base separately so we can free it even after dp moves.
    auto* tape_base_alloca = builder_->CreateAlloca(ptrty, nullptr, "tape_base");
    builder_->CreateStore(tape, tape_base_alloca);
    tape_base_ = tape_base_alloca;

    dp_alloca_ = builder_->CreateAlloca(ptrty, nullptr, "dp");
    builder_->CreateStore(tape, dp_alloca_);

    emitNodes(nodes);

    auto* base = builder_->CreateLoad(ptrty, tape_base_, "tape_base_val");
    builder_->CreateCall(free_fn_, {base});
    builder_->CreateRet(llvm::ConstantInt::get(i32ty, 0));

    std::string err;
    llvm::raw_string_ostream es(err);
    if (llvm::verifyModule(*mod_, &es)) {
        llvm::errs() << "bfc: internal error: module verification failed: " << err << "\n";
        return {};
    }

    runOptPasses();

    if (opts_.dump_ir)
        mod_->print(llvm::errs(), nullptr);

    return {std::move(ctx_), std::move(mod_)};
}

void CodeGen::runOptPasses() {
    llvm::PassBuilder pb;
    llvm::LoopAnalysisManager     lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager    cgam;
    llvm::ModuleAnalysisManager   mam;

    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);

    llvm::OptimizationLevel lvl;
    switch (opts_.opt_level) {
        case OptLevel::O0: return;
        case OptLevel::O1: lvl = llvm::OptimizationLevel::O1; break;
        case OptLevel::O2: lvl = llvm::OptimizationLevel::O2; break;
        case OptLevel::O3: lvl = llvm::OptimizationLevel::O3; break;
    }

    auto mpm = pb.buildPerModuleDefaultPipeline(lvl);
    mpm.run(*mod_, mam);
}

void CodeGen::emitNodes(const std::vector<BFNode>& nodes) {
    for (const auto& n : nodes) {
        switch (n.kind) {
            case BFNode::Kind::Move:   emitMove(n.value);    break;
            case BFNode::Kind::Add:    emitAdd(n.value);     break;
            case BFNode::Kind::Output: emitOutput();         break;
            case BFNode::Kind::Input:  emitInput();          break;
            case BFNode::Kind::Loop:   emitLoop(n.children); break;
            case BFNode::Kind::Clear:  emitClear();          break;
            case BFNode::Kind::Set:    emitSet(n.value);     break;
        }
    }
}

llvm::Value* CodeGen::loadCellPtr() {
    return builder_->CreateLoad(llvm::PointerType::getUnqual(*ctx_), dp_alloca_, "dp");
}

llvm::Value* CodeGen::loadCellVal() {
    auto* dp = loadCellPtr();
    return builder_->CreateLoad(builder_->getInt8Ty(), dp, "cell");
}

void CodeGen::emitMove(int value) {
    auto* dp     = loadCellPtr();
    auto* new_dp = builder_->CreateGEP(
        builder_->getInt8Ty(), dp,
        llvm::ConstantInt::get(builder_->getInt32Ty(), value),
        "dp_moved");
    builder_->CreateStore(new_dp, dp_alloca_);
}

void CodeGen::emitAdd(int value) {
    auto* dp      = loadCellPtr();
    auto* val     = builder_->CreateLoad(builder_->getInt8Ty(), dp, "cell");
    auto* new_val = builder_->CreateAdd(
        val,
        llvm::ConstantInt::get(builder_->getInt8Ty(), static_cast<uint8_t>(value)),
        "cell_add");
    builder_->CreateStore(new_val, dp);
}

void CodeGen::emitOutput() {
    auto* val = loadCellVal();
    auto* ext = builder_->CreateZExt(val, builder_->getInt32Ty(), "cell_i32");
    builder_->CreateCall(putchar_fn_, {ext});
}

void CodeGen::emitInput() {
    auto* ret    = builder_->CreateCall(getchar_fn_, {}, "getchar_ret");
    // getchar() returns -1 on EOF; truncating -1 to i8 gives 0xFF (non-zero),
    // which would prevent loop termination. Store 0 on EOF instead.
    auto* is_eof = builder_->CreateICmpEQ(ret, builder_->getInt32(-1), "is_eof");
    auto* trunc  = builder_->CreateTrunc(ret, builder_->getInt8Ty(), "char_i8");
    auto* val    = builder_->CreateSelect(is_eof, builder_->getInt8(0), trunc, "input_val");
    builder_->CreateStore(val, loadCellPtr());
}

void CodeGen::emitLoop(const std::vector<BFNode>& body) {
    unsigned id = loop_id_++;
    auto*    fn = builder_->GetInsertBlock()->getParent();

    auto* cond_bb = llvm::BasicBlock::Create(*ctx_, "loop.cond." + std::to_string(id), fn);
    auto* body_bb = llvm::BasicBlock::Create(*ctx_, "loop.body." + std::to_string(id), fn);
    auto* exit_bb = llvm::BasicBlock::Create(*ctx_, "loop.exit." + std::to_string(id), fn);

    builder_->CreateBr(cond_bb);

    builder_->SetInsertPoint(cond_bb);
    auto* cmp = builder_->CreateICmpNE(loadCellVal(), builder_->getInt8(0), "loop_cond");
    builder_->CreateCondBr(cmp, body_bb, exit_bb);

    builder_->SetInsertPoint(body_bb);
    emitNodes(body);
    builder_->CreateBr(cond_bb);

    builder_->SetInsertPoint(exit_bb);
}

void CodeGen::emitClear() {
    builder_->CreateStore(builder_->getInt8(0), loadCellPtr());
}

void CodeGen::emitSet(int value) {
    builder_->CreateStore(
        llvm::ConstantInt::get(builder_->getInt8Ty(), static_cast<uint8_t>(value)),
        loadCellPtr());
}

} // namespace bf

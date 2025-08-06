//===----------------------------------------------------------------------===//
//
// HoistAnticipatedExpressionsPass - Hoists computations that are anticipated
// in all successor paths to reduce redundancy.
//
// Build:
//   mkdir build && cd build
//   cmake -DLLVM_DIR=$(llvm-config --cmakedir) ..
//   ninja
//
// Run:
//   opt -load-pass-plugin ./libHoistAnticipatedExpressions.so \
//       -passes=hoist-anticipated-expressions -disable-output test.ll
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/BreadthFirstIterator.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Debug.h"

#include <map>
#include <set>

using namespace llvm;

#define DEBUG_TYPE "hoist-anticipated-expressions"

namespace {

class HoistAnticipatedExpressionsPass
    : public PassInfoMixin<HoistAnticipatedExpressionsPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

private:
  bool isFunctionPure(CallInst *CI, const TargetLibraryInfo &TLI);
  bool isToBeIgnored(Instruction *I, const TargetLibraryInfo &TLI);
  void findUseSet(BasicBlock *BB,
                  std::map<BasicBlock *, std::set<Instruction *>> &UseSets,
                  const TargetLibraryInfo &TLI);
  void findDefSet(BasicBlock *BB,
                  std::map<BasicBlock *, std::set<Instruction *>> &DefSets);
  void findInSet(BasicBlock *BB,
                 std::map<BasicBlock *, std::set<Instruction *>> &UseSets,
                 std::map<BasicBlock *, std::set<Instruction *>> &DefSets,
                 std::map<BasicBlock *, std::set<Instruction *>> &InSets,
                 std::map<BasicBlock *, std::set<Instruction *>> &OutSets);
  void findOutSet(BasicBlock *BB,
                  std::map<BasicBlock *, std::set<Instruction *>> &UseSets,
                  std::map<BasicBlock *, std::set<Instruction *>> &DefSets,
                  std::map<BasicBlock *, std::set<Instruction *>> &InSets,
                  std::map<BasicBlock *, std::set<Instruction *>> &OutSets);
  Instruction *checkBeforeMove(BasicBlock *BB, Instruction *inst);
  bool hoistInstructions(BasicBlock *BB,
                          std::map<BasicBlock *, std::set<Instruction *>> &OutSets);
};

bool HoistAnticipatedExpressionsPass::isFunctionPure(CallInst *CI,
                                                     const TargetLibraryInfo &TLI) {
  Function *Called = CI->getCalledFunction();
  if (!Called)
    return false;

  LibFunc LF;

  if (Called->getReturnType()->isPointerTy())
    return false;

  for (auto &Arg : Called->args())
    if (Arg.getType()->isPointerTy())
      return false;

  if (!TLI.getLibFunc(Called->getName(), LF))
    return false;

  return true;
}

bool HoistAnticipatedExpressionsPass::isToBeIgnored(Instruction *I,
                                                    const TargetLibraryInfo &TLI) {
  if (isa<AllocaInst>(I))
    return true;
  if (auto *CI = dyn_cast<CallInst>(I))
    return !isFunctionPure(CI, TLI);
  return I->mayReadFromMemory() || I->mayHaveSideEffects() || I->isTerminator();
}

void HoistAnticipatedExpressionsPass::findUseSet(
    BasicBlock *BB, std::map<BasicBlock *, std::set<Instruction *>> &UseSets,
    const TargetLibraryInfo &TLI) {
  for (Instruction &I : *BB)
    if (!isToBeIgnored(&I, TLI) && !isa<PHINode>(I))
      UseSets[BB].insert(&I);
}

void HoistAnticipatedExpressionsPass::findDefSet(
    BasicBlock *BB, std::map<BasicBlock *, std::set<Instruction *>> &DefSets) {
  for (Instruction &I : *BB)
    for (Use &U : I.uses())
      if (auto *UI = dyn_cast<Instruction>(U.getUser()))
        if (BB == UI->getParent())
          DefSets[BB].insert(UI);
}

void HoistAnticipatedExpressionsPass::findInSet(
    BasicBlock *BB, std::map<BasicBlock *, std::set<Instruction *>> &UseSets,
    std::map<BasicBlock *, std::set<Instruction *>> &DefSets,
    std::map<BasicBlock *, std::set<Instruction *>> &InSets,
    std::map<BasicBlock *, std::set<Instruction *>> &OutSets) {
  for (auto *I : OutSets[BB])
    if (std::none_of(DefSets[BB].begin(), DefSets[BB].end(),
                     [&](Instruction *DI) { return I->isIdenticalTo(DI); }))
      InSets[BB].insert(I);

  for (auto *I : UseSets[BB])
    if (std::none_of(DefSets[BB].begin(), DefSets[BB].end(),
                     [&](Instruction *DI) { return I->isIdenticalTo(DI); }))
      InSets[BB].insert(I);
}

void HoistAnticipatedExpressionsPass::findOutSet(
    BasicBlock *BB, std::map<BasicBlock *, std::set<Instruction *>> &UseSets,
    std::map<BasicBlock *, std::set<Instruction *>> &DefSets,
    std::map<BasicBlock *, std::set<Instruction *>> &InSets,
    std::map<BasicBlock *, std::set<Instruction *>> &OutSets) {
  std::map<Instruction *, int> count;
  int totalSucc = succ_size(BB);

  for (BasicBlock *Succ : successors(BB)) {
    std::map<Instruction *, bool> seen;
    for (auto *I : InSets[Succ])
      seen[I] = false;

    for (auto *I : InSets[Succ]) {
      bool matched = false;
      for (auto &KV : count)
        if (I->isIdenticalTo(KV.first) && !seen[KV.first]) {
          KV.second++;
          matched = true;
          seen[KV.first] = true;
        }
      if (!matched)
        count[I] = 1;
    }
  }

  for (auto &KV : count)
    if (KV.second == totalSucc)
      OutSets[BB].insert(KV.first);
}

Instruction *HoistAnticipatedExpressionsPass::checkBeforeMove(
    BasicBlock *BB, Instruction *Inst) {
  for (Instruction &I : *BB)
    if (I.isIdenticalTo(Inst))
      return &I;
  return nullptr;
}

bool HoistAnticipatedExpressionsPass::hoistInstructions(
    BasicBlock *BB, std::map<BasicBlock *, std::set<Instruction *>> &OutSets) {
  bool Changed = false;
  std::set<Instruction *> ToDelete;

  for (auto *Orig : OutSets[BB]) {
    auto *Inst = Orig;
    Changed = true;
    auto *End = BB->getTerminator();
    if (auto *Existing = checkBeforeMove(BB, Inst))
      Inst = Existing;
    else
      Inst->moveBefore(End); // pointer form works in LLVM 22

    for (BasicBlock *Succ : breadth_first(BB))
      for (Instruction &I : *Succ)
        if (I.isIdenticalTo(Inst) && &I != Inst) {
          I.replaceAllUsesWith(Inst);
          ToDelete.insert(&I);
        }
  }

  for (auto *I : ToDelete)
    I->eraseFromParent();

  return Changed;
}

PreservedAnalyses HoistAnticipatedExpressionsPass::run(Function &F,
                                                       FunctionAnalysisManager &AM) {
  const auto &TLI = AM.getResult<TargetLibraryAnalysis>(F);

  bool Changed = true;
  while (Changed) {
    Changed = false;
    std::map<BasicBlock *, std::set<Instruction *>> InSets, OutSets, UseSets, DefSets;

    for (BasicBlock *BB : post_order(&F.getEntryBlock())) {
      findUseSet(BB, UseSets, TLI);
      findDefSet(BB, DefSets);
      findOutSet(BB, UseSets, DefSets, InSets, OutSets);
      findInSet(BB, UseSets, DefSets, InSets, OutSets);
    }

    for (BasicBlock *BB : breadth_first(&F.getEntryBlock()))
      if (hoistInstructions(BB, OutSets)) {
        Changed = true;
        break;
      }
  }

  return PreservedAnalyses::none();
}

} // namespace

//===----------------------------------------------------------------------===//
// Pass Registration for LLVM 22.0 (NPM Only)
//===----------------------------------------------------------------------===//

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "hoist-anticipated-expressions",
          LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hoist-anticipated-expressions") {
                    FPM.addPass(HoistAnticipatedExpressionsPass());
                    return true;
                  }
                  return false;
                });
          }};
}

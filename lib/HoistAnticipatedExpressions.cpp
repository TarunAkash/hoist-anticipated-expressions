#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct HoistAnticipatedExpressionsPass : public PassInfoMixin<HoistAnticipatedExpressionsPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    errs() << "[hoist-anticipated-expressions] Running on function: " << F.getName() << "\n";
    
    // This is where your actual analysis and transformation logic will go

    return PreservedAnalyses::all(); // For now, we say we didn't modify anything
  }
};

} // end anonymous namespace

// Register the pass with LLVM so it can be used via `opt -passes=...`
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION,
    "HoistAnticipatedExpressions", // name of pass
    LLVM_VERSION_STRING,
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "hoist-anticipated-expressions") {
            FPM.addPass(HoistAnticipatedExpressionsPass());
            return true;
          }
          return false;
        }
      );
    }
  };
}


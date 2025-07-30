#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/InitLLVM.h"

using namespace llvm;

// Declare function from lib
extern void runAnticipatedExprAnalysis(Function &F);

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);
  LLVMContext Context;
  SMDiagnostic Err;

  if (argc < 2) {
    errs() << "Usage: " << argv[0] << " <input.ll>\n";
    return 1;
  }

  std::unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);
  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }

  for (Function &F : *M) {
    if (!F.isDeclaration()) {
      errs() << "\nAnalyzing Function: " << F.getName() << "\n";
      runAnticipatedExprAnalysis(F);
    }
  }

  return 0;
}


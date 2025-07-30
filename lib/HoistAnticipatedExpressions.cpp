#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <set>
#include <map>
#include <sstream>
#include <algorithm>

using namespace llvm;

// Convert binary instruction to a string like "%a add %b"
std::string exprToStr(Instruction *I) {
    if (auto *BO = dyn_cast<BinaryOperator>(I)) {
        std::string s;
        raw_string_ostream os(s);
        BO->getOperand(0)->printAsOperand(os, false);
        os << " " << BO->getOpcodeName() << " ";
        BO->getOperand(1)->printAsOperand(os, false);
        return os.str();
    }
    return "";
}

// Helper: extract operands like "%a" and "%b" from "%a add %b"
std::set<std::string> extractOperandsFromExpr(const std::string &expr) {
    std::set<std::string> operands;
    std::istringstream iss(expr);
    std::string token;
    while (iss >> token) {
        if (!token.empty() && token[0] == '%') {
            operands.insert(token);
        }
    }
    return operands;
}

void runAnticipatedExprAnalysis(Function &F) {
    std::map<BasicBlock*, std::set<std::string>> GEN, USE, KILL;
    std::map<BasicBlock*, std::set<std::string>> IN, OUT;
    std::set<std::string> allExprs;

    // First pass: collect all expressions in the function
    for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
            if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
                std::string expr = exprToStr(&I);
                allExprs.insert(expr);
            }
        }
    }

    // Second pass: compute GEN, USE, KILL
    for (BasicBlock &BB : F) {
        std::set<std::string> gen, use, kill, defined;

        for (Instruction &I : BB) {
            // GEN: expression defined in this block
            if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
                std::string expr = exprToStr(&I);
                gen.insert(expr);
            }

            // USE: operands used before defined
            for (unsigned i = 0; i < I.getNumOperands(); ++i) {
                Value *op = I.getOperand(i);
                if (isa<Instruction>(op) || isa<Argument>(op)) {
                    std::string opName = op->getName().str();
                    if (!opName.empty() && !defined.count(opName)) {
                        use.insert(opName);
                    }
                }
            }

            // DEF: track what's defined in this block
            if (I.hasName()) {
                defined.insert(I.getName().str());
            }
        }

        // KILL: any expression using a redefined variable
        for (const std::string &expr : allExprs) {
            std::set<std::string> operands = extractOperandsFromExpr(expr);
            for (const std::string &def : defined) {
                if (operands.count("%" + def)) {
                    kill.insert(expr);
                    break;
                }
            }
        }

        GEN[&BB] = gen;
        USE[&BB] = use;
        KILL[&BB] = kill;
    }

    // Initialize IN and OUT
    for (BasicBlock &BB : F) {
        IN[&BB] = allExprs;  // optimistic: assume everything is anticipated
        OUT[&BB] = {};       // nothing anticipated after block initially
    }

    // Dataflow iteration (backward analysis)
    bool changed = true;
    while (changed) {
        changed = false;

        for (BasicBlock &BB : F) {
            BasicBlock *B = &BB;

            // OUT[B] = intersection of IN[succ]
            std::set<std::string> newOUT;
            bool first = true;
            for (BasicBlock *Succ : successors(B)) {
                if (first) {
                    newOUT = IN[Succ];
                    first = false;
                } else {
                    std::set<std::string> temp;
                    std::set_intersection(newOUT.begin(), newOUT.end(),
                                          IN[Succ].begin(), IN[Succ].end(),
                                          std::inserter(temp, temp.begin()));
                    newOUT = temp;
                }
            }

            if (first) newOUT = {}; // no successors (e.g., return block)

            // IN[B] = USE[B] ∪ (OUT[B] − KILL[B])
            std::set<std::string> outMinusKill;
            std::set_difference(newOUT.begin(), newOUT.end(),
                                KILL[B].begin(), KILL[B].end(),
                                std::inserter(outMinusKill, outMinusKill.begin()));

            std::set<std::string> newIN = USE[B];
            newIN.insert(outMinusKill.begin(), outMinusKill.end());

            // Update if changed
            if (newIN != IN[B] || newOUT != OUT[B]) {
                IN[B] = newIN;
                OUT[B] = newOUT;
                changed = true;
            }
        }
    }

    // Print all sets for debugging/inspection
    auto printSet = [](const std::string &label, const std::map<BasicBlock*, std::set<std::string>> &M) {
        errs() << "\n" << label << " sets:\n";
        for (auto &entry : M) {
            BasicBlock *BB = entry.first;
            errs() << "BasicBlock: " << BB->getName() << "\n";
            for (const std::string &expr : entry.second) {
                errs() << "  " << expr << "\n";
            }
        }
    };

    printSet("GEN", GEN);
    printSet("USE", USE);
    printSet("KILL", KILL);
    printSet("IN", IN);
    printSet("OUT", OUT);
}


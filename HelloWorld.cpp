#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"  // For successors()
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

namespace {
struct Hello : public ModulePass {
    static char ID;
    Hello() : ModulePass(ID) {}
    /* A data structure stores the valid set of control-flow transitions, from
     * source to destination. */
    /* One source could have multiple valid destionations. */
    DenseMap<int, SmallVector<int>> src2dstMap;


    DenseMap<BasicBlock *, int> BBIDMap;
    /* A function to assign a unique ID to each basic block in the module. */
    bool assignBasicBocksIDs(Module &M) {
        /* Iterate each basic blocks */
        int id = 0;
        for (auto &F : M) {
            for (auto &BB : F) {
                /* Assign a ID for each blocks */
                BBIDMap[&BB] = id;
                id++;
            }
        }
        return false;
    }

    bool findIndirectJump(Module &M) {
        // Iterate over all instructions in the module
        for (auto &F : M) {
            for (auto &BB : F) {
                for (auto &I : BB) {
                    /* Check for Call or Invoke instructions */
                    // e.g., if (I is an indirect call) {  
                    if(auto *CB = dyn_cast<CallBase>(&I)){
                        /* Get the callee operand (The target to be invoked) */
                            //get the callee operand (the target being called)
                            Value *CalledOperand = CB->getCalledOperand();
                            // Check if the callee operand is a function pointer
                            if(!isa<Function>(CalledOperand)){
                                errs() << "Indirect call found in function: " << F.getName() << "\n";
                                  // Get the function type of the indirect call
                                  // e.g., FuncType = dyn_cast<FunctionType>(Call->getFunctionType());
                                FunctionType *FuncType = CB->getFunctionType();

                                 // Iterate over all functions in the module and find the functioins with matched signature
                                for (auto &Func : M.functions()) {
                                    /* Match function signature */
                                    // e.g., if (Func.getFunctionType() == FuncType) {
                                    if(Func.getFunctionType()==FuncType){
                                        errs() << "Potential target: " << Func.getName() << "\n"; 
                                        

                                        /* Store the valid transition into the Map src2dstMap */
                                        if (!Func.empty()) { // Ensure the function has an entry block.
                                             BasicBlock *EntryBB = &Func.getEntryBlock();

                                            // Retrieve the IDs for the current basic block (source) and the target's entry block (destination).
                                            int srcID = BBIDMap[&BB];
                                            int dstID = BBIDMap[EntryBB];

                                             // Record the allowed transition in the map.
                                            // For a given source basic block (srcID), we add the destination ID (dstID)
                                            // to the vector of allowed targets.
                                            src2dstMap[srcID].push_back(dstID);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    // This function retrieves the `initCFG` function from the module, or declares
    Function *getInitCFGFunction(Module *M) {
        static const std::string InitCFGName = "initCFG";
        // Check if the function already exists in the module
        if (Function *F = M->getFunction(InitCFGName)) {
            return F;
        }

        // Declare the `initCFG` function
        LLVMContext &Context = M->getContext();
        FunctionType *InitCFGType =
            FunctionType::get(Type::getVoidTy(Context), false);
        return Function::Create(InitCFGType, Function::ExternalLinkage,
                                InitCFGName, M);
    }

    Function *getCFICheckFunction(Module *M) {
        static const std::string CFICheckFuncName = "__cfi_check_bb";

        // Check if the function already exists in the module
        if (Function *F = M->getFunction(CFICheckFuncName)) {
            return F;
        }

        // Declare the CFI runtime check function
        LLVMContext &Context = M->getContext();
        FunctionType *CheckFuncType = FunctionType::get(
            Type::getVoidTy(Context), {Type::getInt32Ty(Context)}, false);
        Function *CFICheckFunc = Function::Create(
            CheckFuncType, Function::ExternalLinkage, CFICheckFuncName, M);

        return CFICheckFunc;
    }

    bool instrumentInitCFGFunction(Module &M) {
        Function *InitCFG = getInitCFGFunction(&M);
        for (Function &F : M) {
            if (F.getName() == "main") {
                // Get the LLVM module and context
                Module *M = F.getParent();
                LLVMContext &Context = M->getContext();
                IRBuilder<> Builder(&*F.getEntryBlock().getFirstInsertionPt());

                // Declare or retrieve the `initCFG` function
                Function *InitCFGFunc = getInitCFGFunction(M);

                // Insert the call to `initCFG` at the beginning of the main
                // function
                Builder.CreateCall(InitCFGFunc);

                return true;  // Indicate that the function was modified
            }
        }

        return false;
    }

    bool instrumentCFIcheckFunction(Module &M) {
    // Iterate over all functions in the module
    for (auto &F : M) {
        // Iterate over all basic blocks in the function
        for (auto &BB : F) {
            // Get the ID of the current basic block
            // Use the position of the basic block in the function as its ID
            int targetBBID = BB.getName().empty() ? 0 : std::stoi(BB.getName().str());

            // Get the LLVM context and create an IRBuilder to insert the call
            LLVMContext &Context = M.getContext();
            IRBuilder<> Builder(&BB, BB.begin()); // Insert at the beginning of the basic block

            // Call __cfi_check_bb with the basic block ID as the argument
            Function *CFICheckFunc = M.getFunction("__cfi_check_bb");
            if (CFICheckFunc) {
                // Insert the call to __cfi_check_bb
                Builder.CreateCall(CFICheckFunc, {Builder.getInt32(targetBBID)});
            } else {
                errs() << "Error: __cfi_check_bb function not found in module.\n";
            }
        }
    }

    return true;
}

    bool runOnModule(Module &M) override {
      
        assignBasicBocksIDs(M);
        findIndirectJump(M);
        instrumentCFIcheckFunction(M);
        instrumentInitCFGFunction(M);

        return true;
    }
};  // end of struct Hello
}  // end of anonymous namespace

char Hello::ID = 0;
static RegisterPass<Hello> X("hello", "Hello World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                    PM.add(new Hello());
                                });

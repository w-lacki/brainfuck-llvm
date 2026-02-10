#include <fstream>
#include <iostream>
#include <llvm/Support/raw_ostream.h>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

enum Token {
    tok_eof = -1,
    tok_right = -2,
    tok_left = -3,
    tok_inc = -4,
    tok_dec = -5,
    tok_open = -6,
    tok_close = -7,
    tok_in = -8,
    tok_out = -9,
};


static llvm::LLVMContext *context;
static llvm::IRBuilder<> *builder;
static llvm::Module *module;

void emit_right(llvm::Value *position_ptr, int val) {
    llvm::Value *position = builder->CreateLoad(llvm::Type::getInt8Ty(*context), position_ptr, "position");
    llvm::Value *one = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), val);
    llvm::Value *incremented = builder->CreateAdd(position, one, "incremented_position");
    builder->CreateStore(incremented, position_ptr);
}


void emit_inc(llvm::Value *position_ptr, llvm::Type *tape_type, llvm::Value *memory_ptr, int val) {
    llvm::Value *position = builder->CreateLoad(llvm::Type::getInt8Ty(*context), position_ptr, "position");
    llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 0);

    llvm::Value *address = builder->CreateGEP(tape_type,
                                              memory_ptr,
                                              {
                                                  zero,
                                                  position,
                                              },
                                              "address");

    llvm::Value *curr = builder->CreateLoad(llvm::Type::getInt8Ty(*context), address, "current");
    llvm::Value *one = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), val);
    llvm::Value *incremented = builder->CreateAdd(curr, one, "address");
    builder->CreateStore(incremented, address);
}

void emit_print(llvm::Value *position_ptr, llvm::Type *tape_type, llvm::Value *memory_ptr) {
    llvm::FunctionType *print_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(*context),
                                                             {llvm::Type::getInt32Ty(*context)}, true);
    llvm::FunctionCallee printchar = module->getOrInsertFunction("putchar", print_type);

    llvm::Value *position = builder->CreateLoad(llvm::Type::getInt8Ty(*context), position_ptr, "position");
    llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 0);

    llvm::Value *address = builder->CreateGEP(tape_type,
                                              memory_ptr,
                                              {
                                                  zero,
                                                  position,
                                              },
                                              "address");

    llvm::Value *curr = builder->CreateLoad(llvm::Type::getInt8Ty(*context), address, "current");
    llvm::Value *val_i32 = builder->CreateZExt(curr, llvm::Type::getInt32Ty(*context), "cast_to_int");
    builder->CreateCall(print_type, printchar.getCallee(), {val_i32}, "putchar()");
}


int main() {
    context = new llvm::LLVMContext();
    module = new llvm::Module("top", *context);
    builder = new llvm::IRBuilder<>(*context);

    llvm::FunctionType *main_function_type = llvm::FunctionType::get(builder->getVoidTy(), false);
    llvm::Function *main = llvm::Function::Create(main_function_type, llvm::GlobalValue::ExternalLinkage, "main",
                                                  *module);

    llvm::BasicBlock *block = llvm::BasicBlock::Create(*context, "entry", main);
    builder->SetInsertPoint(block);

    llvm::AllocaInst *position = builder->CreateAlloca(llvm::Type::getInt8Ty(*context), 0, "position");
    llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt8Ty(*context), 0);
    builder->CreateStore(zero, position);

    llvm::ArrayType *tape_type = llvm::ArrayType::get(llvm::Type::getInt8Ty(*context), 2000);
    llvm::AllocaInst *tape = builder->CreateAlloca(tape_type, 0, "tape");
    llvm::Constant *zero_aggregator = llvm::ConstantAggregateZero::get(tape_type);
    builder->CreateStore(zero_aggregator, tape);

    int chr;
    while ((chr = getchar()) != EOF) {
        switch (chr) {
            case '+':
                emit_inc(position, tape_type, tape, 1);
                continue;
            case '-':
                emit_inc(position, tape_type, tape, -1);
                continue;
            case '>':
                emit_right(position, 1);
                continue;
            case '<':
                emit_right(position, -1);
                continue;
            case '.':
                emit_print(position, tape_type, tape);
        }
    }

    builder->CreateRetVoid();

    llvm::verifyFunction(*main);
    llvm::verifyModule(*module);

    module->print(llvm::outs(), nullptr);
    return 0;
}

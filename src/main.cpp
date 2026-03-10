#include <iostream>
#include <stack>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

constexpr int TAPE_LENGTH = 2048;

typedef struct {
    BasicBlock *cond_block;
    BasicBlock *outer_block;
} Loop;
std::stack<Loop> loop_stack;

LLVMContext *context = new LLVMContext();
Module *module = new Module("bf", *context);
IRBuilder<> *builder = new IRBuilder<>(*context);

Value *get_current_position_address(Value *position_ptr, Type *tape_type, Value *tape_ptr) {
    Value *position = builder->CreateLoad(Type::getInt32Ty(*context), position_ptr, "position");
    Value *zero = ConstantInt::get(Type::getInt32Ty(*context), 0);

    return builder->CreateGEP(tape_type,
                              tape_ptr,
                              {
                                  zero,
                                  position,
                              },
                              "address");
}

void emit_position_change(Value *position_ptr, int change_by) {
    Value *position = builder->CreateLoad(Type::getInt32Ty(*context), position_ptr, "position");
    Value *incremented = builder->CreateAdd(position,
                                            ConstantInt::get(Type::getInt32Ty(*context), change_by),
                                            "changed_position");

    builder->CreateStore(incremented, position_ptr);
}


void emit_value_change(Value *position_ptr, Type *tape_type, Value *tape_ptr, char change_by) {
    Value *address = get_current_position_address(position_ptr, tape_type, tape_ptr);
    Value *curr = builder->CreateLoad(Type::getInt8Ty(*context), address, "current");
    Value *incremented = builder->CreateAdd(curr,
                                            ConstantInt::get(Type::getInt8Ty(*context), change_by),
                                            "address");

    builder->CreateStore(incremented, address);
}

void emit_print(Value *position_ptr, Type *tape_type, Value *tape_ptr) {
    Value *address = get_current_position_address(position_ptr, tape_type, tape_ptr);
    Value *curr = builder->CreateLoad(Type::getInt8Ty(*context), address, "current");
    Value *curr_i32 = builder->CreateZExt(curr, Type::getInt32Ty(*context), "current_i32");

    FunctionType *print_type = FunctionType::get(Type::getInt32Ty(*context),
                                                 {Type::getInt32Ty(*context)}, true);
    FunctionCallee printchar = module->getOrInsertFunction("putchar", print_type);
    builder->CreateCall(print_type, printchar.getCallee(), {curr_i32}, "putchar()");
}

void emit_get(Value *position_ptr, Type *tape_type, Value *tape_ptr) {
    FunctionType *get_type = FunctionType::get(Type::getInt32Ty(*context), false);

    FunctionCallee getchar = module->getOrInsertFunction("getchar", get_type);
    Value *result = builder->CreateCall(getchar, getchar.getCallee(), {}, "getchar()");
    Value *result_i8 = builder->CreateTrunc(result, Type::getInt8Ty(*context));

    Value *address = get_current_position_address(position_ptr, tape_type, tape_ptr);
    builder->CreateStore(result_i8, address);
}

void emit_loop_start(Value *position_ptr, Type *tape_type, Value *tape_ptr, Function *main) {
    BasicBlock *condition_block = BasicBlock::Create(*context, "while.cond", main);
    builder->CreateBr(condition_block);
    builder->SetInsertPoint(condition_block);

    Value *address = get_current_position_address(position_ptr, tape_type, tape_ptr);
    Value *curr = builder->CreateLoad(Type::getInt8Ty(*context), address, "current");
    Value *condition = builder->CreateICmpEQ(curr,
                                             ConstantInt::get(Type::getInt8Ty(*context), 0),
                                             "while.cmp");

    BasicBlock *body_block = BasicBlock::Create(*context, "while.body", main);
    BasicBlock *outer_block = BasicBlock::Create(*context, "while.out", main);
    builder->CreateCondBr(condition, outer_block, body_block);
    builder->SetInsertPoint(body_block);

    loop_stack.push({
        .cond_block = condition_block,
        .outer_block = outer_block
    });
}

void emit_loop_end() {
    if (loop_stack.empty()) {
        std::cerr << "unmatched loop closing bracket" << std::endl;
        return;
    }

    Loop loop = loop_stack.top();
    loop_stack.pop();
    builder->CreateBr(loop.cond_block);
    builder->SetInsertPoint(loop.outer_block);
}

int main() {
    FunctionType *main_function_type = FunctionType::get(Type::getInt32Ty(*context), false);
    Function *main = Function::Create(main_function_type, GlobalValue::ExternalLinkage, "main",
                                      *module);

    BasicBlock *block = BasicBlock::Create(*context, "entry", main);
    builder->SetInsertPoint(block);

    AllocaInst *position = builder->CreateAlloca(Type::getInt32Ty(*context), 0, "position");
    Value *zero = ConstantInt::get(Type::getInt32Ty(*context), 0);
    builder->CreateStore(zero, position);

    ArrayType *tape_type = ArrayType::get(Type::getInt8Ty(*context), TAPE_LENGTH);
    AllocaInst *tape = builder->CreateAlloca(tape_type, 0, "tape");

    Constant *zero_aggregator = ConstantAggregateZero::get(tape_type);
    builder->CreateStore(zero_aggregator, tape);

    int chr;
    while ((chr = getchar()) != EOF) {
        switch (chr) {
            case '[':
                emit_loop_start(position, tape_type, tape, main);
                break;
            case ']':
                emit_loop_end();
                break;
            case '+':
                emit_value_change(position, tape_type, tape, 1);
                break;
            case '-':
                emit_value_change(position, tape_type, tape, -1);
                break;
            case '>':
                emit_position_change(position, 1);
                break;
            case '<':
                emit_position_change(position, -1);
                break;
            case '.':
                emit_print(position, tape_type, tape);
                break;
            case ',':
                emit_get(position, tape_type, tape);
                break;
            default: break;
        }
    }

    builder->CreateRet(ConstantInt::get(Type::getInt32Ty(*context), 0));

    verifyFunction(*main);
    verifyModule(*module);

    module->print(outs(), nullptr);
    return 0;
}

clang++ -g -O3 main.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o bf_compiler
./bf_compiler < test.bf > output.ll
clang output.ll -o my_program
./my_program
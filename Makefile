CXX = clang++
CC = clang

LLVM_CXXFLAGS = $(shell llvm-config --cxxflags --ldflags --system-libs --libs core)

TARGET = bf_compiler
TEST_PROGRAM_TARGET = test_program

SRCS = $(wildcard src/*.cpp)

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(SRCS) $(LLVM_CXXFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(TEST_PROGRAM_TARGET) *.ll *.o *.out

test: $(TARGET)
	./$(TARGET) < test.bf > output.ll
	$(CC) output.ll -o $(TEST_PROGRAM_TARGET)
	./$(TEST_PROGRAM_TARGET)

.PHONY: all clean test
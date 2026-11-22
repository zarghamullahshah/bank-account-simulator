CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O2

SRC := src/bank.cpp
BIN := bank-demo
TEST_BIN := test-bank

.PHONY: all demo test clean

all: demo

demo: $(BIN)

$(BIN): src/main.cpp $(SRC) src/bank.h
	$(CXX) $(CXXFLAGS) src/main.cpp $(SRC) -o $(BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): tests/test_bank.cpp $(SRC) src/bank.h
	$(CXX) $(CXXFLAGS) tests/test_bank.cpp $(SRC) -o $(TEST_BIN)

clean:
	rm -f $(BIN) $(TEST_BIN)

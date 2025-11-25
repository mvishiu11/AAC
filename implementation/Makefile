# Simple, explicit Makefile for this project
# - Sources: src/*.cpp
# - Headers: include/
# - Object files -> build/
# - Executables -> bin/ (also creates a .exe copy)

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude -g

SRC := src/main.cpp src/Graph.cpp src/Hungarian.cpp
BUILD_DIR := build
BIN_DIR := bin
OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC))
EXEC := $(BIN_DIR)/main.o
EXEC_EXE := $(EXEC).exe

.PHONY: all clean

all: $(EXEC) $(EXEC_EXE)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXEC): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(EXEC_EXE): $(EXEC) | $(BIN_DIR)
	cp $(EXEC) $(EXEC_EXE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)


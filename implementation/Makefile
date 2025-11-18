# Simple, explicit Makefile for this project
# - Sources: src/*.cpp
# - Headers: include/
# - Object files -> build/
# - Executables -> bin/ (also creates a .exe copy)

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude -g

SRC := src/main.cpp src/Graph.cpp
BUILD_DIR := build
BIN_DIR := bin
OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC))
EXEC := $(BIN_DIR)/main.o
EXEC_EXE := $(EXEC).exe
FASTMURTY_DIR := fastmurty
FASTMURTY_LIB := $(FASTMURTY_DIR)/mhtda.so
LDFLAGS += -Wl,-rpath,'$$ORIGIN/../fastmurty' -L$(FASTMURTY_DIR)
LDLIBS += -l:mhtda.so

.PHONY: all clean

all: $(EXEC) $(EXEC_EXE) $(FASTMURTY_LIB)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXEC): $(OBJS) $(FASTMURTY_LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(EXEC_EXE): $(EXEC) | $(BIN_DIR)
	cp $(EXEC) $(EXEC_EXE)

$(FASTMURTY_LIB):
	$(MAKE) -C $(FASTMURTY_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	-$(MAKE) -C $(FASTMURTY_DIR) clean


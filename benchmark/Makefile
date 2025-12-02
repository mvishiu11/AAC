CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall

SRC_DIR := src
BIN_DIR := bin

all: $(BIN_DIR)/generator $(BIN_DIR)/runner

$(BIN_DIR)/generator: $(SRC_DIR)/Generator.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN_DIR)/runner: $(SRC_DIR)/Runner.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Target to run the benchmark
run: all
	uv run src/ExperimentRunner.py $(ARGS)

# Target to run the new python experiment suite
run-experiments:
	uv run src/ExperimentRunner.py

# Target to visualize results (requires uv)
plot:
	uv run scripts/plot_results.py

clean:
	rm -rf $(BIN_DIR) *.csv *.png temp_bench_data.txt

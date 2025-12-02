# Empirical Testing Harness

This folder contains a C++ benchmarking suite for the Multigraph implementation. It generates synthetic graph data, executes the main binary, and collects performance metrics.

## Structure

* **`src/Generator.cpp`**: C++ tool to generate random multigraph adjacency matrices.
* **`src/Runner.cpp`**: C++ harness for basic performance benchmarking.
* **`src/ExperimentRunner.py`**: Advanced Python-based experiment suite that orchestrates complex test scenarios (Accuracy, Scalability, Density) and generates visualizations.
* **`analysis.ipynb`**: Jupyter notebook for interactive analysis and reporting of experiment results.
* **`scripts/plot_results.py`**: Legacy script for visualizing basic benchmark CSV data.
* **`experiments/`**: Directory where experiment results (CSVs, logs, visualizations) are stored, organized by timestamp.

## Prerequisites

1. **Build the Implementation**:
   The benchmarking tools rely on the compiled binary from the implementation folder.

   ```bash
   cd ../implementation
   make
   ```
2. **Python Environment**:
   Dependencies are managed via `uv` (or standard pip).

   ```bash
   # Install uv (optional but recommended)
   curl -LsSf https://astral.sh/uv/install.sh | sh

   # With uv simply sync to get deps
   uv sync

   # Or install dependencies manually
   pip install pandas matplotlib seaborn networkx jupyter
   ```

## Usage

### 1. Basic Performance Benchmark (C++)

Runs a quick performance check using the C++ runner.

```bash
make run
```

* **Output**: `benchmark_results.csv`
* **Visualization**: `make plot` (generates `results_exact.png`, `results_approx.png`)

### 2. Advanced Scientific Experiments (Python)

Runs a comprehensive suite of experiments to evaluate accuracy, scalability, and robustness.

```bash
make run-experiments
```

* **Output**: A new folder in `experiments/exp_<TIMESTAMP>/` containing:
  * `results.csv`: Raw data.
  * `visualizations/`: PNG images of the generated graphs (G and H) for qualitative analysis.

#### Experiment Types:

* **Accuracy Analysis**:
  * Generates small random multigraphs ($|V| \in [3, 8]$).
  * Runs both **Exact** (Branch & Bound) and **Approx** (Heuristic) algorithms.
  * Compares the "Minimal Extension Cost" (number of added edges) to calculate the approximation ratio.
* **Scalability Analysis**:
  * Tests the **Approx** algorithm on larger graphs (up to $|V|=50$).
  * Measures runtime growth with respect to graph size.
* **Density Analysis**:
  * Fixes graph size and varies edge density ($0.1$ to $0.9$).
  * Analyzes how graph sparsity affects the heuristic's performance.

### 3. Interactive Analysis

Open `analysis.ipynb` in VS Code or Jupyter Lab.

* Automatically loads the latest experiment data.
* Provides detailed plots for Approximation Ratio, Runtime Scalability, and Density Impact.
* Displays generated graph visualizations inline.

## Methodology

### Basic Benchmark (C++)

The runner executes the binary against randomly generated graphs with fixed density (0.5) and varying vertex counts.

### Advanced Experiments (Python)

* **Accuracy:** Generates small random multigraphs ($|V| \in [3, 8]$). Runs both Exact and Approx algorithms. Calculates the ratio of costs.
* **Scalability:** Runs Approx on graphs up to $|V|=50$.
* **Visualization:** Generates PNG images of the input graphs for qualitative assessment.
* **Approx:** Tested on sizes 5 to 50.
* **Metric:** Wall-clock execution time averaged over multiple iterations.

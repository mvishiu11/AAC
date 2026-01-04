# Multigraph Subgraph Matching & Minimal Extension Implementation

## CLI

Makefile build output: `implementation/bin/main.o`

### Subgraph isomorphism / minimal extension

```bash
./bin/main.o exact <file> <N> [v]
./bin/main.o approx <file> <N> [v]
```

- `N` = number of subgraph isomorphisms requested
- `v` = verbose

### Graph edit distance (GED)

```bash
./bin/main.o ged exact <file> [v] [p]
./bin/main.o ged approx <file> <K> [v] [p]

# Backward compatible (== approx)
./bin/main.o ged <file> <K> [v] [p]
```

- `K` = number of candidate mappings used by the approximation
- `v` = verbose
- `p` = print aggregated edit path

## Input file format

Two graphs, each as a vertex count followed by an adjacency matrix:

```
V(G)
[Adjacency matrix V(G)xV(G)]
V(H)
[Adjacency matrix V(H)xV(H)]
```

Matrix convention: column = source, row = destination (i.e. multiplicity is `A[dst][src]`).

## Build

Linux/macOS:

```bash
make -C implementation clean
make -C implementation
```

Windows (CMake):

```powershell
cmake --build implementation/build
```

## Convenience

Run all GED examples with timing:

```bash
cd implementation
zsh scripts/run_ged_examples.sh
K=10 zsh scripts/run_ged_examples.sh
FLAGS="p" zsh scripts/run_ged_examples.sh
```

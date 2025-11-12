#include "../include/Graph.hpp"

int Graph::getSize() {
    int size = 0;
    for(int i = 0; i < nodes; i++) {
        for(int j = 0; j < nodes; j++) {
            if(edges[i][j] != 0) {
                size += edges[i][j];
            }
        }
    }
    return size+nodes;
}

bool Graph::hasNSubgraphs(Graph& G, int N) {
    if (G.getVerticesCount() == 0 || G.getVerticesCount() > this->getVerticesCount()) {
        return true;
    }

    int Hn = this->getVerticesCount();
    int Gn = G.getVerticesCount();

    int** M = (int**)malloc(sizeof(int) * Hn * Gn);

    for(int i = 0; i < Hn; i++) {
        for(int j = 0; j < Gn; j++) {
            if(this->getOutDegree(i) >= G.getOutDegree(j) && this->getInDegree(i) >= G.getInDegree(j)) {
                M[i][j] = 1;
            }
            else {
                M[i][j] = 0;
            }
        }
    }

    // Iterative neighbourhood refinement (prune until fixpoint)
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < Hn; ++i) {
            for (int j = 0; j < Gn; ++j) {
                if (M[i][j] == 0) continue;

                // Check outgoing neighbours of pattern-vertex j
                bool ok_out = true;
                for (int u = 0; u < Gn; ++u) {
                    int mult_pattern = G.getOutDegree(j, u); // multiplicity of (j->u) in G
                    if (mult_pattern <= 0) continue;

                    bool found = false;
                    for (int v = 0; v < Hn; ++v) {
                        int mult_host = this->getOutDegree(i, v); // multiplicity of (i->v) in H
                        if (mult_host >= mult_pattern && M[v][u] == 1) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) { ok_out = false; break; }
                }

                if (!ok_out) {
                    M[i][j] = 0;
                    changed = true;
                    continue;
                }

                // Check incoming neighbours of pattern-vertex j
                bool ok_in = true;
                for (int u = 0; u < Gn; ++u) {
                    int mult_pattern = G.getOutDegree(u, j); // multiplicity of (u->j) in G
                    if (mult_pattern <= 0) continue;

                    bool found = false;
                    for (int v = 0; v < Hn; ++v) {
                        int mult_host = this->getOutDegree(v, i); // multiplicity of (v->i) in H
                        if (mult_host >= mult_pattern && M[v][u] == 1) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) { ok_in = false; break; }
                }

                if (!ok_in) {
                    M[i][j] = 0;
                    changed = true;
                }
            }
        }
    }

    // If any pattern-vertex j has no candidates left, no isomorphism possible
    for (int j = 0; j < Gn; ++j) {
        int col_sum = 0;
        for (int i = 0; i < Hn; ++i) col_sum += M[i][j];
        if (col_sum == 0) return false;
    }
}
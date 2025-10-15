from multigraph.graph import MultiGraph
from multigraph.extension import minimal_extension


def injective_edge_edit_distance(G: MultiGraph, H: MultiGraph) -> int:
    """
    A simple, asymmetric, injective edge-edit distance:
    - Minimum number of edge insertions required in H so that G is a subgraph of H'.
    (No deletions, no relabels, vertices fixed to |V(G)|<=|V(H)|.)

    This equals the minimal-extension cost from H's perspective.
    """
    if G.n == 0:
        return 0
    if G.n > H.n:
        # If we strictly forbid adding vertices, this is infeasible.
        # Return a large number to indicate "far".
        return 10**9
    cost, _, _ = minimal_extension(G, H)
    return cost

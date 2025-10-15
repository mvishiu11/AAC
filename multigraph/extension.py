from typing import Dict, List, Optional, Tuple

from multigraph.graph import MultiGraph


def _initial_candidates(G: MultiGraph, H: MultiGraph) -> List[List[bool]]:
    M = [[False] * H.n for _ in range(G.n)]
    G_deg = [G.degree(i) for i in range(G.n)]
    H_deg = [H.degree(j) for j in range(H.n)]
    for i in range(G.n):
        for j in range(H.n):
            # allow looser filter here; degree(i) <= degree(j)+slack could help,
            # but keep simple: deg(i) <= deg(j) OR allow mapping anyway for extension.
            M[i][j] = G_deg[i] <= max(H_deg[j], G_deg[i])  # always True; placeholder for symmetry
            M[i][j] = True
    return M


def minimal_extension(
    G: MultiGraph, H: MultiGraph
) -> Tuple[int, Dict[int, int], List[Tuple[int, int, int]]]:
    """
    Find the minimal number of additional edges (with multiplicity) to add to H
    so that G is a subgraph of H' under some injective mapping phi.

    Returns:
      (cost, mapping, missing_edges)
        cost: minimal number of parallel edges to add
        mapping: dict i_in_G -> j_in_H
        missing_edges: list of (j1, j2, k) edges to add in H (indices in H),
                       where k is how many parallel edges to add between j1 and j2
    """
    assert G.n <= H.n, "Assumption: |V(G)| <= |V(H)|"

    if G.n == 0:
        return 0, {}, []

    M = _initial_candidates(G, H)
    used_H = [False] * H.n
    order = sorted(range(G.n), key=lambda i: -G.degree(i))

    best_cost = float("inf")
    best_map: Optional[List[int]] = None

    def incremental_deficit(i: int, j: int, mapping: List[Optional[int]], t: int) -> int:
        """
        Extra edges (with multiplicity) needed when we map v_i -> u_j,
        considering already placed vertices in positions < t of 'order'.
        """
        add = 0
        for s in range(t):
            a = order[s]
            ma = mapping[a]
            if ma is None:
                continue
            # both directions (i,a) and (a,i) same in undirected; count once (a<i in order)
            req = G.multiplicity(i, a)
            if req > 0:
                have = H.multiplicity(j, ma)
                if have < req:
                    add += (req - have)
        return add

    mapping: List[Optional[int]] = [None] * G.n

    def backtrack(t: int, cost_so_far: int):
        nonlocal best_cost, best_map
        if cost_so_far >= best_cost:
            return
        if t == len(order):
            best_cost = cost_so_far
            best_map = [mapping[i] for i in range(G.n)]  # type: ignore
            return

        i = order[t]
        for j in range(H.n):
            if used_H[j] or not M[i][j]:
                continue
            # optimistic lower bound: zero; (you can add a stronger LB if needed)
            add = incremental_deficit(i, j, mapping, t)
            new_cost = cost_so_far + add
            if new_cost >= best_cost:
                continue

            mapping[i] = j
            used_H[j] = True
            backtrack(t + 1, new_cost)
            used_H[j] = False
            mapping[i] = None

    backtrack(0, 0)

    if best_map is None:
        # Should not happen since we allow any injective mapping; fallback
        return -1, {}, []

    # Build missing edges list for chosen mapping
    missing: Dict[Tuple[int, int], int] = {}
    for a in range(G.n):
        for b in range(a, G.n):  # a<=b to avoid double-count
            req = G.multiplicity(a, b)
            if req == 0:
                continue
            ja, jb = best_map[a], best_map[b]
            have = H.multiplicity(ja, jb)
            if have < req:
                missing[(min(ja, jb), max(ja, jb))] = missing.get(
                    (min(ja, jb), max(ja, jb)), 0
                ) + (req - have)

    missing_edges = [(i, j, k) for (i, j), k in missing.items()]
    mapping_dict = {i: best_map[i] for i in range(G.n)}
    return int(best_cost), mapping_dict, missing_edges

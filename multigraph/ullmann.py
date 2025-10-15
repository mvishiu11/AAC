from typing import Dict, Generator, List, Optional

from multigraph.graph import MultiGraph


def _initial_candidate_matrix(G: MultiGraph, H: MultiGraph) -> List[List[bool]]:
    """Degree-based filtering: v in G can map to u in H only if deg(v) <= deg(u)."""
    M = [[False] * H.n for _ in range(G.n)]
    G_deg = [G.degree(i) for i in range(G.n)]
    H_deg = [H.degree(j) for j in range(H.n)]
    for i in range(G.n):
        for j in range(H.n):
            M[i][j] = G_deg[i] <= H_deg[j]
    return M


def _refine(G: MultiGraph, H: MultiGraph, M: List[List[bool]]) -> bool:
    """
    Simple refinement:
    If v_i can map to u_j, then for every neighbor v_k of v_i,
    there must exist at least one u_l neighbor of u_j with multiplicity >= required.
    This is a light version of Ullmann's pruning for multigraphs.
    Returns True if no contradiction found; False if a row becomes all False.
    """
    changed = True
    while changed:
        changed = False
        for i in range(G.n):
            # For each candidate j for i, check feasibility vs neighbors
            row_any = False
            for j in range(H.n):
                if not M[i][j]:
                    continue
                feasible = True
                # check neighbors of i
                for k in range(G.n):
                    req = G.multiplicity(i, k)
                    if req == 0:
                        continue
                    # there must be some l where M[k][l] and m_H(j,l) >= req
                    found = False
                    for l in range(H.n):
                        if not M[k][l]:
                            continue
                        if H.multiplicity(j, l) >= req:
                            found = True
                            break
                    if not found:
                        feasible = False
                        break
                if not feasible:
                    M[i][j] = False
                    changed = True
                else:
                    row_any = True
            if not row_any:
                return False
    return True


def is_subgraph(G: MultiGraph, H: MultiGraph) -> bool:
    """Return True if G is a subgraph of H (multiplicity-aware)."""
    if G.n == 0:
        return True
    if G.n > H.n:
        return False

    M = _initial_candidate_matrix(G, H)
    if not _refine(G, H, M):
        return False

    used_H = [False] * H.n
    order = sorted(range(G.n), key=lambda i: -G.degree(i))

    def backtrack(t: int, mapping: List[Optional[int]]) -> bool:
        if t == len(order):
            # verify multiplicities
            for a in range(G.n):
                for b in range(G.n):
                    if G.multiplicity(a, b) > H.multiplicity(mapping[a], mapping[b]):
                        return False
            return True

        i = order[t]
        for j in range(H.n):
            if not M[i][j] or used_H[j]:
                continue
            # partial feasibility check vs already mapped neighbors
            ok = True
            for s in range(t):
                a = order[s]
                if mapping[a] is None:
                    continue
                req = G.multiplicity(i, a)
                if req > 0:
                    if H.multiplicity(j, mapping[a]) < req:
                        ok = False
                        break
            if not ok:
                continue
            mapping[i] = j
            used_H[j] = True
            if backtrack(t + 1, mapping):
                return True
            used_H[j] = False
            mapping[i] = None
        return False

    mapping: List[Optional[int]] = [None] * G.n
    return backtrack(0, mapping)


def enumerate_subgraphs(G: MultiGraph, H: MultiGraph) -> Generator[Dict[int, int], None, None]:
    """
    Yield all injective mappings phi: V(G)->V(H) that satisfy multiplicities.
    Mapping is returned as {i_in_G: j_in_H} over vertex indices.
    """
    if G.n == 0 or G.n > H.n:
        return

    M = _initial_candidate_matrix(G, H)
    if not _refine(G, H, M):
        return

    used_H = [False] * H.n
    order = sorted(range(G.n), key=lambda i: -G.degree(i))
    mapping: List[Optional[int]] = [None] * G.n

    def backtrack(t: int):
        if t == len(order):
            # verify full feasibility
            for a in range(G.n):
                for b in range(G.n):
                    if G.multiplicity(a, b) > H.multiplicity(mapping[a], mapping[b]):
                        return
            yield {i: mapping[i] for i in range(G.n)}
            return

        i = order[t]
        for j in range(H.n):
            if not M[i][j] or used_H[j]:
                continue
            # partial multiplicity check
            ok = True
            for s in range(t):
                a = order[s]
                if mapping[a] is None:
                    continue
                req = G.multiplicity(i, a)
                if req > 0 and H.multiplicity(j, mapping[a]) < req:
                    ok = False
                    break
            if not ok:
                continue

            mapping[i] = j
            used_H[j] = True
            yield from backtrack(t + 1)
            used_H[j] = False
            mapping[i] = None

    yield from backtrack(0)

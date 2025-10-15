import time
from itertools import permutations

from multigraph import MultiGraph, minimal_extension, is_subgraph


def build_harder_graphs():
    # G: 5 vertices with varied multiplicities and a self-loop
    # Vertices: a,b,c,d,e (order preserved internally)
    G = MultiGraph.from_edges([
        ("a", "b", 2),  # doubled
        ("b", "c", 1),
        ("c", "d", 2),  # doubled
        ("d", "e", 1),
        ("a", "e", 1),
        ("b", "d", 1),
        ("c", "c", 1),  # self-loop at c
    ])

    # H: 6 vertices, a path with a couple chords (not enough multiplicity)
    # Vertices: u0..u5
    H = MultiGraph.from_edges([
        ("u0", "u1", 1),
        ("u1", "u2", 1),
        ("u2", "u3", 1),
        ("u3", "u4", 1),
        ("u4", "u5", 1),
        ("u0", "u2", 1),  # chord
        ("u2", "u4", 1),  # chord
        ("u1", "u3", 2),  # one doubled edge somewhere in H
        # no self-loops in H
    ])
    return G, H


def brute_force_min_extension(G: MultiGraph, H: MultiGraph):
    """
    Ground-truth oracle for small sizes:
    enumerate all injective mappings phi: V(G)->V(H),
    return the minimal deficit and the best mapping.
    """
    assert G.n <= H.n
    best = float("inf")
    best_map = None

    idx_G = list(range(G.n))
    for chosen in permutations(range(H.n), G.n):  # 6P5 = 720 mappings here
        phi = {i: chosen[i] for i in idx_G}
        # compute deficit (missing multiplicity) for this mapping
        add = 0
        for i in range(G.n):
            for j in range(i, G.n):
                req = G.multiplicity(i, j)
                if req == 0:
                    continue
                have = H.multiplicity(phi[i], phi[j])
                if have < req:
                    add += (req - have)
        if add < best:
            best = add
            best_map = phi
            if best == 0:
                break
    return int(best), best_map


def main():
    G, H = build_harder_graphs()

    print("G:", G, "size:", G.size())
    print("H:", H, "size:", H.size())
    print("Is G subgraph of H?", is_subgraph(G, H))

    t0 = time.perf_counter()
    cost, mapping, missing = minimal_extension(G, H)
    t1 = time.perf_counter()

    print("\n--- Algorithm (branch-and-bound) ---")
    print("Minimal extension cost:", cost)
    print("Mapping (G->H indices):", mapping)
    print("Missing edges to add to H (i, j, k):", missing)
    print(f"Time: {(t1 - t0)*1000:.2f} ms")

    # Oracle check
    t2 = time.perf_counter()
    oracle_cost, oracle_map = brute_force_min_extension(G, H)
    t3 = time.perf_counter()

    print("\n--- Oracle (brute-force over injective mappings) ---")
    print("Oracle minimal cost:", oracle_cost)
    print("Oracle example mapping (indices):", oracle_map)
    print(f"Time: {(t3 - t2)*1000:.2f} ms")

    # Consistency
    ok = (cost == oracle_cost)
    print("\nConsistency (algorithm == oracle)?", ok)


if __name__ == "__main__":
    main()

from multigraph import MultiGraph, is_subgraph, minimal_extension, injective_edge_edit_distance


def main():
    # G: triangle with one doubled edge
    G = MultiGraph.from_edges([
        ("a", "b", 2),
        ("b", "c", 1),
        ("a", "c", 1),
    ])

    # H: path on 3 vertices (no doubled edges)
    H = MultiGraph.from_edges([
        ("u", "v", 1),
        ("v", "w", 1),
    ])

    print("G:", G, "size:", G.size())
    print("H:", H, "size:", H.size())

    print("Is G subgraph of H?", is_subgraph(G, H))

    cost, mapping, missing = minimal_extension(G, H)
    print("Minimal extension cost:", cost)
    print("Mapping (G->H indices):", mapping)
    print("Missing edges to add to H (i, j, k):", missing)

    print("Injective edge-edit distance (G vs H):", injective_edge_edit_distance(G, H))


if __name__ == "__main__":
    main()

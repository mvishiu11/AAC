from collections import defaultdict
from typing import Dict, Iterable, List, Tuple


class MultiGraph:
    """
    Undirected multigraph with non-negative integer edge multiplicities.

    Internal representation:
      - vertices: list[str] in insertion order
      - index map: name -> idx
      - adj: Dict[int, Dict[int, int]] holding multiplicity (symmetric)
    """

    def __init__(self, vertices: Iterable[str] = ()):
        self._verts: List[str] = []
        self._idx: Dict[str, int] = {}
        self._adj: Dict[int, Dict[int, int]] = defaultdict(lambda: defaultdict(int))
        for v in vertices:
            self.add_vertex(v)

    # ------- vertices -------
    @property
    def n(self) -> int:
        return len(self._verts)

    def vertices(self) -> List[str]:
        return list(self._verts)

    def add_vertex(self, v: str) -> None:
        if v in self._idx:
            return
        self._idx[v] = len(self._verts)
        self._verts.append(v)

    def index_of(self, v: str) -> int:
        return self._idx[v]

    # ------- edges / multiplicities -------
    def add_edge(self, u: str, v: str, k: int = 1) -> None:
        """Add k parallel edges between u and v (k>=1)."""
        if k <= 0:
            return
        if u not in self._idx:
            self.add_vertex(u)
        if v not in self._idx:
            self.add_vertex(v)
        i, j = self._idx[u], self._idx[v]
        if i == j:
            # Allow self-loops; count on diagonal
            self._adj[i][i] += k
        else:
            self._adj[i][j] += k
            self._adj[j][i] += k

    def multiplicity(self, i: int, j: int) -> int:
        return self._adj[i].get(j, 0)

    def degree(self, i: int) -> int:
        # Sum of multiplicities of incident edges
        # (self-loop counts twice in undirected-by-convention; here we count it once)
        return sum(self._adj[i].values())

    def edge_count(self) -> int:
        # Each undirected edge counted twice off-diagonal; self-loops once
        total = 0
        for i in range(self.n):
            for j, k in self._adj[i].items():
                if i == j:
                    total += k
                else:
                    total += k
        # off-diagonal are doubled
        offdiag = 0
        for i in range(self.n):
            for j, k in self._adj[i].items():
                if i < j:
                    offdiag += k
        # total edges = selfloops + offdiag (counted once)
        selfloops = sum(self._adj[i].get(i, 0) for i in range(self.n))
        return selfloops + offdiag

    def size(self) -> int:
        return self.n + self.edge_count()

    # ------- convenience -------
    @classmethod
    def from_edges(cls, edges: Iterable[Tuple[str, str, int]]):
        """
        edges: iterable of (u, v, multiplicity>=1)
        """
        g = cls()
        for u, v, k in edges:
            g.add_edge(u, v, k)
        return g

    def as_adjacency_matrix(self) -> List[List[int]]:
        A = [[0] * self.n for _ in range(self.n)]
        for i in range(self.n):
            for j, k in self._adj[i].items():
                A[i][j] = k
        return A

    def __repr__(self) -> str:
        return f"MultiGraph(n={self.n}, m={self.edge_count()})"

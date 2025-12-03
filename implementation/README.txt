Calling:

./main.exe <exact/approx> <path-to-file> <number-of-copies>

Example:
./main.exe exact examples\ex1.txt 2 

The file should be in form of:

V(G)
[Adjancency matrix V(G)xV(G)]
V(H)
[Adjancency matrix V(H)xV(H)]

Example:

3
4 2 5
3 4 1
2 3 4
5
4 2 5 3 1
3 4 1 2 4
2 3 4 5 0
1 0 3 4 2
5 1 0 2 3


Where column index represents source of edge, and row index represents destination of edge.
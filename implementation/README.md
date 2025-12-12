# Multigraph Subgraph Matching & Minimal Extension Implementation

Calling form:

```bash
./main.o <exact/approx> <path-to-file> <N>
```

The file should be in form of:

```
V(G)
[Adjancency matrix V(G)xV(G)]
V(H)
[Adjancency matrix V(H)xV(H)]
```

Example:

```
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
```

Where column index represents source of edge, and row index represents destination of edge.

To compile, run:

PS C:\Users\<user>\Documents\GitHub\AAC> cmake --build implementation/build
Wersja programu MSBuild 18.0.5+e22287bf1 dla .NET Framework

  Hungarian.cpp
  main.vcxproj -> C:\Users\<user>\Documents\GitHub\AAC\implementation\build\Debug\main.exe
  Building Custom Rule C:/Users/remek/Documents/GitHub/AAC/implementation/CMakeLists.txt
PS C:\Users\<user>\Documents\GitHub\AAC> 

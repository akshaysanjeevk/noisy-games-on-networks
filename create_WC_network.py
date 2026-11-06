#!/usr/bin/env python3
import sys
import numpy as np

def build_lattice(L):
    N, K = L * L, 8
    forward = [(0, 1), (1, -1), (1, 0), (1, 1)]
    edges = []
    for dr, dc in forward:
        for r in range(L):
            for c in range(L):
                edges.append((r * L + c, ((r + dr) % L) * L + (c + dc) % L))
    return N, K, edges

def main():
    if len(sys.argv) < 3:
        sys.exit("Usage: python create_WS_network.py <L> <p> [seed]")
    
    L = int(sys.argv[1])
    p = float(sys.argv[2])
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else None

    if seed is not None:
        np.random.seed(seed)

    N, K, edges = build_lattice(L)
    
    # Build adjacency list (sets)
    adj = [set() for _ in range(N)]
    for s, t in edges:
        adj[s].add(t)
        adj[t].add(s)

    # Watts-Strogatz rewiring
    for source, target in edges:
        if np.random.random() < p:
            newtgt = target
            while newtgt == source or newtgt in adj[source]:
                newtgt = int(np.random.randint(0, N))
            
            adj[source].discard(target)
            adj[target].discard(source)
            adj[source].add(newtgt)
            adj[newtgt].add(source)

    # Write neighbour list file
    sname = f"neighbour_list_N_{N}_p_{p:g}.dat"
    
    with open(sname, "w") as f:
        for node in range(N):
            f.write(" ".join(map(str, sorted(adj[node]))) + "\n")
            
    print(f"Built and saved {sname} (N = {N}, p = {p})")

if __name__ == "__main__":
    main()
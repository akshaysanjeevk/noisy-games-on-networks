import sys
import numpy as np

# generate the edge list of a regular square lattice of
# dimension LxL with a Moore neighbourhood (degree 8)
def build_lattice(L):

    # only consider 4 forward-facing directions to avoid
    # double-counting, specifically E, SW, S, SE
    forward = [(0, 1), (1, -1), (1, 0), (1, 1)]
    edges = []
    for dr, dc in forward:
        for r in range(L):
            for c in range(L):
                src = r * L + c # convert (r,c) to 1D index
                # account for periodic boundary conditions
                # when finding the target index
                tgt = ((r + dr) % L) * L + (c + dc) % L
                edges.append((src, tgt))
    return edges

def main():
    if len(sys.argv) < 3:
        sys.exit("Usage: python create_WS_network.py <L> <p> [seed]")
    
    L = int(sys.argv[1])
    p = float(sys.argv[2])
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else None

    if seed is not None:
        np.random.seed(seed)

    N = L * L
    edges = build_lattice(L)
    
    # Convert edge list to a set (to speed up lookup and
    # prevent duplicates when later rewired)
    adj = [set() for _ in range(N)]

    # The edge list has only one undirected link per pair,
    # so we ensure that for each tuple (s,t) each node records
    # the other as a neighbour
    for s, t in edges:
        adj[s].add(t)
        adj[t].add(s)

    # Watts-Strogatz rewiring: cycle through every edge and rewire
    # if a number randomly drawn for that edge is less than p
    for source, target in edges:
        if np.random.random() < p:
            # change the target to a randomly selected index that
            # is not the source or a neighbour of the source
            newtgt = target
            while newtgt == source or newtgt in adj[source]:
                newtgt = int(np.random.randint(0, N))

            # update the connections in the set
            adj[source].discard(target)
            adj[target].discard(source)
            adj[source].add(newtgt)
            adj[newtgt].add(source)

    # Write neighbour adjacency list file
    sname = f"neighbour_list_N_{N}_p_{p:g}.dat"
    with open(sname, "w") as f:
        for node in range(N):
            # sort neighbour list and convert to string
            f.write(" ".join(map(str, sorted(adj[node]))) + "\n")
            
    print(f"Adjacency list for network with N = {N}, p = {p} saved: {sname}")

if __name__ == "__main__":
    main()
# Noisy games on networks

A set of codes for investigating the effect of noise on the collective dynamics of a system of agents interacting via two-player games over a specified network, in line with the approach described in the manuscript:

> Menon SN, Sasidevan V, & Sinha S (2018) Emergence of cooperation as a non-equilibrium transition in noisy spatial games. _Front. Phys._ **6**: 34.
> <https://doi.org/10.3389/fphy.2018.00034>

Briefly, at every iteration, each agent simultaneously plays independent games with each of their neighbours on the network, accumulates payoffs and updates their actions via a Fermi rule. Here each agent ```i``` (with net payoff ```π_i```) chooses, at random, a neighbour ```j``` (with net payoff ```π_j```) and copies the action of the neighbour with a probability given by the Fermi distribution function:

```Π_{i->j} = 1/(1+exp(-(π_j-π_i)/K))```

The payoffs that an agent recieves is derived from the payoff matrix:

|           | Cooperate | Defect |
|    ---    |    ---    |   ---  |
| Cooperate |    R, R   |  S, T  |
|  Defect   |    T, S   |  P, P  |

where the values ```R=1```, ```P=0```, ```S=0``` are hardcoded, while ```T``` is treated as a free parameter. Note that ```T>1``` corresponds to the Prisoner's Dilemma game, while ```T<1``` corresponds to the Stag Hunt game.

These codes use the xoshiro256++ random number generator (public domain / CC0) by David Blackman and Sebastiano Vigna (<https://prng.di.unimi.it>).
_____________________________________________________________________________________________

## Simulation details

(1) **Create a network adjacency list**

The C script requires an index-based adjacency list in DAT format, where the elements in row ```i``` correspond to the indices of the network neighbours of node ```i```. This code supports networks with variable node degrees.

In order to create the sorts of networks used in Menon et al (2018), use:

```bash
python create_WS_network.py <N> <p>
```

where ```<N>``` is the system size and ```<p>``` is the probability that each node on the regular lattice is rewired (via the Watts-Strogatz technique). This creates a file ```neighbour_list_N_<N>_p_<p>.dat```.

(2) **Compile and run the script**

```bash
gcc -O3 -lm -o script network_game_fermi.c
./script <network_file>.dat <T> <K> <tmax> <seed> [OPTIONAL: <rec_int>]
```

where we supply the following input arguments:

| argument | Description |
| --- | --- |
| ```network_file``` | file with network adjacency list |
| ```T``` | temptation of an agent to defect |
| ```K``` | temperature in the Fermi rule |
| ```tmax``` | maximum number of iterations |
| ```seed``` | initializes the random number generator |
| ```rec_int``` | interval between recorded time series values (**optional**) |

The script generates a file ```data_<network_file>/nc_T_<T>_K_<K>_seed<seed>.dat``` containing the value of the final number of cooperators (```nc```) in the network.

If ```rec_int(>0)``` is specified in the argument list then the script creates an additional file ```data_<network_file>/TS_T_<T>_K_<K>_seed<seed>.dat``` which contains time-series data for ```nc```.

_____________________________________________________________________________________________

## Contents

The repository contains the following files:

* ```network_game_fermi.c``` : main file (start here)

* ```cli_utils.h``` : contains command-line argument processing utilities

* ```network_loader.h``` : contains function ```scan_network()``` that parses, validates, and loads network file.

* ```rng_xoshiro.c``` : contains wrapper for third party random number generator (xoshiro256++, see: <https://prng.di.unimi.it>)

* ```create_WS_network.py``` : script for creating an index-based adjacency list for a Watts-Strogatz rewired network of specified size ```N``` and rewiring probability ```p```, starting with a regular square lattice, assuming a Moore neighbourhood (8 neighbours).

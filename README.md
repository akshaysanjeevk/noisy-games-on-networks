# Noisy games on networks

A set of C codes for investigating the effect of noise on the collective dynamics of a system of agents interacting via two-player games on a specified network, in line with the specific approach described in the manuscript:

> Menon SN, Sasidevan V, & Sinha S (2018) Emergence of cooperation as a non-equilibrium transition in noisy spatial games. _Front. Phys._ **6**: 34.
> <https://doi.org/10.3389/fphy.2018.00034>

Briefly, at every iteration every agent simultaneously plays a game with each of their neighbours on the network, accumulates payoffs and updates their actions via a Fermi rule. Here each agent ```i``` (with net payoff ```π_i```) chooses, at random, a neighbour ```j``` (with net payoff ```π_j```) and copies the action of the neighbour with a probability given by the Fermi distribution function:

```Π_{i->j} = 1/(1+exp(-(π_j-π_i)/K))```

This code uses the xoshiro256++ random number generator (public domain / CC0) by David Blackman and Sebastiano Vigna.
_____________________________________________________________________________________________

The code can be run as follows:

(1) **Create a network adjacency list.** For example to create the sorts of networks used in Menon et al (2018), use:

```bash
python create_WS_network.py <N> <p>
```

where ```<N>``` is the system size and ```<p>``` is the probability that each node on the regular lattice is rewired (via the Watts-Strogatz technique).

(2) **Compile and run the script.**

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

(Note that ```T>1``` corresponds to the Prisoner's Dilemma game, while ```T<1``` corresponds to the Stag Hunt game.)

If the input network provided is named ```<network_file>.dat``` the script generates a folder ```data_<network_file>``` that contains a file ```nc_T_<T>_K_<K>_seed<seed>.dat``` with the final number of cooperators (```nc```) in the network. If ```rec_int(>0)``` is specified in the argument list then the new folder will contain an additional file ```TS_T_<T>_K_<K>_seed<seed>.dat``` which contains time-series data for ```nc```.

_____________________________________________________________________________________________
The repository contains the following files:

* ```network_game_fermi.c``` : main file (start here)

* ```cli_utils.h``` : contains command-line argument processing utilities

* ```network_loader.h``` : contains function ```scan_network()``` that parses, validates, and loads network file

* ```rng_xoshiro.c``` : contains wrapper for third party random number generator (xoshiro256++, see: <https://prng.di.unimi.it>)

* ```create_WS_network.py``` : script for creating an index-based adjacency list for a Watts-Strogatz rewired network of specified size ```N``` and rewiring probability ```p```, starting with a regular square lattice, assuming a Moore neighbourhood (8 neighbours).

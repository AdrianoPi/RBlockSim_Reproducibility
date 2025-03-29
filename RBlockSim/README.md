# RBlockSim
A blockchain simulation model for ROOT-Sim simulator

## Fast start
Generate a network topology:
```bash
python3 generate_topology.py --n_nodes 1000 --min_peers 2 --max_peers 6 --seed 0
```
Generates a network topology with 1000 nodes, each with a number of peers between 2 and 6, and uses RNG seed 0.

Compile the project:
```bash
make rblocksim
```

Run the simulation:
```bash
./rblocksim -w 2 -i 13 -a 51 -h 0.7 -r 1234 -o output
```
Runs the simulation with 2 worker threads (`-w 2`) (default uses all available cores) and an average block time of 13 seconds (`-i 13`),
simulating a 51% attack (`-a 51`) with an attacker having 70% of the network's hashrate (`-h 0.7`). It also sets the rng seed to 1234 (`-r 1234`),
and sets the file name for output stats to "output" (`-o output`)

The simulation statistics results will be saved in the `results_N` directory.

## Command line options
- `a` - attack type in {51, selfish}
- `c` - (only during attacks) maximum depth the node's main chain can lag behind before switching chains to one on which it has mined fewer blocks
- `d` - (selfish mining only) number of blocks the attacker mines in secret before publishing them
- `h` - percentage of the network's hashrate controlled by the attacker (default for 51% attack: 0.51. Default for selfish mining: 0.34)
- `i` - average block time in seconds
- `o` - node statistics output file name
- `r` - rng seed
- `s` - (selfish mining only) start time of the attack in seconds
- `w` - number of worker threads

## Corner case examples
Inputs that generate corner cases forcing a change in the simulation base parameters

### Fork too long
Requires a DEPTH_TO_KEEP of 200 to work.
```
python3 generate_topology.py --n_nodes 1000 --min_peers 2 --max_peers 6 --seed 0
make rblocksim
./rblocksim -w 6 -i 13 -a 51 -h 0.51 -c 10 -o out -r 12
```


# PADS 2025 Reproducibility artifact

This document describes how to reproduce the results discussed in the paper:

"RBlockSim: Parallel and Distributed Simulation for Blockchain Benchmarking"

Accepted to ACM SIGSIM PADS 2025

The public repository can be found at: https://github.com/AdrianoPi/RBlockSim_Reproducibility

## Authors & Contacts

* Adriano Pimpini <adriano.pimpini@gmail.com>
* Alessandro Pellegrini <a.pellegrini@ing.uniroma2.it>

Reproducibility contact: Adriano Pimpini <adriano.pimpini@gmail.com>

## Requirements

* RAM: depending on the network size. For ~48 GB 100k nodes , ~8 GB for 10k nodes, <2 GB for 1k nodes or less.
* Disk space: realistically 2 GB. Or ~400 MB per iteration.

Configuration used by the authors:

* CPU: 2 x Intel Xeon E5-2696 v4 @ 2.20GHz
* RAM: 256 GB
* OS: Ubuntu 22.04.5 LTS

## Dependencies

The reproducibility code is containerized.
* Docker Engine
* Docker CLI
* Git (for the submodules)

## License

The software is released under the GPL 3.0 license.

## Setup
Update the submodules

```
git submodule update --init --recursive
```

You're ready to run.

## Reproducing the published results

The artifact is containerized, and a shell script `reproduce.sh` is provided that builds the container image, creates the output folder and spins up the container. The entrypoint to the container is the `entrypoint.py` Python script.

A configuration file is provided in the form of a JSON file containing a dictionary.
The dictionary has the following fields:
* `"what_to_reproduce"` - Specifies which experiments to run. Acceptable values are:
    * `"attacks"` to reproduce figures 3, 4, and 5.
    * `"figure_8"` to reproduce figure 8.
    * `"performance"` to reproduce figures 6 and 7.
    * `"all"` to reproduce everything.
* `"network_sizes_attacks"` - Specifies the list of network sizes to run the attack simulations with `[100, 1000, 10000]` are the ones used in the paper.
* `"worker_threads_attacks"` - Specifies the (single) number of worker threads to use for attack evaluation simulations. Default is `8`, we suggest setting it to the highest number possible **less than or equal to 16** for a speedier reproduction of the results, but feel free to experiment. Do NOT pick a number higher than the available computational units on your machine, or the runs will not start.
* `"network_sizes_figure_8"` - Specifies the list of network sizes to run the attack simulations for Figure 8 with. `[1000, 10000, 100000]` are the ones used in the paper.
* `"worker_threads_figure_8"` - Specifies the (single) number of worker threads to use for attack evaluation simulations. Default is `8`, we suggest setting it to the highest number possible **less than or equal to 16** for a speedier reproduction of the results, but feel free to experiment. Do NOT pick a number higher than the available computational units on your machine, or the runs will not start.
* `"network_sizes_performance"` - Specifies the list of network sizes to run the performance experiments with `[100, 1000, 10000, 100000]` are the ones used in the paper.
* `"worker_threads_performance"` - Specifies the various numbers of worker threads to run the simulation with **for performance experiments** (Figures 6 and 7) `[1, 4, 8, 16, 22, 44, 66, 88]` are the values used in the paper. Change it to match the capabilities of your machine. Do NOT pick numbers higher than the available computational units on your machine, or the runs will not start.
* `"iterations"` - Specifies the number of times each experiment is to be repeated. Default `1`.

The run-time will heavily depend on the configuration.

**Note about Figure 8**: a version of Figure 8 will also be produced when running the reproduction configuration `"attacks"`.
However, the original Figure 8 also uses network size 100'000, which greatly increased the time required to reproduce the experiments.
A quality of life improvement is thus the reason for separating Figure 8 from the rest of attack-related experiments.

## Reproduced results

Once all the experiment are finished running, you will find the results in the `reproduced/plots/plots_N/` folder.
Every time the artifact is run, it finds the lowest `N` for which the `plots_N` does not exist, and uses it to store the generated plots.

Please note that variation from the presented results is expected, as (pseudo) random numbers are used.

## Reproducing CBlockSim's reported performance
Similarly to reproducing the published results, to reproduce the performance of CBlockSim you can use the command `reproduce_CBlockSim.sh`. It uses a config file `cblocksim_reproducibility_config.json` which holds the network sizes for which to reproduce the results. The script will print the execution times in seconds for each of the executed network sizes. The only possible network sizes are the ones used in the paper `[100, 1000, 10000, 100000]`. Only a subset of those is acceptable for the script.

import multiprocessing
import json
import os
import shutil

DRY_RUN = 0
PATH_TO_REPRO_RUNNER = "/RBlockSim_repro/RBlockSim/scripts/repro_runner.py"
PATH_TO_RBLOCKSIM = "/RBlockSim_repro/RBlockSim/rblocksim"
PATH_TO_PLOTTER = "/RBlockSim_repro/RBlockSim/scripts/repro_plotter.py"
attacks_sizes_to_folder = {}
performance_sizes_to_folder = {}
g_plots_folder = None


def main():
    with open("reproducibility_config.json") as f:
        config = json.load(f)

    check_config(config)

    should_reproduce_attacks = (
        config["what_to_reproduce"] == "attacks" or config["what_to_reproduce"] == "all"
    )
    should_reproduce_performance = (
        config["what_to_reproduce"] == "performance"
        or config["what_to_reproduce"] == "all"
    )

    create_folders(config)

    if should_reproduce_attacks:
        reproduce_attacks(
            config["worker_threads_attacks"],
            config["network_sizes_attacks"],
            config["iterations"],
        )
        plot_attacks(config)

    if should_reproduce_performance:
        reproduce_performance(
            config["worker_threads_performance"],
            config["network_sizes_performance"],
            config["iterations"],
        )
        plot_performance(config)


def create_folders(config):
    for size in config["network_sizes_attacks"]:
        results_folder = f"/results_attacks/results_{size}"
        os.mkdir(results_folder)
        attacks_sizes_to_folder[size] = results_folder

    for size in config["network_sizes_performance"]:
        results_folder = f"/results_performance/results_{size}"
        os.mkdir(results_folder)
        performance_sizes_to_folder[size] = results_folder

    # Create the plots folders
    counter = 0
    while os.path.exists(f"/reproduced/plots/plots_{counter}"):
        counter += 1

    global g_plots_folder
    g_plots_folder = f"/reproduced/plots/plots_{counter}"
    os.mkdir(g_plots_folder)


def build_executable(network_size):
    print("Generating network of size", network_size)
    os.chdir("/RBlockSim_repro/RBlockSim/src/")
    os.system(
        f"python3 generate_topology.py --n_nodes {network_size} --min_peers 2 --max_peers 6"
    )
    os.chdir("../")
    os.system("make rblocksim")


def reproduce_attacks(worker_threads, network_sizes, iterations):
    print("Running attacks simulation")

    for size in network_sizes:
        # Generate network
        build_executable(size)
        os.chdir("/RBlockSim_repro/RBlockSim/scripts/")

        # Create the config file for the runner
        config_51 = {
            "executable_path": PATH_TO_RBLOCKSIM,
            "run_type": "51",
            "worker_threads": worker_threads,
            "intervals": [600],
            "iterations": iterations,
            "network_size": size,
            "work_folder": attacks_sizes_to_folder[size],
        }

        config_selfish = {
            "executable_path": PATH_TO_RBLOCKSIM,
            "run_type": "selfish",
            "worker_threads": worker_threads,
            "intervals": [600],
            "iterations": iterations,
            "network_size": size,
            "work_folder": attacks_sizes_to_folder[size],
        }

        # Run
        print("Launching 51% attack simulation for network size", size)
        run_simulation(config_51)
        print("Launching selfish mining attack simulation for network size", size)
        run_simulation(config_selfish)

    # Copy the results to the reproduced folder
    # Find a progresive value to name the folder when copying, to avoid overwriting
    counter = 0
    while os.path.exists(f"/reproduced/results_attacks_{counter}"):
        counter += 1

    shutil.copytree(
        "/results_attacks/",
        f"/reproduced/results_attacks_{counter}",
        dirs_exist_ok=True,
    )


def reproduce_performance(worker_threads_list, network_sizes, iterations):
    print("Running performance simulations")

    for size in network_sizes:
        # Generate network
        build_executable(size)
        os.chdir("/RBlockSim_repro/RBlockSim/scripts/")

        config_performance = {
            "executable_path": PATH_TO_RBLOCKSIM,
            "run_type": "benchmark",
            "worker_threads": worker_threads_list,
            "intervals": [13, 600],
            "iterations": iterations,
            "network_size": size,
            "work_folder": performance_sizes_to_folder[size],
        }

        print("Launching performance simulation for network size", size)
        run_simulation(config_performance)

    # Copy the results to the reproduced folder
    # Find a progressive value to name the folder when copying, to avoid overwriting
    counter = 0
    while os.path.exists(f"/reproduced/results_performance_{counter}"):
        counter += 1

    shutil.copytree(
        "/results_performance/",
        f"/reproduced/results_performance_{counter}",
        dirs_exist_ok=True,
    )


def plot_performance(config):
    # Populate the config file for the plotter script
    plotter_config = {
        "should_plot_performance": True,
        "should_plot_attacks": False,
        "network_sizes_performance": config["network_sizes_performance"],
        "performance_network_size_to_data_folders": performance_sizes_to_folder,
        "plot_output_folder": g_plots_folder,
    }

    print(plotter_config)

    invoke_plotter(plotter_config)


def plot_attacks(config):
    # Populate the config file for the plotter script
    plotter_config = {
        "should_plot_performance": False,
        "should_plot_attacks": True,
        "network_sizes_attacks": config["network_sizes_attacks"],
        "attacks_network_size_to_data_folders": attacks_sizes_to_folder,
        "plot_output_folder": g_plots_folder,
    }

    print(plotter_config)

    invoke_plotter(plotter_config)


def invoke_plotter(plotter_config):
    # Dump the config to a file
    with open("plotter_config.json", "w") as f:
        json.dump(plotter_config, f)

    # Run the plotter
    os.system(f"python3 {PATH_TO_PLOTTER} plotter_config.json")


def run_simulation(config):
    # Dump the config to a file
    with open("config.json", "w") as f:
        json.dump(config, f)

    # Run the simulation
    os.system(f"python3 {PATH_TO_REPRO_RUNNER} {DRY_RUN} config.json")


def check_config_attacks(config):
    if config["what_to_reproduce"] == "performance":
        return

    if config["worker_threads_attacks"] < 1:
        # Error: no worker threads
        print(
            "Error: attacks simulation requested no worker threads. Please select a number above 0"
        )
        exit(1)

    max_cpus = multiprocessing.cpu_count()
    if config["worker_threads_attacks"] > max_cpus:
        # Error: too many worker threads
        print(
            "Error: attacks simulation requested more worker threads than available cores. Please select a number below or equal to",
            max_cpus,
        )
        exit(1)


def check_config_performance(config):
    if config["what_to_reproduce"] == "attacks":
        return

    if min(config["worker_threads_performance"]) < 1:
        # Error: no worker threads
        print(
            "Error: performance simulation requested no worker threads. Please select a number above 0"
        )
        exit(1)

    max_cpus = multiprocessing.cpu_count()
    if max(config["worker_threads_performance"]) > max_cpus:
        # Error: too many worker threads
        print(
            "Error: performance simulation requested more worker threads than available cores. Please select a number below or equal to",
            max_cpus,
        )
        exit(1)


def check_config(config):
    return check_config_attacks(config) and check_config_performance(config)


if __name__ == "__main__":
    main()

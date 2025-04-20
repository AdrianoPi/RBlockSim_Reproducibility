import matplotlib.pyplot as plt
from matplotlib.legend_handler import HandlerLine2D
import numpy as np
from scipy.interpolate import make_interp_spline

import os
import sys
import re
import json

# Some definitions for accessing the data
# stats is a list of lists [[Attacker blocks committed to MC], [MC height], [Blocks mined by me], [Own blocks in MC], [Switches to selfish-mined chain]]
STATS_ATTACKER_BLOCKS_COMMITTED_TO_MC = 0
STATS_MC_HEIGHT = 1
STATS_BLOCKS_MINED_BY_ME = 2
STATS_OWN_BLOCKS_IN_MC = 3
STATS_SWITCHES_TO_SELFISH = 4

FONT_SIZE = 13
LABEL_FONT_SIZE = 18

g_network_sizes = None
g_data_path = None  # Read from config "/path/to/results/folder/{netsz}_nodes"
ram_peaks_name = "RAM_usage_peaks_Bytes_{netsize}_a{runtype}.json"
g_plot_output_folder = None  # Read from config "/path/to/output/folder/plots"

g_processed_json_files_folder = None  # This will hold the json files that have been read and dumped, ready to read again


def read_performance_experiment_data(data_folder):
    """Reads experiment data from files in a folder and builds an array of tuples.

    Args:
        data_folder: Path to the folder containing experiment data files.

    Returns:
        An array of tuples (number_of_worker_threads, average_simulation_time).
    """

    data = {}

    for filename in sorted(os.listdir(data_folder)):
        print("Reading file:", filename)
        # Extract worker threads and iteration from filename
        # match = re.search(r"out_sz(\d+)_w(\d+)_bi(\d+)_it(\d+)\.txt", filename)
        match = re.search(
            r"out_sz(\d+k*)_w(\d+)_bi(\d+)_abenchmark_h0_c0_d0_rng(\d+)_it(\d+)\.txt",
            filename,
        )
        if match:
            worker_threads = int(match.group(2))
            block_interval = match.group(3)
            iteration = int(match.group(5))

            # Read file and extract simulation time
            filepath = os.path.join(data_folder, filename)
            simulation_time = 0
            with open(filepath, "r") as f:
                line = f.readlines()[-4]
                match = re.search(r"Simulation completed in (\d+\.\d+) seconds", line)
                if match:
                    simulation_time = float(match.group(1))

            # Add data to dictionary
            if block_interval not in data:
                data[block_interval] = {}
            # Average over iterations. If first iteration, no averaging. Otherwise, update average.
            if worker_threads not in data[block_interval]:
                data[block_interval][worker_threads] = []

            data[block_interval][worker_threads].append(simulation_time)

    output = {}
    # Calculate average simulation time for each worker thread count
    for block_interval, block_interval_data in data.items():
        for worker_threads, times in block_interval_data.items():
            average_time = sum(times) / len(times)
            # add the (worker_threads, average_time) tuple to the output list
            if block_interval not in output:
                output[block_interval] = []
            output[block_interval].append((worker_threads, average_time))

    for k in output:
        output[k].sort()

    return output


def plot_performance_experiment_results(
    data,
    reference_line=None,
    logscale=True,
    title="Run Time for different block intervals, changing worker threads",
    ylabel="Run Time (seconds) - log scale",
    xlabel="Number of Worker Threads",
    use_legend=True,
    legend_title="Block Interval\n(seconds)",
    save_path=None,
):
    """Plots run time vs number of worker threads for each node in a dictionary.

    Args:
        data: A dictionary where keys are experiment setup names and values are arrays of tuples (number_of_worker_threads, run_time).
        reference_line: A reference line to plot on the graph.
        logscale: Whether to use a logarithmic scale for the y-axis.
        title: The title of the plot.
        ylabel: The label for the y-axis.
        xlabel: The label for the x-axis.
        use_legend: Whether to use a legend.
    """

    scale = "log" if logscale else "linear"

    # Set default font size
    print("Default font size", plt.rcParams["font.size"])
    plt.rcParams["font.size"] = 20

    # Collect all y values for setting y-ticks
    all_x_values = set()
    for node_data in data.values():
        for num_threads, _ in node_data:
            all_x_values.add(num_threads)

    fig, ax = plt.subplots(
        figsize=(12, 6)
    )  # Create figure with twice as much width as height
    # Set y-axis scale
    ax.set_yscale(scale)

    # Create a grid for the plot
    ax.grid(True, color="lightgray", linestyle="--", linewidth=0.5, which="both")

    for data_label, node_data in data.items():
        # Unpack data into separate arrays
        num_threads, run_times = zip(*node_data)

        # Plot the data with a label for the node
        ax.plot(num_threads, run_times, marker="o", label=f"{data_label}")

    if scale == "linear":
        ax.set_ylim(bottom=0)

    # Add a reference line if specified
    if reference_line is not None:
        # Do not put the reference line in the legend. Rather, put the label on the graph itself.
        refline_label = "CBlockSim"
        ax.axhline(y=reference_line, color="r", linestyle="--")

        """
        outside_left_xytext = (-80, -2.75)
        inside_left_xytext = (ax.get_xbound()[0], 2)
        ax.annotate(refline_label, xy=(0, reference_line),
                    xytext=inside_left_xytext,  # Adjust label position relative to line end
                    textcoords="offset points")
        """

        inside_rightend_xytext = (-109, 2)
        ax.annotate(
            refline_label,
            xy=(ax.get_xbound()[1], reference_line),
            xytext=inside_rightend_xytext,
            textcoords="offset points",
        )
        toplim = ax.get_ylim()[1]
        if toplim / 1.5 < reference_line:
            ax.set_ylim(top=toplim * 1.5)

    # Set labels and title
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)

    # Set x-ticks using the union of all y values
    ax.set_xticks(sorted(all_x_values))

    # Add legend
    if use_legend:
        ncols = 1 if len(data) < 4 else 2
        plt.legend(
            title=legend_title, ncols=ncols, loc="center", bbox_to_anchor=(0.5, 0.355)
        )

    if save_path is not None:
        plt.savefig(save_path, bbox_inches="tight")

    plt.clf()
    plt.cla()


# Read RAM usage from file and put it in suitable format for plotting
def read_peak_ram_usage(r_path):
    with open(r_path, "r") as f:
        ram_usages = json.load(f)

    pivoted_ram_usages = {}
    for wt in ram_usages:
        for block_interval in ram_usages[wt]:
            if block_interval not in pivoted_ram_usages:
                pivoted_ram_usages[block_interval] = []
            pivoted_ram_usages[block_interval].append(
                (int(wt), ram_usages[wt][block_interval] / 1024 / 1024 / 1024)
            )

    for k in pivoted_ram_usages:
        pivoted_ram_usages[k].sort()

    return pivoted_ram_usages


def plot_performance(network_sizes, network_size_to_data_folders, plot_output_folder):
    # print parameters
    print("Network sizes:", network_sizes)
    print("Network size to data folders:", network_size_to_data_folders)
    print("Plot output folder:", plot_output_folder)

    for size in network_sizes:
        d_path = network_size_to_data_folders[size]
        expdata = read_performance_experiment_data(d_path)
        print(expdata)
        plot_performance_experiment_results(
            expdata,
            logscale=True,
            title="",
            # title=f"{size} nodes",
            save_path=os.path.join(plot_output_folder, f"Figure_6_{size}.eps"),
            use_legend=False,
        )

    # Read RAM peaks
    ram_usages = {}
    for size in network_sizes:
        d_path = network_size_to_data_folders[size]
        r_path = os.path.join(
            d_path, ram_peaks_name.format(netsize=size, runtype="benchmark")
        )
        ram_usages[size] = read_peak_ram_usage(r_path)

    # Plot peak RAM usages
    # Aggregate all read RAM usages, and plot them into a single logscale graph
    rams = {}
    for size, usages in ram_usages.items():
        for block_interval, ram_usage in usages.items():
            key = "Nodes " + str(size) + " - BI " + str(block_interval)
            if key not in rams:
                rams[key] = []
            rams[key] += ram_usage

    plot_performance_experiment_results(
        rams,
        logscale=False,
        title="",  # title="Peak RAM usage (GB)",
        ylabel="Peak RAM usage (GB)",
        xlabel="Number of Worker Threads",
        legend_title="Nodes - Block Interval (sec.)",
        save_path=os.path.join(plot_output_folder, "Figure_7.eps"),
    )


def split_runs_to_smaller_files(all_runs, output_folder, output_file_prefix):
    """Process the data and split it into small-ish json files"""
    all_runs_51 = all_runs["51"]
    all_runs_selfish = all_runs["selfish"]

    # I also want to separate the 51% runs by the catchup tolerance
    fiftyone_catchup = {}
    for run in all_runs_51:
        catchup_tolerance = run["attack_info"]["catchup_tolerance"]
        if catchup_tolerance not in fiftyone_catchup.keys():
            fiftyone_catchup[catchup_tolerance] = []

        fiftyone_catchup[catchup_tolerance].append(run)

    # Dump the 51% runs separated by catchup tolerance to a json file each
    for catchup in fiftyone_catchup.keys():
        print("output_path will be ", output_folder, output_file_prefix)
        output_path = os.path.join(
            output_folder, output_file_prefix + "_51_c" + str(catchup) + ".json"
        )
        with open(output_path, "w") as f:
            json.dump(fiftyone_catchup[catchup], f)

    # I also want to separate the selfish runs by the depth of the attack, and the catchup tolerance
    selfish_depths = {}
    for run in all_runs_selfish:
        depth = run["attack_info"]["depth"]
        catchup_tolerance = run["attack_info"]["catchup_tolerance"]
        if depth not in selfish_depths.keys():
            selfish_depths[depth] = {}

        if catchup_tolerance not in selfish_depths[depth].keys():
            selfish_depths[depth][catchup_tolerance] = []

        selfish_depths[depth][catchup_tolerance].append(run)

    # Dump the selfish runs separated by depth and catchup tolerance to a json file each
    for depth in selfish_depths.keys():
        for catchup in selfish_depths[depth].keys():
            output_path = os.path.join(
                output_folder,
                output_file_prefix
                + "_selfish_c"
                + str(catchup)
                + "_d"
                + str(depth)
                + ".json",
            )
            with open(output_path, "w") as f:
                json.dump(selfish_depths[depth][catchup], f)


def load_runs_separate_by_attack_type(main_folder_path):
    runs = {}
    for run_folder in os.listdir(main_folder_path):
        print(run_folder)
        if not run_folder.startswith("Results_"):
            continue

        run_folder_path = os.path.join(main_folder_path, run_folder)
        if not os.path.isdir(run_folder_path):
            continue

        single_run_data = load_attack_data(run_folder_path)
        if single_run_data is None:
            continue

        attack_type = single_run_data["attack_info"]["attack_type"]
        if attack_type not in runs:
            runs[attack_type] = []

        runs[attack_type].append(single_run_data)

    return runs


def load_attack_data(folder_path):
    files = os.listdir(folder_path)
    info_file = [f for f in files if f.endswith("attack_info.json")]
    if not info_file:
        return None
    info_file = info_file[0]
    data_file = [f for f in files if f.startswith("stats")]
    if not data_file:
        return None
    data_file = data_file[0]

    with open(os.path.join(folder_path, info_file), "r") as f:
        info = json.load(f)

    with open(os.path.join(folder_path, data_file), "r") as f:
        stats = json.load(f)

    attack_info = {}

    attack_info["attack_type"] = info["attack_type"]
    attack_info["attacker_id"] = int(info["attacker"])
    attack_info["attacker_hashpower"] = float(info["attacker_hashpower"])
    attack_info["depth"] = int(info["depth"])
    attack_info["catchup_tolerance"] = int(info["catchup_tolerance"])
    attack_info["failed_attacks"] = int(info["failed_attacks"])
    attack_info["successful_conceals"] = int(info["successful_conceals"])

    (
        _,
        network_size,
        wts,
        block_interval,
        attack,
        hashrate,
        risk_tolerance,
        depth,
        rng_seed,
    ) = data_file.split("_")
    network_size = network_size[2:]
    wts = wts[1:]
    block_interval = block_interval[2:]

    # Unused
    attack = attack[1:]
    hashrate = hashrate[1:]
    risk_tolerance = risk_tolerance[1:]
    depth = depth[1:]
    rng_seed = rng_seed[3:]
    # End unused

    simulation_info = {}
    simulation_info["network_size"] = network_size
    simulation_info["wts"] = wts
    simulation_info["block_interval"] = block_interval

    return {
        "stats": stats,
        "attack_info": attack_info,
        "simulation_info": simulation_info,
    }


def plot_attacks(network_sizes, network_size_to_data_folders, plot_output_folder):
    data_dir = os.path.dirname(list(network_size_to_data_folders.values())[0])
    plot_ram_usage_vs_sim_progress(data_dir, network_sizes, plot_output_folder)

    for size in network_sizes:
        main_folder_path = network_size_to_data_folders[size]
        all_runs = load_runs_separate_by_attack_type(main_folder_path)
        split_runs_to_smaller_files(all_runs, g_processed_json_files_folder, str(size))

    plot_expected_vs_actual_mining_rate(
        os.path.join(plot_output_folder, "Figure_3.eps")
    )

    for size in network_sizes:
        plot_51(size, os.path.join(plot_output_folder, f"Figure_4_fiftyone_{size}.eps"))
        plot_selfish(
            size, os.path.join(plot_output_folder, f"Figure_4_selfish_{size}.eps")
        )
        plot_selfish_success_rate(
            size, os.path.join(plot_output_folder, f"Figure_5_{size}.eps")
        )


def plot_figure_8(network_sizes, network_size_to_data_folders, plot_output_folder):
    data_dir = os.path.dirname(list(network_size_to_data_folders.values())[0])
    plot_ram_usage_vs_sim_progress(data_dir, network_sizes, plot_output_folder)


def load_smaller_json_files_together(input_folder, input_files):
    all_runs = []
    for f in input_files:
        with open(os.path.join(input_folder, f), "r") as f:
            all_runs.append(json.load(f))

    return all_runs


def plot_selfish_success_rate(size, out_path=None):
    plt.rcParams["font.size"] = FONT_SIZE

    size_str = str(size)
    files_selfish = [
        size_str + "_selfish_c1_d2.json",
        size_str + "_selfish_c1_d3.json",
        size_str + "_selfish_c2_d2.json",
        size_str + "_selfish_c2_d3.json",
    ]
    runs_selfish = load_smaller_json_files_together(
        g_processed_json_files_folder, files_selfish
    )
    labels = ["Tol. 1 Depth 2", "Tol. 1 Depth 3", "Tol. 2 Depth 2", "Tol. 2 Depth 3"]
    for i, r in enumerate(runs_selfish):
        x, y = extract_plot_data_for_attack_success_rate(runs_selfish[i])
        plt.plot(x, y, label=labels[i])
    plt.ylim(0)
    plt.legend()
    plt.xlabel("Attacker hashpower", fontsize=LABEL_FONT_SIZE)
    plt.ylabel("Selfish mining success rate", fontsize=LABEL_FONT_SIZE)
    if out_path is not None:
        plt.savefig(out_path, bbox_inches="tight")
    plt.clf()
    plt.cla()


def extract_plot_data_for_attack_success_rate(runs_data):
    # Now I want to see the success rate of the attack vs the attacker's hashpower
    hash_to_success = {}
    for run in runs_data:
        successful_conceals = run["attack_info"]["successful_conceals"]
        failed_conceals = run["attack_info"]["failed_attacks"]
        total = successful_conceals + failed_conceals

        if total != 0:
            actual_switches = compute_selfish_mining_successes(
                run["stats"]["data"],
                run["attack_info"]["depth"],
                int(run["simulation_info"]["network_size"]),
            )
            success_rate = float(actual_switches) / total
        else:
            success_rate = 0

        hash_power = run["attack_info"]["attacker_hashpower"]
        if hash_power not in hash_to_success.keys():
            hash_to_success[hash_power] = []

        hash_to_success[hash_power].append(success_rate)

    # Now hash_to_success holds all attacker hashes for which we ran the simulation, with associated a list of success rates
    # I now want to average said success rates, to have a single value
    hash_to_average_success = {}
    for hash in hash_to_success.keys():
        hash_to_average_success[hash] = sum(hash_to_success[hash]) / len(
            hash_to_success[hash]
        )

    # Now I want to plot the hashpower on the x axis and the average success rate on the y axis
    x = sorted(list(hash_to_average_success.keys()))
    y = []
    for hash in x:
        y.append(hash_to_average_success[hash])

    return x, y


def compute_selfish_mining_successes(stats, depth, nodes_count):
    # Stats is a list of lists [[Attacker blocks committed to MC], [MC height], [Blocks mined by me], [Own blocks in MC], [Switches to selfish-mined chain]]
    total = 0
    for s in stats:
        miner_blocks_in_mc = s[STATS_ATTACKER_BLOCKS_COMMITTED_TO_MC]
        total += miner_blocks_in_mc

    return float(total) / (depth * nodes_count)


def plot_expected_vs_actual_mining_rate(out_path=None):
    plt.rcParams["font.size"] = FONT_SIZE

    f_all = [
        fname
        for fname in os.listdir(g_processed_json_files_folder)
        if fname.endswith(".json")
    ]

    runs_all = load_smaller_json_files_together(g_processed_json_files_folder, f_all)

    x, y = extract_actual_node_hashrate(runs_all)

    expected = np.arange(0, 1, 0.01)
    plt.plot(expected, expected, "--", label="Expected", color="gray")

    plt.plot(x, y, label="Observed mining rate")

    plt.xlabel("Hash rate", fontsize=LABEL_FONT_SIZE)
    plt.ylabel("Observed block mining rate", fontsize=LABEL_FONT_SIZE)
    plt.legend()
    if out_path is not None:
        plt.savefig(out_path, bbox_inches="tight")
    plt.clf()
    plt.cla()


def plot_ram_usage_vs_sim_progress(data_folder_path, network_sizes, plot_output_folder):
    usages = []
    for size in network_sizes:
        new_usages = get_splined_ram_usage(data_folder_path, size)
        if new_usages is None:
            # Not enough datapoints to spline
            continue
        usages.append(new_usages)

    for size, usages in zip(network_sizes, usages):
        # structure of usages[attack_type][catchup_tolerance][depth][worker_threads] = (xnew, splined_usage)
        elected_usage = None
        elected_x = None
        for attack_type in usages.keys():
            for catchup_tolerance in usages[attack_type].keys():
                for depth in usages[attack_type][catchup_tolerance].keys():
                    for worker_threads in usages[attack_type][catchup_tolerance][
                        depth
                    ].keys():
                        x, splined_usage = usages[attack_type][catchup_tolerance][
                            depth
                        ][worker_threads]
                        elected_usage = splined_usage
                        elected_x = x
                    break
                break
            break

        plt.plot(elected_x, elected_usage, label=str(size) + " nodes")

    plt.legend()
    plt.xlabel("Simulation Progress %", fontsize=LABEL_FONT_SIZE)
    plt.ylabel("RAM usage (GB)", fontsize=LABEL_FONT_SIZE)
    plt.savefig(os.path.join(plot_output_folder, "Figure_8.eps"), bbox_inches="tight")
    plt.clf()
    plt.cla()


def get_splined_ram_usage(data_folder_path, network_size):
    all_runs = load_RAM_usages_separate_by_attack_type(data_folder_path, network_size)
    # Internal organization of the data: runs[attack_type][catchup_tolerance][depth][worker_threads] -> list of lists of RAM usages
    # I want to plot the RAM usages
    try:
        splined_usages = {}
        for attack_type in all_runs.keys():
            splined_usages[attack_type] = {}
            for catchup_tolerance in all_runs[attack_type].keys():
                splined_usages[attack_type][catchup_tolerance] = {}
                for depth in all_runs[attack_type][catchup_tolerance].keys():
                    splined_usages[attack_type][catchup_tolerance][depth] = {}
                    for worker_threads in all_runs[attack_type][catchup_tolerance][
                        depth
                    ].keys():
                        splined_usages[attack_type][catchup_tolerance][depth][
                            worker_threads
                        ] = {}

                        usages = all_runs[attack_type][catchup_tolerance][depth][
                            worker_threads
                        ]
                        elected_usage = usages[0]
                        for usage in usages:
                            if len(usage) > len(elected_usage):
                                elected_usage = usage
                            for i, point in enumerate(usage):
                                usage[i] = point / 1024 / 1024 / 1024  # Convert to GB
                            # plt.plot(usage)

                        xnew = np.linspace(0.0, 1.0, 300)
                        spl = make_interp_spline(
                            np.linspace(0.0, 1.0, len(elected_usage)),
                            elected_usage,
                            k=3,
                        )
                        splined_usage = spl(xnew)
                        splined_usages[attack_type][catchup_tolerance][depth][
                            worker_threads
                        ] = (xnew, splined_usage)
        return splined_usages
    except Exception as e:
        return None


def load_RAM_usages_separate_by_attack_type(folder_path, network_size):
    # To have a coherent reading of the data, I want to separate the runs by attack type, and then by catchup tolerance, and then by depth, and then by worker thread count
    runs = {}

    fiftyone_runs_metadata = []
    selfish_runs_metadata = []
    figure_8_runs_metadata = []

    # In the main folder, open the 'experiments_ran_metadata_and_RAM_{size}.json' file
    try:
        with open(
            os.path.join(
                folder_path,
                f"results_{network_size}",
                "experiments_ran_metadata_and_RAM_" + str(network_size) + "_a51.json",
            ),
            "r",
        ) as f:
            fiftyone_runs_metadata = json.load(f)
        
        with open(
        os.path.join(
            folder_path,
            f"results_{network_size}",
            "experiments_ran_metadata_and_RAM_" + str(network_size) + "_aselfish.json",
        ),
        "r",
        ) as f:
            selfish_runs_metadata = json.load(f)
    except:
        with open(
        os.path.join(
            folder_path,
            f"results_{network_size}",
            "experiments_ran_metadata_and_RAM_" + str(network_size) + "_afigure_8.json",
        ),
        "r",
        ) as f:
            figure_8_runs_metadata = json.load(f)


    runs_metadata = fiftyone_runs_metadata + selfish_runs_metadata + figure_8_runs_metadata

    for run in runs_metadata:
        attack_type = run["attack"]
        if attack_type == "selfish":
            depth = run["depth"]
        else:
            depth = "0"
        catchup_tolerance = run["catchup"]
        worker_threads = run["wt"]

        if attack_type not in runs.keys():
            runs[attack_type] = {}

        if catchup_tolerance not in runs[attack_type].keys():
            runs[attack_type][catchup_tolerance] = {}

        if depth not in runs[attack_type][catchup_tolerance].keys():
            runs[attack_type][catchup_tolerance][depth] = {}

        if worker_threads not in runs[attack_type][catchup_tolerance][depth].keys():
            runs[attack_type][catchup_tolerance][depth][worker_threads] = []

        # Append the usage data to the list
        runs[attack_type][catchup_tolerance][depth][worker_threads].append(
            run["RAM_history"]
        )

    return runs


def extract_actual_node_hashrate(configs):
    hashrate_to_minerate = {}
    for runs in configs:
        for run in runs:
            total_mined_blocks = 0
            for stat in run["stats"]["data"]:
                total_mined_blocks += stat[STATS_BLOCKS_MINED_BY_ME]

            attacker_id = run["attack_info"]["attacker_id"]
            attacker_mined_blocks = run["stats"]["data"][attacker_id][
                STATS_BLOCKS_MINED_BY_ME
            ]
            hashrate = run["attack_info"]["attacker_hashpower"]

            if hashrate not in hashrate_to_minerate.keys():
                hashrate_to_minerate[hashrate] = []
            hashrate_to_minerate[hashrate].append(
                float(attacker_mined_blocks) / total_mined_blocks
            )

    hashrate_to_average_mining_rate = {}
    # Now that I have the hashrate to mining rates mapping, i want to average the mining rates for each hashrate
    for hashrate in hashrate_to_minerate.keys():
        hashrate_to_average_mining_rate[hashrate] = sum(
            hashrate_to_minerate[hashrate]
        ) / len(hashrate_to_minerate[hashrate])
    # Now I want to plot the hashrate on the x axis and the average mining rate on the y axis
    x = sorted(list(hashrate_to_average_mining_rate.keys()))
    y = []
    for hashrate in x:
        y.append(hashrate_to_average_mining_rate[hashrate])
    return x, y


def extract_plot_data_for_average_attacker_committed_percentage(runs_data):
    # Now I want to plot the percentage of attacker's blocks in the main chain vs the attacker's hashpower
    hash_to_committed = {}
    for run in runs_data:
        committed_percent = compute_percentage_of_attacker_committed_blocks(
            run["stats"]["data"]
        )
        hash_power = run["attack_info"]["attacker_hashpower"]
        if hash_power not in hash_to_committed.keys():
            hash_to_committed[hash_power] = []

        hash_to_committed[hash_power].append(committed_percent)
    # Now hash_to_committed holds all attacker hashes for which we ran the simulation, with associated a list of percentages of attacker's committed blocks
    # I now want to average said percentages, to have a single value
    hash_to_average_committed = {}
    for hash in hash_to_committed.keys():
        hash_to_average_committed[hash] = sum(hash_to_committed[hash]) / len(
            hash_to_committed[hash]
        )
    # Now I want to plot the hashpower on the x axis and the average percentage of committed blocks on the y axis
    x = sorted(list(hash_to_average_committed.keys()))
    y = []
    for hash in x:
        y.append(hash_to_average_committed[hash])
    return x, y


def compute_percentage_of_attacker_committed_blocks(stats):
    # Stats is a list of lists [[Attacker blocks committed to MC], [MC height], [Blocks mined by me], [Own blocks in MC], [Switches to selfish-mined chain]]
    counts = {}
    for s in stats:
        attacker_committed = s[STATS_ATTACKER_BLOCKS_COMMITTED_TO_MC]
        main_chain_height = s[STATS_MC_HEIGHT]
        k = (attacker_committed, main_chain_height)
        if k not in counts.keys():
            counts[k] = 0
        counts[k] += 1

    best_count = 0
    winner_pair = None

    for pair in counts.keys():
        if counts[pair] > best_count:
            best_count = counts[pair]
            winner_pair = pair

    attacker_commit_percent = float(winner_pair[0]) / winner_pair[1]
    return attacker_commit_percent


def plot_51(size, out_path=None):
    plt.rcParams["font.size"] = FONT_SIZE

    files_51 = [str(size) + "_51_c1.json", str(size) + "_51_c2.json"]
    runs_51 = load_smaller_json_files_together(g_processed_json_files_folder, files_51)
    labels = ["Tolerance 1", "Tolerance 2"]

    for i, r in enumerate(runs_51):
        x, y = extract_plot_data_for_average_attacker_committed_percentage(r)
        plt.plot(x, y, label=labels[i])

    expected = np.arange(0, 1, 0.01)
    plt.plot(expected, expected, "--", label="Expected", color="gray")

    plt.legend()

    plt.xlabel("Attacker hashpower", fontsize=LABEL_FONT_SIZE)
    plt.ylabel("% of attacker blocks in main chain", fontsize=LABEL_FONT_SIZE)

    if out_path is not None:
        plt.savefig(out_path, bbox_inches="tight")
    plt.clf()
    plt.cla()


def plot_selfish(size, out_path=None):
    plt.rcParams["font.size"] = FONT_SIZE

    size_str = str(size)
    files_selfish = [
        size_str + "_selfish_c1_d1.json",
        size_str + "_selfish_c1_d2.json",
        size_str + "_selfish_c1_d3.json",
        size_str + "_selfish_c2_d1.json",
        size_str + "_selfish_c2_d2.json",
        size_str + "_selfish_c2_d3.json",
    ]
    runs_selfish = load_smaller_json_files_together(
        g_processed_json_files_folder, files_selfish
    )

    labels = [
        "Tol. 1 Depth 1",
        "Tol. 1 Depth 2",
        "Tol. 1 Depth 3",
        "Tol. 2 Depth 1",
        "Tol. 2 Depth 2",
        "Tol. 2 Depth 3",
    ]
    markers = ["o", "x", "s", "v", "^", "D"]

    expected = np.arange(0, 1, 0.01)
    plt.plot(expected, expected, label="Expected", color="gray")

    """Process the data and plot the percentage of attacker's blocks in the main chain vs the attacker's hashpower"""
    for i, r in enumerate(runs_selfish):
        x, y = extract_plot_data_for_average_attacker_committed_percentage(r)
        plt.plot(
            x,
            y,
            label=labels[i],
            linestyle=(0, (1, 1)),
            marker=markers[i],
            markersize=4,
        )

    del runs_selfish

    plt.legend()

    plt.xlabel("Attacker hashpower", fontsize=LABEL_FONT_SIZE)
    plt.ylabel("% of attacker blocks in main chain", fontsize=LABEL_FONT_SIZE)
    # plt.title('Average percentage of committed blocks vs attacker hashpower')
    if out_path is not None:
        plt.savefig(out_path, bbox_inches="tight")
    plt.clf()
    plt.cla()


if __name__ == "__main__":
    """
    # Replace this with your actual data dictionary
    my_data = {
        "100k": [(1, 10000), (2, 6000), (4, 3000), (88, 1000)],
        "10k": [(1, 1000), (2, 600), (4, 300), (88, 100)],
        "1k": [(1, 100), (2, 60), (4, 30), (88, 10)],
        "100": [(1, 10), (2, 6), (4, 3), (88, 1)],
    }
    # plot_experiment_results(my_data)
    """

    # Read config file
    if len(sys.argv) != 2:
        print(f"Usage: python3 {__file__} <config_file>")
        exit(1)

    config_file = sys.argv[1]
    with open(config_file, "r") as f:
        config = json.load(f)

    # Read config values
    print(config)

    if config["should_plot_performance"]:
        print("Plotting performance data")

        network_sizes = config["network_sizes_performance"]
        network_size_to_data_folders = {}
        sz_to_data_strkeys = config["performance_network_size_to_data_folders"]
        for k in sz_to_data_strkeys.keys():
            network_size_to_data_folders[int(k)] = sz_to_data_strkeys[k]
        plot_output_folder = config["plot_output_folder"]

        plot_performance(
            network_sizes, network_size_to_data_folders, plot_output_folder
        )

    if config["should_plot_attacks"]:
        print("Plotting attack data")

        network_sizes = config["network_sizes_attacks"]
        network_size_to_data_folders = {}
        sz_to_data_strkeys = config["attacks_network_size_to_data_folders"]
        for k in sz_to_data_strkeys.keys():
            network_size_to_data_folders[int(k)] = sz_to_data_strkeys[k]
        plot_output_folder = config["plot_output_folder"]

        g_processed_json_files_folder = os.path.join(
            config["plot_output_folder"], "jsons"
        )
        os.makedirs(g_processed_json_files_folder, exist_ok=True)

        plot_attacks(network_sizes, network_size_to_data_folders, plot_output_folder)

    if config["should_plot_figure_8"]:
        print("Plotting Figure 8")

        network_sizes = config["network_sizes_figure_8"]
        network_size_to_data_folders = {}

        sz_to_data_strkeys = config["figure_8_network_size_to_data_folders"]
        for k in sz_to_data_strkeys.keys():
            network_size_to_data_folders[int(k)] = sz_to_data_strkeys[k]

        plot_output_folder = config["plot_output_folder"]

        g_processed_json_files_folder = os.path.join(
            config["plot_output_folder"], "jsons"
        )
        os.makedirs(g_processed_json_files_folder, exist_ok=True)

        plot_figure_8(network_sizes, network_size_to_data_folders, plot_output_folder)

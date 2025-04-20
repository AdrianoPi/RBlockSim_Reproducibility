# This script will run RBlocksim with a varying amount of threads.
# Specify the number of threads in a list, then every time invoke the command with the correct number of threads.
# I also want to wait for each run to finish before starting the next one.
import random
import subprocess
import psutil
import time
import json
import sys
import shlex
import os

g_run_type = None
g_wts = None
g_intervals = None
g_iterations = None
g_network_size = None
modes = ["benchmark", "51", "selfish", "figure_8"]
catchup_tolerances = [1, 2]
depth = [1, 2, 3]
hashrates = range(1, 100, 5)
command_performance = (
    "{executable_path} -w {wt} -i {interval} -o out{netsize} -r {rng_seed}"
)
command_51 = "{executable_path} -w {wt} -i {interval} -a 51 -h {hashrate} -c {catchup} -o out{netsize} -r {rng_seed}"
command_selfish = "{executable_path} -w {wt} -i {interval} -a selfish -h {hashrate} -c {catchup} -d {depth} -s 0 -o out{netsize} -r {rng_seed}"
output_file = "{outerr}_sz{netsize}_w{wt}_bi{interval}_a{attack}_h{hashrate}_c{catchup}_d{depth}_rng{rng_seed}_it{iteration}.txt"
metadata_file = "experiments_ran_metadata_and_RAM_{netsize}_a{attack}.json"
ram_usage_file = "RAM_usage_peaks_Bytes_{netsize}_a{attack}.json"
global_metadata = []


def run_benchmark(executable_path, wts, intervals, iterations, network_size):
    global metadata_file
    metadata_file = metadata_file.format(netsize=network_size, attack="benchmark")
    global ram_usage_file
    ram_usage_file = ram_usage_file.format(netsize=network_size, attack="benchmark")
    for it in range(iterations):
        for wt in wts:
            for i in intervals:
                rng_seed = random.randint(0, 2**32 - 1)

                run_metadata = {
                    "wt": wt,
                    "interval": i,
                    "attack": "benchmark",
                    "rng_seed": rng_seed,
                    "iteration": it,
                }
                global_metadata.append(run_metadata)

                out_file = output_file.format(
                    outerr="out",
                    netsize=network_size,
                    wt=wt,
                    interval=i,
                    attack="benchmark",
                    hashrate=0,
                    catchup=0,
                    depth=0,
                    rng_seed=rng_seed,
                    iteration=it,
                )
                err_file = output_file.format(
                    outerr="err",
                    netsize=network_size,
                    wt=wt,
                    interval=i,
                    attack="benchmark",
                    hashrate=0,
                    catchup=0,
                    depth=0,
                    rng_seed=rng_seed,
                    iteration=it,
                )

                rng_seed = random.randint(0, 2**32 - 1)
                this_command = command_performance.format(
                    executable_path=executable_path,
                    wt=wt,
                    interval=i,
                    netsize=network_size,
                    rng_seed=rng_seed,
                )

                print(f"Running command: {this_command}")
                print(f"Output file: {out_file}")
                print(f"Error file: {err_file}")
                if dry_run:
                    print("")
                    continue

                # ADD RAM USAGE TRACKING

                proc = subprocess.Popen(
                    shlex.split(this_command),
                    stdout=open(out_file, "w"),
                    stderr=open(err_file, "w"),
                )

                # Track RAM usage and peak RAM usage
                peak_memory = 0
                while proc.poll() is None:
                    try:
                        # Get process memory usage
                        mem_info = psutil.Process(proc.pid).memory_info()
                        # Update peak if current is higher
                        usage = mem_info.rss
                        peak_memory = max(peak_memory, usage)
                    except psutil.NoSuchProcess:
                        # Process might have exited before memory info retrieval
                        pass
                    time.sleep(1)

                # Wait for process to finish (optional)
                proc.wait()
                print(
                    f"Finished iteration {it} for performance experiments - wt {wt}, interval {i}, seed {rng_seed}"
                )

                # Save peak memory usage
                RAM_peaks[wt][i] = max(RAM_peaks[wt][i], peak_memory)
                with open(ram_usage_file, "w") as f:
                    json.dump(RAM_peaks, f)

                with open(metadata_file, "w") as f:
                    json.dump(global_metadata, f)


def run_51(executable_path, wts, intervals, iterations, network_size):
    global metadata_file
    metadata_file = metadata_file.format(netsize=network_size, attack="51")
    global ram_usage_file
    ram_usage_file = ram_usage_file.format(netsize=network_size, attack="51")
    for it in range(iterations):
        for wt in wts:
            for i in intervals:
                for catchup in catchup_tolerances:
                    for hashrate in hashrates:
                        hashrate = hashrate / 100.0
                        rng_seed = random.randint(0, 2**32 - 1)
                        RAM_history = []

                        run_metadata = {
                            "wt": wt,
                            "interval": i,
                            "attack": "51",
                            "catchup": catchup,
                            "hashrate": hashrate,
                            "rng_seed": rng_seed,
                            "iteration": it,
                            "RAM_history": RAM_history,
                        }
                        global_metadata.append(run_metadata)

                        print(
                            f"Starting iteration {it} for size {network_size} wt {wt}, interval {i}, attack 51, hashrate {hashrate}, catchup {catchup}, seed {rng_seed}"
                        )

                        this_command = command_51.format(
                            executable_path=executable_path,
                            wt=wt,
                            interval=i,
                            hashrate=hashrate,
                            catchup=catchup,
                            rng_seed=rng_seed,
                            netsize=network_size,
                            iteration=it,
                        )

                        out_file = output_file.format(
                            outerr="out",
                            netsize=network_size,
                            wt=wt,
                            interval=i,
                            attack="51",
                            hashrate=hashrate,
                            catchup=catchup,
                            depth=0,
                            rng_seed=rng_seed,
                            iteration=it,
                        )
                        err_file = output_file.format(
                            outerr="err",
                            netsize=network_size,
                            wt=wt,
                            interval=i,
                            attack="51",
                            hashrate=hashrate,
                            catchup=catchup,
                            depth=0,
                            rng_seed=rng_seed,
                            iteration=it,
                        )

                        print(f"Running command: {this_command}")
                        print(f"Output file: {out_file}")
                        print(f"Error file: {err_file}")

                        if dry_run:
                            print("")
                            continue

                        proc = subprocess.Popen(
                            shlex.split(this_command),
                            stdout=open(out_file, "w"),
                            stderr=open(err_file, "w"),
                        )

                        # Track RAM usage and peak RAM usage
                        peak_memory = 0
                        while proc.poll() is None:
                            try:
                                # Get process memory usage
                                mem_info = psutil.Process(proc.pid).memory_info()
                                # Update peak if current is higher
                                usage = mem_info.rss
                                peak_memory = max(peak_memory, usage)
                                RAM_history.append(usage)
                            except psutil.NoSuchProcess:
                                # Process might have exited before memory info retrieval
                                pass
                            time.sleep(1)

                        # Wait for process to finish (optional)
                        proc.wait()
                        print(
                            f"Finished iteration {it} for wt {wt}, interval {i}, attack 51, hashrate {hashrate}, catchup {catchup}, seed {rng_seed}"
                        )

                        # Save peak memory usage
                        RAM_peaks[wt][i] = max(RAM_peaks[wt][i], peak_memory)
                        with open(ram_usage_file, "w") as f:
                            json.dump(RAM_peaks, f)

                        with open(metadata_file, "w") as f:
                            json.dump(global_metadata, f)


def run_selfish(executable_path, wts, intervals, iterations, network_size):
    global metadata_file
    metadata_file = metadata_file.format(netsize=network_size, attack="selfish")
    global ram_usage_file
    ram_usage_file = ram_usage_file.format(netsize=network_size, attack="selfish")

    for it in range(iterations):
        for wt in wts:
            for i in intervals:
                for catchup in catchup_tolerances:
                    for d in depth:
                        for hashrate in hashrates:
                            hashrate = hashrate / 100.0
                            rng_seed = random.randint(0, 2**32 - 1)
                            RAM_history = []

                            run_metadata = {
                                "wt": wt,
                                "interval": i,
                                "attack": "selfish",
                                "catchup": catchup,
                                "depth": d,
                                "hashrate": hashrate,
                                "rng_seed": rng_seed,
                                "iteration": it,
                                "RAM_history": RAM_history,
                            }
                            global_metadata.append(run_metadata)

                            print(
                                f"Starting iteration {it} for size {network_size} wt {wt}, interval {i}, attack selfish, hashrate {hashrate}, catchup {catchup}, seed {rng_seed}"
                            )

                            this_command = command_selfish.format(
                                executable_path=executable_path,
                                wt=wt,
                                interval=i,
                                hashrate=hashrate,
                                catchup=catchup,
                                depth=d,
                                rng_seed=rng_seed,
                                netsize=network_size,
                                iteration=it,
                            )

                            out_file = output_file.format(
                                outerr="out",
                                netsize=network_size,
                                wt=wt,
                                interval=i,
                                attack="selfish",
                                hashrate=hashrate,
                                catchup=catchup,
                                depth=0,
                                rng_seed=rng_seed,
                                iteration=it,
                            )
                            err_file = output_file.format(
                                outerr="err",
                                netsize=network_size,
                                wt=wt,
                                interval=i,
                                attack="selfish",
                                hashrate=hashrate,
                                catchup=catchup,
                                depth=0,
                                rng_seed=rng_seed,
                                iteration=it,
                            )

                            print(f"Running command: {this_command}")
                            print(f"Output file: {out_file}")
                            print(f"Error file: {err_file}")

                            if dry_run:
                                print("")
                                continue

                            proc = subprocess.Popen(
                                shlex.split(this_command),
                                stdout=open(out_file, "w"),
                                stderr=open(err_file, "w"),
                            )

                            # Track RAM usage and peak RAM usage
                            peak_memory = 0
                            while proc.poll() is None:
                                try:
                                    # Get process memory usage
                                    mem_info = psutil.Process(proc.pid).memory_info()
                                    # Update peak if current is higher
                                    usage = mem_info.rss
                                    peak_memory = max(peak_memory, usage)
                                    RAM_history.append(usage)
                                except psutil.NoSuchProcess:
                                    # Process might have exited before memory info retrieval
                                    pass
                                time.sleep(1)

                            # Wait for process to finish (optional)
                            proc.wait()
                            print(
                                f"Finished iteration {it} for wt {wt}, interval {i}, attack selfish, hashrate {hashrate}, catchup {catchup}, depth {d}, seed {rng_seed}"
                            )

                            # Save peak memory usage
                            RAM_peaks[wt][i] = max(RAM_peaks[wt][i], peak_memory)
                            with open(ram_usage_file, "w") as f:
                                json.dump(RAM_peaks, f)

                            # Save metadata. Twice in case I corrupt one of the files.
                            with open(metadata_file, "w") as f:
                                json.dump(global_metadata, f)


def run_figure_8(executable_path, wts, intervals, iterations, network_size):
    global metadata_file
    metadata_file = metadata_file.format(netsize=network_size, attack="figure_8")
    global ram_usage_file
    ram_usage_file = ram_usage_file.format(netsize=network_size, attack="figure_8")

    catchup = 2
    d = 2
    hashrate = 0.4

    for it in range(iterations):
        for wt in wts:
            for i in intervals:
                rng_seed = random.randint(0, 2**32 - 1)
                RAM_history = []

                run_metadata = {
                    "wt": wt,
                    "interval": i,
                    "attack": "figure_8",
                    "catchup": catchup,
                    "depth": d,
                    "hashrate": hashrate,
                    "rng_seed": rng_seed,
                    "iteration": it,
                    "RAM_history": RAM_history,
                }
                global_metadata.append(run_metadata)

                print(
                    f"Starting iteration {it} for Figure 8 size {network_size} wt {wt}, interval {i}, hashrate {hashrate}, catchup {catchup}, seed {rng_seed}"
                )

                this_command = command_selfish.format(
                    executable_path=executable_path,
                    wt=wt,
                    interval=i,
                    hashrate=hashrate,
                    catchup=catchup,
                    depth=d,
                    rng_seed=rng_seed,
                    netsize=network_size,
                    iteration=it,
                )

                out_file = output_file.format(
                    outerr="out",
                    netsize=network_size,
                    wt=wt,
                    interval=i,
                    attack="figure_8",
                    hashrate=hashrate,
                    catchup=catchup,
                    depth=0,
                    rng_seed=rng_seed,
                    iteration=it,
                )
                err_file = output_file.format(
                    outerr="err",
                    netsize=network_size,
                    wt=wt,
                    interval=i,
                    attack="figure_8",
                    hashrate=hashrate,
                    catchup=catchup,
                    depth=0,
                    rng_seed=rng_seed,
                    iteration=it,
                )

                print(f"Running command: {this_command}")
                print(f"Output file: {out_file}")
                print(f"Error file: {err_file}")

                if dry_run:
                    print("")
                    continue

                proc = subprocess.Popen(
                    shlex.split(this_command),
                    stdout=open(out_file, "w"),
                    stderr=open(err_file, "w"),
                )

                # Track RAM usage and peak RAM usage
                peak_memory = 0
                while proc.poll() is None:
                    try:
                        # Get process memory usage
                        mem_info = psutil.Process(proc.pid).memory_info()
                        # Update peak if current is higher
                        usage = mem_info.rss
                        peak_memory = max(peak_memory, usage)
                        RAM_history.append(usage)
                    except psutil.NoSuchProcess:
                        # Process might have exited before memory info retrieval
                        pass
                    time.sleep(1)

                # Wait for process to finish (optional)
                proc.wait()
                print(
                    f"Finished Figure 8 iteration {it} for wt {wt}, interval {i}, hashrate {hashrate}, catchup {catchup}, depth {d}, seed {rng_seed}"
                )

                # Save peak memory usage
                RAM_peaks[wt][i] = max(RAM_peaks[wt][i], peak_memory)
                with open(ram_usage_file, "w") as f:
                    json.dump(RAM_peaks, f)

                # Save metadata.
                with open(metadata_file, "w") as f:
                    json.dump(global_metadata, f)


if __name__ == "__main__":
    # Parse arguments. First is dry run, second is path to config file
    if len(sys.argv) != 3:
        print(f"Usage: python3 {__file__} <dry_run> <config_file>")
        exit(1)

    dry_run = int(sys.argv[1])
    if dry_run == 1:
        dry_run = True
        print("Dry run mode")
    else:
        dry_run = False
        print("Normal mode")

    config_file = sys.argv[2]
    with open(config_file, "r") as f:
        config = json.load(f)

    executable_path = config["executable_path"]
    g_run_type = config["run_type"]
    g_wts = config["worker_threads"]
    g_intervals = config["intervals"]
    g_iterations = config["iterations"]
    g_network_size = config["network_size"]

    if g_run_type not in modes:
        print(f"Invalid run type {g_run_type}")
        exit(1)

    # Suppose all other inputs are correct

    # Set up metadata for peak RAM usage
    global_metadata = []
    RAM_peaks = {}

    if not isinstance(g_wts, list):
        g_wts = [g_wts]
    for wt in g_wts:
        RAM_peaks[wt] = {}
        for i in g_intervals:
            RAM_peaks[wt][i] = 0

    # Change working directory to the correct one
    work_folder = config["work_folder"]
    if work_folder is not None:
        if not os.path.exists(work_folder):
            print(f"Error: work folder {work_folder} does not exist")
            exit(1)
        os.chdir(work_folder)
        print(f"Changed working directory: {os.getcwd()}")

    # Run experiments
    if g_run_type == "benchmark":
        run_benchmark(executable_path, g_wts, g_intervals, g_iterations, g_network_size)
    elif g_run_type == "51":
        run_51(executable_path, g_wts, g_intervals, g_iterations, g_network_size)
    elif g_run_type == "selfish":
        run_selfish(executable_path, g_wts, g_intervals, g_iterations, g_network_size)
    elif g_run_type == "figure_8":
        run_figure_8(executable_path, g_wts, g_intervals, 1, g_network_size)

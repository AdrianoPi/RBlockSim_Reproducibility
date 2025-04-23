import multiprocessing
import json
import os
import shutil
import time
from subprocess import Popen

DRY_RUN = 0
PATH_TO_CBLOCKSIM_FOLDER = "/RBlockSim_repro/CBlockSim/"
PATH_TO_BUILD_FOLDER = "/RBlockSim_repro/CBlockSim/build/"
PATH_TO_CONFIG_FILES = "/RBlockSim_repro/CBlockSim_aux/Configs/"
PATH_TO_PATCH = "/RBlockSim_repro/CBlockSim_aux/Patch/CBlockSim_no_print.patch"
CONFIG_FILE_PATTERN = "Configs_{size}.h"


def main():
    with open("cblocksim_reproducibility_config.json") as f:
        config = json.load(f)

    check_config(config)

    os.chdir(PATH_TO_CBLOCKSIM_FOLDER)
    # Remove unnecessary prints
    os.system(f"git apply {PATH_TO_PATCH}")

    runtimes = []
    sizes = config["network_sizes"]
    for size in sizes:
        os.chdir(PATH_TO_CBLOCKSIM_FOLDER)
        fname = CONFIG_FILE_PATTERN.format(size=size)
        fullpath = os.path.join(PATH_TO_CONFIG_FILES, fname)

        # Copy the config for size
        os.system(f"cp {fullpath} Configs.h")

        runtimes.append(run_cblocksim(size))
    
    print("\n\n\n")

    for i, time in enumerate(runtimes):
        size = sizes[i]
        print(f"CBlockSim execution time for size {size}: {time}")


def run_cblocksim(size):
    os.chdir(PATH_TO_BUILD_FOLDER)
    p = Popen("bash start.sh", shell=True)
    timeStarted = time.time()
    p.communicate()
    timeDelta = time.time() - timeStarted
    print(f"CBlockSim execution time for size {size}: {timeDelta}")
    return timeDelta


def check_config(config):
    for size in config["network_sizes"]:
        fname = CONFIG_FILE_PATTERN.format(size=size)
        fullpath = os.path.join(PATH_TO_CONFIG_FILES, fname)
        if not os.path.exists(fullpath):
            print(
                "ERROR: Requested network of size",
                f"{size},",
                "but no configuration exists for it (file",
                f"{fullpath},",
                "does not exist). Pick values in {100, 1000, 10000, 100000}",
            )
            exit(1)
    return


if __name__ == "__main__":
    main()

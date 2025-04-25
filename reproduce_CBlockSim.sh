#!/bin/bash

# To access the first commandline argument
$1

# Build the docker image
echo "Building docker image..."
docker build -t rblocksim:latest .

# Create the outputs directory
outdir="reproduced"
echo "Creating output directory '$outdir'..."

mkdir "$outdir"

# Start the container
echo "Starting container..."
command="docker run -it -v ./$outdir:/$outdir rblocksim:latest python3 entrypoint_CBlockSim.py"
echo "Running command: $command"
docker run -it -v ./$outdir:/$outdir rblocksim:latest python3 entrypoint_CBlockSim.py
#docker run -it -v ./$outdir:/$outdir rblocksim:latest /bin/bash

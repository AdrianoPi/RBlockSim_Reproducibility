#!/bin/bash

# Build the docker image
echo "Building docker image..."
docker build -t rblocksim:latest .

# Create the outputs directory
outdir="reproduced"
echo "Creating output directory '$outdir'..."
# Remove directory if it exists?
#if [-d "$outdir" ]; then

mkdir "$outdir"
mkdir "$outdir/plots"


# Start the container
echo "Starting container..."
command="docker run -it -v ./$outdir:/$outdir rblocksim:latest"
echo "Running command: $command"
docker run -it -v ./$outdir:/$outdir rblocksim:latest

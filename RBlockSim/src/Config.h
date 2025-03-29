#pragma once

#include "Topology.h"
#include "RBlockSim.h"

#define REGIONS_NUM 6
#define RNG_SEED 1234
#define TERMINATION_TIME (60 * 60 * 24) // 24 hours
#define TXN_NUMBER 500000
#define BLOCK_SIZE 0.18 //0.8  // Mb
extern double BLOCK_INTERVAL; // [seconds] Expected block time

#define BLOCK_VALIDATION_TIME 0.03 // [seconds] to validate a block

#define DEPTH_TO_KEEP 200 // Maximum depth to keep blocks for. Blocks deeper than this, might get dropped

#define GOSSIP_FANOUT 80 // Number of neighbors each node forwards the block to. Set to 0 to forward to all peers

extern struct simulation_configuration conf;
extern const double REGIONS_DISTRIBUTION[REGIONS_NUM];
extern const double LATENCIES[REGIONS_NUM][REGIONS_NUM]; // LATENCIES[i][j] is the latency[s] between region i and j
extern const double DOWNLOAD_BANDWIDTHS[REGIONS_NUM + 1];
extern const double UPLOAD_BANDWIDTHS[REGIONS_NUM + 1];
extern unsigned int rng_seed;

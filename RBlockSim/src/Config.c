#include "Config.h"

struct simulation_configuration conf = {
        .lps = N_NODES,
        .n_threads = 0,
        .termination_time = TERMINATION_TIME,
        .gvt_period = 100000,
        .log_level = LOG_DEBUG,
        .core_binding = true,
        .dispatcher = ProcessEvent,
        .committed = CanEnd
};

double BLOCK_INTERVAL = 13; //(10*60) // [seconds] Expected block time

const double REGIONS_DISTRIBUTION[REGIONS_NUM] = { 0.476, 0.222, 0, 0.297, 0.005, 0 }; //{0.2712, 0.5539, 0.0094, 0.1235, 0.0253, 0.0168};//{ 0.476, 0.222, 0, 0.297, 0.005, 0 }; // NodeTracker + Archive 2019

// Latency&Bandwidth Settings
// LATENCIES[i][j] is the latency between regions i and j
const double LATENCIES[REGIONS_NUM][REGIONS_NUM] = {{0.032, 0.124, 0.184, 0.198, 0.151, 0.189},
                                                    {0.124, 0.011, 0.227, 0.237, 0.252, 0.294},
                                                    {0.184, 0.227, 0.088, 0.325, 0.301, 0.322},
                                                    {0.198, 0.237, 0.325, 0.085, 0.058, 0.198},
                                                    {0.151, 0.252, 0.301, 0.058, 0.012, 0.126},
                                                    {0.189, 0.294, 0.322, 0.198, 0.126, 0.016}};

// download bandwidth [Mbps] of each region, the last one is the inter - regional bandwidth, from SimBlock
const double DOWNLOAD_BANDWIDTHS[REGIONS_NUM + 1] = { 52, 40, 18, 22.8, 22.8, 29.9, 6 };
// upload bandwidth [Mbps] of each region, the last one is the inter - regional bandwidth, from SimBlock
const double UPLOAD_BANDWIDTHS[REGIONS_NUM + 1] = { 19.2, 20.7, 5.8, 15.7, 10.2, 11.3, 6 };

unsigned int rng_seed = RNG_SEED;
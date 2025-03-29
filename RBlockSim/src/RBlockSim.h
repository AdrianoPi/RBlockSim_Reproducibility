#pragma once

#include <ROOT-Sim.h>
#include <ROOT-Sim/random.h>
#include <stdatomic.h>
#include "Typedefs.h"
#include "Config.h"

extern __thread node_id_t currentNode;

/**
 * Main file for the model.
 * Holds common declarations.
 * */

enum rblocksim_event {
    RBLOCKSIM_INIT = 0, // Internal initialization event. Used for e.g. computing the portion of hashpower w.r.t. the total.
    RECEIVE_BLOCK,
    REQUEST_BLOCK,
    GENERATE_BLOCK = LP_RETRACTABLE
};

//simtime_t transmissionDelays[N_NODES][N_NODES];

extern struct simulation_configuration conf;
extern struct SharedHashPower totalHashPower;

bool CanEnd(lp_id_t me, const void *snapshot);

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *event_content,
                  unsigned event_size, void *st);

/// Information to uniquely identify the requested block
struct request_block_evt {
    node_id_t requester;
    node_id_t miner;
    size_t height;
};

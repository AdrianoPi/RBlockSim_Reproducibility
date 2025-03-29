#pragma once
#include "RBlockSim.h"

#define N_NODES 1000
#define MIN_PEERS 40
#define MAX_PEERS 120

extern const size_t peer_list_sizes[N_NODES]; /// Holds the size of the peer list for each node
extern const node_id_t peer_lists[N_NODES][MAX_PEERS]; /// Holds the list of peers for each node

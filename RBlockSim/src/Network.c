#include "Network.h"
#include "util.h"

/**
 * Holds topology information to allow for delay calculation
 * */

size_t nodes_in_region[REGIONS_NUM]; /// Holds the number of nodes in each region

void initNetwork() {
    // Initialize the number of nodes in each region
    size_t tot = 0;
    for (int i = 0; i < REGIONS_NUM - 1; i++) {
        nodes_in_region[i] = N_NODES * REGIONS_DISTRIBUTION[i];
        tot += nodes_in_region[i];
    }
    nodes_in_region[REGIONS_NUM - 1] = N_NODES - tot;
}

void deinitNetwork() {}

int getRegion(node_id_t node) {
    for (int i = 0; i < REGIONS_NUM; i++) {
        if (node < nodes_in_region[i]) {
            return i;
        }
        node -= nodes_in_region[i];
    }
    fprintf(stderr, "Node %u not found in any region\n", node);
    abort();
}

/**
 * @brief Return the time required to transmit data from \a src to \a dst
 *
 * @param src ID of sender node
 * @param dst ID of destination node
 * @param data_size [Currently ignored] Size of data to transfer
 * @param rng Random Number Generator state
 *
 * @return Transmission delay
 */
simtime_t getTransmissionDelay(node_id_t src, node_id_t dst, size_t data_size, struct rng_t *rng) {
    if (!rng) {
        return LATENCIES[getRegion(src)][getRegion(dst)];
    }
    return Expent(rng, LATENCIES[getRegion(src)][getRegion(dst)]);
}

/**
 * @brief Propagates a single block to a single node
 * @param sender The ID of the node sending the block
 * @param receiver The ID of the node receiving the block
 * @param send_time The time at which the block sending starts
 * @param block The Block itself
 * @param rng Random Number Generator state
 * @param evt_type The event type for the message scheduling
 */
void sendSingleBlock(node_id_t sender, node_id_t receiver, simtime_t send_time, struct Block *block, struct rng_t *rng,
                     enum rblocksim_event evt_type) {
    size_t event_size = sizeof(struct Block) + sizeofAdditionalTransactionDataBuffer(
            block->transactionData.high - block->transactionData.low);
    simtime_t delivery_time = send_time + getTransmissionDelay(sender, receiver, block->size, rng);
    ScheduleNewEvent(receiver, delivery_time, evt_type, block, event_size);
}

/**
 * @brief Propagates a block via gossiping
 * @param sender The ID of the node sending the block
 * @param send_time The time at which the block sending starts
 * @param block The Block itself
 * @param rng Random Number Generator state
 * @param peers The list of connected peers
 * @param n_peers The number of connected peers
 */
void
gossipBlock(node_id_t sender, simtime_t send_time, const struct Block *block, struct rng_t *rng, const node_id_t *peers,
            size_t n_peers) {
    size_t event_size = sizeof(struct Block) + sizeofAdditionalTransactionDataBuffer(
            block->transactionData.high - block->transactionData.low);

    // From the list of connected nodes, select a random subset of nodes to send the block to, and send it to them
    if (!GOSSIP_FANOUT || n_peers <= GOSSIP_FANOUT || block->miner == sender) {
        // If the fanout is not bigger than the number of peers, send to all
        for (size_t i = 0; i < n_peers; i++) {
            simtime_t delivery_time = send_time + getTransmissionDelay(sender, peers[i], block->size, rng);
            ScheduleNewEvent(peers[i], delivery_time, RECEIVE_BLOCK, block, event_size);
        }
    } else {
        // Otherwise, select a random subset of nodes. Do not select the same node twice.
        block_bitmap *selected = malloc(bitmap_required_size(n_peers));
        bitmap_initialize(selected, n_peers);
        for (size_t i = 0; i < GOSSIP_FANOUT; i++) {
            node_id_t selected_peer = (node_id_t) RandomRange(rng, 0, (int) n_peers - 1);
            while (bitmap_check(selected, selected_peer)) {
                selected_peer = (node_id_t) RandomRange(rng, 0, (int) n_peers - 1);
            }
            bitmap_set(selected, selected_peer);
            simtime_t delivery_time = send_time + getTransmissionDelay(sender, peers[selected_peer], block->size, rng);
            ScheduleNewEvent(peers[selected_peer], delivery_time, RECEIVE_BLOCK, block, event_size);
        }
        free(selected);
    }

    // Other changes that need to be done: when a block without parent is received, keep it but ask for the parent from the sender node
}

void send_to_everyone(node_id_t sender, simtime_t send_time, struct Block *block, struct NodeState *state) {
    simtime_t max_d_time = 0;
    size_t event_size = sizeof(struct Block) + sizeofAdditionalTransactionDataBuffer(
            block->transactionData.high - block->transactionData.low);
    for (int d = 0; d < conf.lps; d++) {
        if (d == sender) {
            continue;
        }
        simtime_t delivery_time = send_time + getTransmissionDelay(sender, d, block->size, state->rng);
        max_d_time = max_d_time > delivery_time ? max_d_time : delivery_time;
        ScheduleNewEvent(d, delivery_time, RECEIVE_BLOCK, block, event_size);
    }
}

/**
 * @brief Propagates the newly generated block with the chosen approach
 * @param sender The ID of the node generating the block
 * @param send_time The time at which the block sending starts
 * @param block Pointer to the created Block
 * @param rng Random number generator state
 *
 * Takes care of propagating the block to OTHER NODES using the chosen propagation algorithm
 */
void propagateBlock(node_id_t sender, simtime_t send_time, const struct Block *block, struct rng_t *rng) {
    gossipBlock(sender, send_time, block, rng, peer_lists[sender], peer_list_sizes[sender]);
}

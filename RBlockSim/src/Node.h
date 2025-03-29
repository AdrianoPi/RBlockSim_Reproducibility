#pragma once

#include "Block.h"
#include "Statistics.h"

/**
 * Node represents a Node in the peer-to-peer network.
 * A node may
 * 1. Mine blocks
 * 2. Receive blocks
 * 3. Verify blocks
 * */

/**
 * More complex nodes can add other functionalities such as transaction generation, relaying etc.
 * */

struct NodeState {
    struct rng_t *rng;                        ///< Random Number Generator state
    struct BlockchainState blockchainState;   ///< Blockchain management information
    struct TransactionState transactionState; ///< Transaction management information
    struct StatsState statsState;             ///< Statistics management information
};

struct AttackerNodeState {
    struct NodeState nodeState;    ///< The basic node state
    size_t last_propagated_height; ///< The height of the last block propagated
    uint32_t failed_attacks;       ///< The number of failed attacks
    uint32_t successful_conceals;   ///< The number of times the attacker successfully concealed enough blocks
    bool isSelfishMining;          ///< Whether the attacker is currently selfish mining or not
    bool finishedSelfishMining;    ///< Whether the attacker has finished selfish mining
};

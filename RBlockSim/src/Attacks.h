#pragma  once

#include "Typedefs.h"
#include <ROOT-Sim/random.h>

#define DEFAULT_SELFISH_HASHPOWER 0.34
#define DEFAULT_SELFISH_DEPTH 2
#define DEFAULT_FIFTY_ONE_HASHPOWER 0.51
#define DEFAULT_CATCHUP_TOLERANCE 1
#define DEFAULT_SELFISH_START_TIME 600.0

enum attack_type {
    ATTACK_NONE,
    ATTACK_SELFISH_MINING,
    ATTACK_FIFTY_ONE,
};

struct selfish_config {
    double hashPowerPortion; ///< Hash power of the selfish miner
    size_t depth;            ///< Depth of the selfish mining attack
    size_t catchupTolerance; ///< Number of blocks the attacker can be behind the main chain, before switching back to it
    simtime_t startTime;     ///< Time at which the attacker starts concealing blocks
};

struct fifty_one_config {
    double hashPowerPortion; ///< Hash power of the selfish miner
    size_t catchupTolerance; ///< Number of blocks the attacker can be behind the main chain, before switching back to it
};

// Holds the configuration for the attack
struct attack_config {
    enum attack_type type;
    union {
        struct selfish_config selfish;
        struct fifty_one_config fiftyOne;
    };
};

// Here are variables for simulating attacks
extern node_id_t *attacker_ids;
extern size_t num_attackers;
extern struct attack_config attackConfig;

/**
 * @brief Initializes the attacker management information
 *
 * @param attackers_count The number of attackers to simulate
 * @param rng The random number generator state
 * */
void initAttackers(size_t attackers_count, struct rng_t *rng);

/**
 * @brief Releases the resources allocated for the attackers management
 * */
void deinitAttackers();

/**
 * @brief Checks if a node is an attacker
 *
 * @param node_id The ID of the node to check
 *
 * @returns true if the node is an attacker, false otherwise
 * */
bool is_attacker(node_id_t node_id);

/**
 * @brief Prints the attackers information to the standard output
 * */
void printAttackers();
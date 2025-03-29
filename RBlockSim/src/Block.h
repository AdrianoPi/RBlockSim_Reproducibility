#pragma once

#include "RBlockSim.h"
#include "Transaction.h"
#include "Statistics.h"

/**
 * Header for Block-related functionality.
 * */

#define isOrphan(ChainNode_ptr) ((ChainNode_ptr)->flags & CHAIN_NODE_FLAG_ORPHAN)

struct SharedHashPower {
    union {
        atomic_uint_fast64_t hashpower_atomic;
        unsigned long long hashpower;
    };
};

extern struct SharedHashPower totalHashPower;

/// Transfer object for a Block in the chain. Counterpart of ChainNode
struct Block {
    double timestamp;          ///< Creation timestamp
    size_t size;               ///< Size in memory
    node_id_t miner;           ///< ID of node that mined the block
    node_id_t prevBlockMiner;  ///< ID of node that mined the PARENT block
    node_id_t sender;          ///< ID of node that last relayed the block
    size_t height;             ///< Block number AND height (no miner will mine at the same height twice)
    bool is_attack_block;      ///< True if the block is the last of a selfish mining attack
    struct TransactionData transactionData; ///< Transaction information, transparent for this layer
};

enum chain_node_flag { CHAIN_NODE_FLAG_ORPHAN = 1, CHAIN_NODE_FLAG_INCLUDED = 2 };

/// Storage object for a Block in the chain. Counterpart of Block
struct ChainNode {
    union {
        size_t parent_index;                  ///< Index of parent ChainNode inside of the ChainLevel previous height
        node_id_t parentMinerId;              ///< Id of the parent's miner. Used when still an orphan.
    };
    struct TransactionData *transactionData;  ///< Transaction information, transparent for this layer
    double timestamp;                         ///< Block timestamp
    node_id_t miner;                          ///< ID of node that mined the block
    size_t height;                            ///< Block number AND height (no miner will mine at the same height twice)
    uint32_t score;                           ///< Consensus score for the chain this block is the last of. For longest chain it is the height, for GHOST it is the amount of work done.
    uint32_t flags;                           ///< Flags of the node.
    uint32_t ancestorsMined;                  ///< How many ancestors of this node have been mined by me, including this node
};

/// Single level of the blockchain. Contains all Blocks at a given height
struct ChainLevel {
    struct ChainNode* nodes;    ///< Array of all Blocks at this level
    size_t size;                ///< How many ChainNodes are actually present in the @a nodes array
    size_t capacity;            ///< Few siblings are expected per level, grows one entry at a time
};

/// The entire blockchain
struct Blockchain {
    struct ChainLevel *current_levels;   ///< Current array of ChainLevels
    struct ChainLevel *old_levels;       ///< Old array of ChainLevels
    size_t main_chain_index;             ///< Index of main chain ChainNode inside of the ChainLevel of relevant height
    size_t height;                       ///< Height of last block of main chain
    size_t max_height;                   ///< Height of the highest block in the chain. Includes orphans
    size_t min_height;                   ///< Height of the deepest block still held in the chain
};

/// Portion of the state dedicated to mining information
struct MiningState {
    union {
        uint_fast64_t hashPower;    /// Needed for init
        double hashPowerPortion;    /// Percentage of hashpower w.r.t. the total amount.
    };
};

/// Portion of the state dedicated to blockchain management
struct BlockchainState {
    struct Blockchain chain;   /// The actual chain
    struct MiningState miningState;
    unsigned mined_by_me;      /// Count of Blocks produced by the node
};

/**
 * @brief Returns a pointer to the last ChainNode of the main chain of 'blockchain'
 * @param blockchain Pointer to the Blockchain instance
 */
struct ChainNode *getMainChain(const struct Blockchain *blockchain);

/**
 * @brief Retrieves a Block starting from the chain
 *
 * @warning Caller owns the returned pointer and must free it when done
 *
 * @param[in] chain the Blockchain to search in
 * @param[in] miner ID of the block miner
 * @param[in] height the height of the ChainNode
 *
 * @return pointer to the Block if found
 */
struct Block *retrieveBlock(struct Blockchain *chain, node_id_t miner, size_t height);

/**
 * @brief Finds a ChainNode in the chain
 *
 * @param[in] chain the Blockchain to search in
 * @param[in] miner ID of the block miner
 * @param[in] height the height of the ChainNode
 *
 * @return pointer to the ChainNode if found, NULL otherwise
 */
struct ChainNode *findChainNode(struct Blockchain *chain, node_id_t miner, size_t height);

/**
 * @brief Generates a block
 * @param me The ID of the node generating the block
 * @param now The time at which the block generation is triggered
 * @param state The state portion for blockchain management
 * @param transactionState The state portion for transaction management. NULL to ignore.
 * @param statsState The state portion for statistics management
 *
 * Creates and populates a Block. If transactions are taken into account, they are also selected here.
 */
struct Block *generateBlock(node_id_t me, simtime_t now, struct BlockchainState *state, struct TransactionState *transactionState, struct StatsState *statsState);

/**
 * @brief Validate a Block
 *
 * @param block the block to validate
 * @param[out] is_valid will hold True if the block is considered valid
 *
 * @return The processing time needed to validate the block
 */
simtime_t validateBlock(const struct Block *block, bool *is_valid);

/**
 * @brief The node receives a Block. Adds it to the local chain if valid
 * @param now the time at which the block is received
 * @param state state portion for the blockchain management
 * @param transactionState state portion for transaction management
 * @param block the block to add to the chain
 * @param me the ID of the node
 * @param statsState state portion for statistics management
 * @param[out] updatedMainChain set to true if main chain was updated
 * @param[out] foundParent set to true if the parent of the block was found
 *
 * @return The processing time needed to perform the operations
 */
simtime_t
receiveBlock(simtime_t now, struct BlockchainState *blockchainState, struct TransactionState *transactionState,
             const struct Block *block, node_id_t me, struct StatsState *statsState, bool *updatedMainChain, bool *foundParent);

/**
 * @brief Computes how long until a block is generated given the portion of hash power
 * @param rng Pointer to random number generator state
 * @param hashPowerPortion The portion of hash power (with respect to total network hash power) of the generating node
 *
 * @returns The time interval until next block generation
 */
simtime_t getNextGenDelay(struct rng_t *rng, double hashPowerPortion);

/**
 * @brief Schedules a retractable event to maybe generate a block in the future
 * @param now The time at which the block generation is triggered
 * @param rng Random Number Generator state
 * @param hashPowerPortion The node's portion of hash power with respect to the entire network
 *
 * This selects a generation timestamp and schedules a retractable event.
 * The block is actually generated once the scheduled event is extracted.
 */
void scheduleNextBlockGeneration(simtime_t now, struct rng_t *rng, struct BlockchainState *state);

/**
 * @brief Gets the height of blockchain's main chain
 * @param blockchain The blockchain for which the height is required
 *
 * @returns The height of the main chain
 */
size_t getMainChainHeight(struct Blockchain *blockchain);

/**
 * @brief Initializes a BlockchainState
 *
 * @param state Pointer to the BlockchainState to initialize
 * @param rng Pointer to the random number generator state
 */
void initBlockchainState(struct BlockchainState *state, struct rng_t *rng);

/**
 * @brief Initializes a BlockchainState for attacker nodes
 *
 * @param state Pointer to the BlockchainState to initialize
 * @param rng Pointer to the random number generator state
 */
void attackerInitBlockchainState(struct BlockchainState *state, struct rng_t *rng);

/**
 * @brief Deinitializes BlockchainState \a state
 *
 * @param state Pointer to the BlockchainState to deinitialize
 */
void deinitBlockchainState(struct BlockchainState *state);

/**
 * @brief Deinitializes Blockchain \a chain
 *
 * @param chain Pointer to the Blockchain to deinitialize
 */
void deinitBlockChain(struct Blockchain *chain);

/**
 * @brief Performs initialization actions that need to be carried out after everyone has completed the first initialization round
 *
 * @param state Pointer to the BlockchainState
 */
void afterInitBlockchainState(struct BlockchainState *state);

/**
 * @brief Secondary initialization for attackers
 *
 * @param state Pointer to the BlockchainState
 */
void attackerAfterInitBlockchainState(struct BlockchainState *state);

/**
 * @brief Applies the effects of @a node to @a blockchain and @a transactionState
 *
 * @param blockchain Pointer to the Blockchain
 * @param transactionState Pointer to the TransactionState
 * @param node Pointer to the ChainNode to apply
 * @param me The ID of the caller node
 * @param statsState Pointer to the StatsState
 */
void applyChainNode(struct Blockchain *blockchain, struct TransactionState *transactionState, struct ChainNode *node, node_id_t me, struct StatsState *statsState);

/**
 * @brief Reverts the effects of applying @a node from @a blockchainState and @a transactionState
 *
 * @param blockchain Pointer to the Blockchain
 * @param transactionState Pointer to the TransactionState
 * @param node Pointer to the ChainNode to revert
 * @param me The ID of the caller node
 * @param statsState Pointer to the StatsState
 */
void revertAppliedChainNode(struct Blockchain *blockchain, struct TransactionState *transactionState, struct ChainNode *node, node_id_t me, struct StatsState *statsState);

#include "Block.h"
#include "util.h"
#include "Attacks.h"
#include "Statistics.h"
#include <string.h>

// Cumulative hash power of the honest nodes
struct SharedHashPower totalHashPower = {.hashpower = 0};

const struct ChainNode genesis_block = {
        .miner = NODE_ID_MAX,
        .parentMinerId = NODE_ID_MAX,
        .transactionData = NULL,
        .timestamp = -1.0,
        .score = 0,
        .height = 0,
        .flags = 0
};

#define setOrphan(ChainNode_ptr) ((ChainNode_ptr)->flags |= CHAIN_NODE_FLAG_ORPHAN)
#define unOrphan(ChainNode_ptr) ((ChainNode_ptr)->flags &= !CHAIN_NODE_FLAG_ORPHAN)

void printChainNode(const struct ChainNode *node);
void sprintChainNode(const struct ChainNode *node, char *outbuf);
void printChainLevel(const struct ChainLevel *level);
void sprintChainLevel(const struct ChainLevel *level, char *outbuf);
void printChain(const struct Blockchain *chain);
void sprintChain(const struct Blockchain *chain, char *outbuf);

extern struct ChainLevel *getChainLevel(const struct Blockchain *blockchain, size_t height);

inline struct ChainLevel *getChainLevel(const struct Blockchain *blockchain, size_t height) {
    if (height < blockchain->min_height) {
        fprintf(stderr, "getChainLevel - Height %lu is below minimum height %lu.\n", height, blockchain->min_height);
        abort();
    }
#ifndef NDEBUG
    if (height > blockchain->min_height + 2 * DEPTH_TO_KEEP) {
        fprintf(stderr, "getChainLevel - Height %lu is above maximum height %lu.\n", height, blockchain->max_height);
        abort();
    }
#endif
    height -= blockchain->min_height;
    if (height < DEPTH_TO_KEEP) {
        return &(blockchain->old_levels[height]);
    } else {
        return &(blockchain->current_levels[height - DEPTH_TO_KEEP]);
    }
}

simtime_t getNextGenDelay(struct rng_t *rng, double hashPowerPortion) {
    simtime_t del = Expent(rng, BLOCK_INTERVAL / hashPowerPortion);
    return del;
}

void initChainLevel(struct ChainLevel *chainLevel) {
    chainLevel->capacity = 0;
    chainLevel->size = 0;
    chainLevel->nodes = NULL;
}

void applyChainNode(struct Blockchain *blockchain, struct TransactionState *transactionState, struct ChainNode *node,
                    node_id_t me, struct StatsState *statsState) {
    if (transactionState)
        applyBlockTransactions(transactionState, node->transactionData);
    blockchain->height = node->height;
    if (statsType == STATS_SELFISH) {
        statsAddBlockSelfish(statsState, node->miner, me);
    } else if (statsType == STATS_FIFTY_ONE) {
        statsAddBlockFiftyOne(statsState, node->miner, me);
    }
}

void
revertAppliedChainNode(struct Blockchain *blockchain, struct TransactionState *transactionState, struct ChainNode *node,
                       node_id_t me, struct StatsState *statsState) {
    if (transactionState)
        revertAppliedBlockTransactions(transactionState, node->transactionData);
    blockchain->height = node->height - 1;
    blockchain->main_chain_index = node->parent_index;
    if (statsType == STATS_SELFISH) {
        statsRemoveBlockSelfish(statsState, node->miner, me);
    } else if (statsType == STATS_FIFTY_ONE) {
        statsRemoveBlockFiftyOne(statsState, node->miner, me);
    }
}

/**
 * @brief Allocates and populates one of the ChainLevel arrays used by Blockchain
 *
 * @return pointer to the newly created ChainLevel array
 */
struct ChainLevel *generateChainLevels() {
    struct ChainLevel *levels = rs_malloc(DEPTH_TO_KEEP * sizeof(struct ChainLevel));
    for (int i = 0; i < DEPTH_TO_KEEP; i++) {
        initChainLevel(&levels[i]);
    }
    return levels;
}

struct ChainNode *
addBlock(simtime_t now, struct Blockchain *chain, struct TransactionState *transactionState, const struct Block *block, node_id_t me, struct StatsState *statsState);

void initBlockchain(struct Blockchain *chain) {
    chain->current_levels = generateChainLevels();
    chain->old_levels = generateChainLevels();

    chain->main_chain_index = 0;
    chain->min_height = 0;
    chain->height = 0;
    chain->max_height = 0;

    struct ChainLevel *level = getChainLevel(chain, 0);
    level->capacity = 1;
    level->size = 1;
    level->nodes = rs_malloc(sizeof(struct ChainNode));
    level->nodes[0] = genesis_block;
}

void initBlockchainState(struct BlockchainState *state, struct rng_t *rng) {
    initBlockchain(&state->chain);
    state->mined_by_me = 0;

    state->miningState.hashPower = (uint_fast64_t) NormalExpanded(rng, 5000, 1000);
    atomic_fetch_add_explicit(&(totalHashPower.hashpower_atomic), state->miningState.hashPower, memory_order_relaxed);
}

void attackerInitBlockchainState(struct BlockchainState *state, struct rng_t *rng) {
    initBlockchain(&state->chain);
    state->mined_by_me = 0;
}

void afterInitBlockchainState(struct BlockchainState *state) {
    state->miningState.hashPowerPortion = (double) state->miningState.hashPower / (double) totalHashPower.hashpower;

    // If simulating an attack, scale the hash power portion
    if (attackConfig.type == ATTACK_SELFISH_MINING) {
        state->miningState.hashPowerPortion *= 1 - attackConfig.selfish.hashPowerPortion;
    } else if (attackConfig.type == ATTACK_FIFTY_ONE) {
        state->miningState.hashPowerPortion *= 1 - attackConfig.fiftyOne.hashPowerPortion;
    }
}

void attackerAfterInitBlockchainState(struct BlockchainState *state) {
    if (attackConfig.type == ATTACK_NONE) {
        fprintf(stderr, "attackerAfterInitBlockchainState - No attack configured\n");
        abort();
    }

    if (attackConfig.type == ATTACK_SELFISH_MINING) {
        state->miningState.hashPowerPortion = attackConfig.selfish.hashPowerPortion;
    } else if (attackConfig.type == ATTACK_FIFTY_ONE) {
        state->miningState.hashPowerPortion = attackConfig.fiftyOne.hashPowerPortion;
    } else {
        state->miningState.hashPowerPortion = 0.0;
    }
}

void scheduleNextBlockGeneration(simtime_t now, struct rng_t *rng, struct BlockchainState *state) {
    if (state->miningState.hashPowerPortion <= 0.0) return;

    simtime_t next_gen_time = now + getNextGenDelay(rng, state->miningState.hashPowerPortion);
    // A block will be generated when extracting this event.
    ScheduleRetractableEvent(next_gen_time);
}

extern struct ChainNode *getChainNode(const struct Blockchain *blockchain, size_t height, size_t index);

inline struct ChainNode *getChainNode(const struct Blockchain *blockchain, size_t height, size_t index) {
    return &(getChainLevel(blockchain, height)->nodes[index]);
}

inline struct ChainNode *getMainChain(const struct Blockchain *blockchain) {
    return getChainNode(blockchain, blockchain->height, blockchain->main_chain_index);
}

simtime_t validateBlock(const struct Block *block, bool *is_valid) {
    (void) block;
    *is_valid = true;
    return BLOCK_VALIDATION_TIME;
}

/**
 * @brief Populates a ChainNode starting from a Block
 *
 * @param block the starting Block
 * @param chainNode the ChainNode to populate
 */
void populateChainNode(const struct Block *block, struct ChainNode *chainNode) {
    chainNode->parentMinerId = block->prevBlockMiner; // Populated to miner ID of parent block
    chainNode->timestamp = block->timestamp;
    chainNode->miner = block->miner;
    chainNode->flags = 0;
    chainNode->height = block->height; // For maintenance purposes
    chainNode->score = 0;
    chainNode->ancestorsMined = 0;

    size_t transaction_data_size = sizeofTransactionData(&block->transactionData);
    chainNode->transactionData = rs_malloc(transaction_data_size);
    memcpy(chainNode->transactionData, &block->transactionData, transaction_data_size);
}

/**
 * @brief Creates and populates a Block starting from a ChainNode
 *
 * @warning Caller owns the returned pointer and must free it when done
 *
 * @param chain the Blockchain
 * @param node the starting Node
 *
 * @returns pointer to the newly created and populated Block
 */
struct Block *blockFromNode(const struct Blockchain *chain, const struct ChainNode *node) {
    size_t block_mem_size = sizeof(struct Block);
    struct TransactionData *txn_data = node->transactionData;
    block_mem_size += sizeofAdditionalTransactionDataBuffer(txn_data->high - txn_data->low);

    struct Block *block = malloc(block_mem_size);
    if (isOrphan(node)) { // Bit of coupling -> Todo: Introduce function getParentsMiner
        block->prevBlockMiner = node->parentMinerId;
    } else {
        struct ChainNode *parent = getChainNode(chain, node->height - 1, node->parent_index);
        block->prevBlockMiner = parent->miner;
    }
    block->timestamp = node->timestamp;
    block->miner = node->miner;
    block->height = node->height;
    block->size = 10; // TODO: Actually compute size
    block->is_attack_block = false;
    size_t transaction_data_size = sizeofTransactionData(node->transactionData);
    memcpy(&block->transactionData, node->transactionData, transaction_data_size);
    return block;
}

static inline struct ChainNode *chainNodeMaxHonest(struct ChainNode *nodeA, struct ChainNode *nodeB) {
    if (nodeA->score > nodeB->score) return nodeA;
    if (nodeA->score < nodeB->score) return nodeB;
    // Same score
    if (nodeA->ancestorsMined > nodeB->ancestorsMined) return nodeA;
    if (nodeA->ancestorsMined < nodeB->ancestorsMined) return nodeB;
    // Same number of ancestors mined
    if (nodeA->timestamp < nodeB->timestamp) return nodeA;
    if (nodeA->timestamp > nodeB->timestamp) return nodeB;
    // Same timestamp
    return (nodeA->miner <= nodeB->miner) ? nodeA : nodeB;
}

static inline struct ChainNode *chainNodeMaxAttacker(struct ChainNode *nodeA, struct ChainNode *nodeB) {
    // Give priority to number of ancestors mined personally, up to "attackConfig.fiftyOne.catchupTolerance"
    if (nodeA->ancestorsMined == nodeB->ancestorsMined) {
        return chainNodeMaxHonest(nodeA, nodeB);
    }

    if (nodeA->ancestorsMined < nodeB->ancestorsMined) {
        struct ChainNode *tmp = nodeA;
        nodeA = nodeB;
        nodeB = tmp;
    }

    // nodeA has more ancestors mined
    if (nodeA->score >= nodeB->score) {
        return nodeA;
    }

    size_t tolerance = 0;
    // nodeA has more ancestors mined, but nodeB has more score
    switch (attackConfig.type) {
        case ATTACK_FIFTY_ONE: {
            tolerance = attackConfig.fiftyOne.catchupTolerance;
            break;
        }
        case ATTACK_SELFISH_MINING: {
            tolerance = attackConfig.selfish.catchupTolerance;
            break;
        }
        default:
            fprintf(stderr, "chainNodeMaxAttacker - Invalid attack type %d\n", attackConfig.type);
            exit(EXIT_FAILURE);
    }

    size_t virtual_score = nodeA->score + tolerance;
    if (virtual_score > nodeB->score) return nodeA;
    if (virtual_score < nodeB->score) return nodeB;

    if (nodeA->timestamp < nodeB->timestamp) return nodeA;
    if (nodeA->timestamp > nodeB->timestamp) return nodeB;

    return (nodeA->miner <= nodeB->miner) ? nodeA : nodeB;
}

/**
 * @brief Selects the ChainNode with the best score
 *
 * @param nodeA first ChainNode to compare
 * @param nodeB second ChainNode to compare
 *
 * @return pointer to the ChainNode with the best score
 *
 * In case of even score, ancestorsMined is used. Then the timestamp is used. Then the miner ID is used.
 */
struct ChainNode *chainNodeMax(struct ChainNode *nodeA, struct ChainNode *nodeB) {
    if (is_attacker(currentNode)) {
        return chainNodeMaxAttacker(nodeA, nodeB);
    } else {
        return chainNodeMaxHonest(nodeA, nodeB);
    }
}

/**
 * @brief Links all orphans related to block 'parent'
 *
 * @param[in,out] chain the blockchain
 * @param parent ChainNode to un-orphan the children of
 * @param parent_index Displacement of the parent ChainNode in its own ChainLevel
 * @param child_height height at which to look for orphans
 * @param[out] orphan_index index of the found orphan inside its own ChainLevel
 *
 * @return pointer to the child with the best score
 */
struct ChainNode *
unorphanBlocks(struct Blockchain *chain, struct ChainNode *parent, size_t parent_index, size_t child_height,
               size_t *orphan_index) {
    if (child_height > chain->max_height) return NULL;

    // A node might be the last before a fork. So out of the various chains it gave rise to, we have to select the best.
    struct ChainNode *bestChild = NULL;
    struct ChainLevel *level = getChainLevel(chain, child_height);

    for (size_t i = 0; i < level->size; i++) {
        struct ChainNode *orphanNode = &(level->nodes[i]);

        if (!isOrphan(orphanNode)) {
            continue;
        }

        if (orphanNode->parentMinerId == parent->miner) { // It IS a child
            unOrphan(orphanNode);
            orphanNode->parent_index = parent_index;
            orphanNode->ancestorsMined = parent->ancestorsMined;
            orphanNode->score = parent->score + 1; // TODO use function for score update
            if (!bestChild) {
                bestChild = orphanNode;
                *orphan_index = i;
            } else {
                bestChild = chainNodeMax(bestChild, orphanNode);
                if (bestChild == orphanNode) {
                    *orphan_index = i;
                }
            }

            size_t child_index = 0;
            struct ChainNode *aux = unorphanBlocks(chain, orphanNode, i, child_height + 1, &child_index);
            if (aux) {
                bestChild = chainNodeMax(bestChild, aux);
                if (bestChild == aux) {
                    *orphan_index = child_index;
                }
            }
        }
    }

    return bestChild;
}

#define resize_array(arr, sz)                                                                       \
do {                                                                                                \
    (arr) = realloc((arr), sizeof(*(arr)) * (sz));                                                  \
    if (!(arr)) {                                                                                   \
        fprintf(stderr, "Failed to allocate memory of size %ld.\n", sz);                            \
        abort();                                                                                    \
    }                                                                                               \
} while(0);

#define resize_array_managed(arr, sz)                                                               \
do {                                                                                                \
    (arr) = rs_realloc((arr), sizeof(*(arr)) * (sz));                                               \
    if (!(arr)) {                                                                                   \
        fprintf(stderr, "Failed to allocate memory of size %ld.\n", sz);                            \
        abort();                                                                                    \
    }                                                                                               \
} while(0);

/**
 * @brief Actually performs the switch from the old main chain to a new one
 *
 * @param[in,out] chain the blockchain
 * @param[in,out] transactionState the state for transactions
 * @param new_chain_node last node of the new chain
 * @param new_chain_index index of new_chain_node inside its own ChainLevel
 * @param me ID of the node
 * @param statsState the state for statistics
 *
 * The chain ending in newChainNode becomes the main chain.
 * This takes care of de-applying changes made by the old chain, and applying the effects of the new chain.
 */
void switchChains(struct Blockchain *chain, struct TransactionState *transactionState, struct ChainNode *new_chain_node,
                  size_t new_chain_index, node_id_t me, struct StatsState *statsState) {
    // 1. Find common ancestor
    size_t buf_sz = new_chain_node->height == chain->height ? 1 : new_chain_node->height > chain->height ?
                                                                  new_chain_node->height - chain->height :
                                                                  chain->height - new_chain_node->height;
    struct ChainNode **to_apply = malloc(sizeof(struct ChainNode *) * buf_sz);
    size_t next_i = 0;

    struct ChainNode *chain_walker = new_chain_node;
    while (chain_walker->height > chain->height) {
        if (buf_sz <= next_i) {
            // Grow the chain
            buf_sz *= 2;
            resize_array(to_apply, buf_sz);
        }
        to_apply[next_i] = chain_walker;
        chain_walker = getChainNode(chain, chain_walker->height - 1, chain_walker->parent_index);
        next_i++;
    }

    // Walk back on main chain until same height as new chain
    struct ChainNode *main_chain_node = getMainChain(chain);
    while (main_chain_node->height > new_chain_node->height) {
        revertAppliedChainNode(chain, transactionState, main_chain_node, me, statsState);
        main_chain_node = getChainNode(chain, main_chain_node->height - 1, main_chain_node->parent_index);
    }

    // Now both ChainNodes are at the same height. Just go back until we find the common parent
    while (main_chain_node != chain_walker) {
        // Walk back main chain
        revertAppliedChainNode(chain, transactionState, main_chain_node, me, statsState);
        main_chain_node = getChainNode(chain, main_chain_node->height - 1, main_chain_node->parent_index);

        // Walk back new chain
        if (buf_sz <= next_i) {
            // Grow the chain
            buf_sz *= 2;
            resize_array(to_apply, buf_sz);
        }
        to_apply[next_i] = chain_walker;
        chain_walker = getChainNode(chain, chain_walker->height - 1, chain_walker->parent_index);
        next_i++;
    }

    // We got to the same height. Apply all nodes on new chain
    for (size_t i = next_i; i > 0; i--) {
        applyChainNode(chain, transactionState, to_apply[i - 1], me, statsState);
    }

    chain->main_chain_index = new_chain_index;
    free(to_apply);
}

/**
 * @brief Checks whether the new chain is better than the old main chain. In which case, the new chain becomes main.
 *
 * @param[in,out] chain the blockchain
 * @param[in,out] transactionState the state portion for transaction management
 * @param new_chain_node last node of another chain
 * @param new_chain_index index of new_chain_node inside its own ChainLevel
 * @param me ID of the node
 * @param statsState the state portion for statistics
 *
 * If new_chain_node's score is better than that of the current main chain, the chain ending in new_chain_node becomes the
 * main chain. This takes care of de-applying changes made by the old chain, and applying the effects of the new chain.
 */
void
maybeSwitchChains(struct Blockchain *chain, struct TransactionState *transactionState, struct ChainNode *new_chain_node,
                  size_t new_chain_index, node_id_t me, struct StatsState *statsState) {
    if (new_chain_node == chainNodeMax(getMainChain(chain), new_chain_node)) {
        switchChains(chain, transactionState, new_chain_node, new_chain_index, me, statsState);
    }
}

/**
 * @brief Takes a ChainLevel[] and resets each of them to be virtually empty
 *
 * @param[in,out] levels the ChainLevel array
 *
 * @warning The ChainLevel s are not emptied physically, just virtually. They still hold old data
 */
void resetLevels(struct ChainLevel *levels) {
    for (int i = 0; i < DEPTH_TO_KEEP; i++) {
        struct ChainLevel *l = &levels[i];
        for (size_t j = 0; j < l->size; j++) { // Iterate up to l->size otherwise we double free
            struct ChainNode *node = &l->nodes[j];
            rs_free(node->transactionData);
            //node->transactionData = NULL; // Not really needed
        }
        l->size = 0;
    }
}

/**
 * @brief Moves @a chain one step forward by emptying @a old_levels and swapping @a current_levels and @a old_levels
 *
 * @param[in,out] chain the Blockchain to move forward
 *
 * Empties @a chain->old_levels and swaps it with @a chain->current_levels. Also updates auxiliary fields
 */
void moveChainForward(struct Blockchain *chain) {
    resetLevels(chain->old_levels);
    struct ChainLevel *aux = chain->old_levels;
    chain->old_levels = chain->current_levels;
    chain->current_levels = aux;
    chain->min_height += DEPTH_TO_KEEP;
}

struct Block *retrieveBlock(struct Blockchain *chain, node_id_t miner, size_t height) {
    struct ChainNode *node = findChainNode(chain, miner, height);
    if (node) {
        return blockFromNode(chain, node);
    }
    return NULL;
}

struct ChainNode *findChainNode(struct Blockchain *chain, node_id_t miner, size_t height) {
    if (height > chain->max_height) return NULL;
    struct ChainLevel *level = getChainLevel(chain, height);
    for (size_t i = 0; i < level->size; i++) {
        struct ChainNode *node = &level->nodes[i];
        if (node->miner == miner) {
            return node;
        }
    }
    return NULL;
}

/**
 * @brief Adds a block to the local chain
 *
 * @param[in,out] chain the blockchain
 * @param[in,out] transactionState the state portion for transaction management
 * @param block block to un-orphan the children of
 * @param me ID of the node
 * @param statsState the state portion for statistics
 *
 * @return a ChainNode pointer, representation of the block as part of the local chain
 *
 * Adds a block to the local chain by creating space for an additional ChainNode at the correct level and populating it.
 */
struct ChainNode *addBlock(simtime_t now, struct Blockchain *chain, struct TransactionState *transactionState,
                           const struct Block *block, node_id_t me, struct StatsState *statsState) {
    if (block->height > chain->max_height) {
        chain->max_height = block->height;
        // See whether there the chain needs to move forward to make space
        if (block->height >= chain->min_height + 2 * DEPTH_TO_KEEP) {
            moveChainForward(chain);
        }
    }

    // Get the chain level
    struct ChainLevel *chainLevel = getChainLevel(chain, block->height);

    // Maybe grow the ChainLevel
    if (chainLevel->size >= chainLevel->capacity) {
        chainLevel->capacity++;
        size_t sz = sizeof(struct ChainNode) * chainLevel->capacity;
        struct ChainNode *aux = rs_realloc(chainLevel->nodes, sz); // Am I reallocating genesis block's node or smth?
        if (!aux) {
            fprintf(stderr, "addBlock - Failed to allocate memory of size %ld for ChainNodes.\n", sz);
            abort();
        }
        chainLevel->nodes = aux;
    }

    // Add the block to the level
    size_t chain_node_index = chainLevel->size;
    struct ChainNode *chainNode = &(chainLevel->nodes[chain_node_index]);
    populateChainNode(block, chainNode);
    chainNode->timestamp = now;
    chainLevel->size++;

    // Seek the parent
    // Is the parent in the previous level and not an orphan itself? Then set parent pointer, otherwise set orphan flag
    struct ChainLevel *parentLevel = getChainLevel(chain, block->height - 1);
    bool found = false;
    for (size_t i = 0; i < parentLevel->size; i++) {
        struct ChainNode *maybe_parent = &parentLevel->nodes[i];
        if (maybe_parent->miner == chainNode->parentMinerId) {
            if (isOrphan(maybe_parent)) break;
            chainNode->parent_index = i;
            chainNode->ancestorsMined = maybe_parent->ancestorsMined;
            chainNode->score = maybe_parent->score + 1; // TODO use function for score update
            found = true;
            break;
        }
    }
    if (!found) {
        setOrphan(chainNode);
        return chainNode;
    }

    size_t best_orphan_index = 0;
    // Unorphan any child that was dangling. Save the best scoring child
    struct ChainNode *unorphaned = unorphanBlocks(chain, chainNode, chain_node_index, block->height + 1,
                                                  &best_orphan_index);
    struct ChainNode *best = chainNode;
    if (unorphaned) {
        best = chainNodeMax(chainNode, unorphaned);
        if (best == unorphaned) {
            chain_node_index = best_orphan_index;
        }
    }

    // Try switching chains. If chainNode is the new best chain, the method will take care of unapplying blocks now out
    // of the main chain (if any) and reapply the blocks
    maybeSwitchChains(chain, transactionState, best, chain_node_index, me, statsState);

    return chainNode;
}

struct Block *
generateBlock(node_id_t me, simtime_t now, struct BlockchainState *state, struct TransactionState *transactionState, struct StatsState *statsState) {

    size_t block_mem_size = sizeof(struct Block);
    struct TransactionData *txn_data = NULL;
    if (transactionState) {
        txn_data = generateTransactionData(transactionState, now, me);
        if (txn_data) {
            block_mem_size += sizeofAdditionalTransactionDataBuffer(txn_data->high - txn_data->low);
        }
    }

    struct Block *b = rs_malloc(block_mem_size);
    b->timestamp = now;
    b->size = 10; // TODO: actually use a size
    b->miner = me;
    b->sender = me;
    struct ChainNode *mainChain = getMainChain(&state->chain);
    b->prevBlockMiner = mainChain->miner;
    b->height = mainChain->height + 1;
    b->is_attack_block = false;

    if (transactionState && txn_data) {
        memcpy(&b->transactionData, txn_data, sizeofTransactionData(txn_data));
        rs_free(txn_data);
    } else {
        memset(&b->transactionData, 0, sizeof(struct TransactionData));
    }

    state->mined_by_me++;

    struct ChainNode *newNode = addBlock(now, &state->chain, transactionState, b, me, statsState);
    newNode->ancestorsMined++; // Copied from parent, increase as we mined this block, too
    return b;
}

simtime_t receiveBlock(simtime_t now, struct BlockchainState *state, struct TransactionState *transactionState,
                       const struct Block *block, node_id_t me, struct StatsState *statsState, bool *updatedMainChain, bool *foundParent) {
    bool valid;
    simtime_t elapsed = validateBlock(block, &valid);
    if (valid) {
        size_t old_index = state->chain.main_chain_index;
        size_t old_height = state->chain.height;

        struct ChainNode *node = addBlock(now, &state->chain, transactionState, block, me, statsState);

        if (state->chain.height != old_height || state->chain.main_chain_index != old_index) {
            *updatedMainChain = true;
        }

        *foundParent = !isOrphan(node);
    }
    return elapsed;
}

void deinitBlockchainState(struct BlockchainState *state) {
    deinitBlockChain(&(state->chain));
}

void deinitChainNode(struct ChainNode *node) {
    rs_free(node->transactionData);
}

void deinitChainLevel(struct ChainLevel *level) {
    for (size_t i = 0; i < level->size; i++)
        deinitChainNode(&level->nodes[i]);
    rs_free(level->nodes);
}

void deinitBlockChain(struct Blockchain *chain) {
    for (size_t i = 0; i < DEPTH_TO_KEEP; i++) {
        deinitChainLevel(&chain->current_levels[i]);
        deinitChainLevel(&chain->old_levels[i]);
    }
    rs_free(chain->current_levels);
    rs_free(chain->old_levels);
}

void printChainNode(const struct ChainNode *node) {
    // If orphan, print parentMinerId instead of parent_index
    if (isOrphan(node)) {
        printf("{\n\tparentMinerId %u\n\ttimestamp %lf\n\tminer %u\n\theight %lu\n\tscore %u\n\tflags %d\n}\n",
               node->parentMinerId, node->timestamp, node->miner, node->height, node->score, node->flags);
    } else {
        printf("{\n\tparent_index %lu\n\ttimestamp %lf\n\tminer %u\n\theight %lu\n\tscore %u\n\tancestorsMined %u\n\tflags %d\n}\n",
               node->parent_index, node->timestamp, node->miner, node->height, node->score, node->ancestorsMined, node->flags);
    }
}

void sprintChainNode(const struct ChainNode *node, char *outbuf) {
    // If orphan, print parentMinerId instead of parent_index
    if (isOrphan(node)) {
        sprintf(outbuf,
                "{\"parentMinerId\": %u, \"timestamp\": %lf, \"miner\": %u, \"height\": %lu, \"score\": %u, \"flags\": %d}",
                node->parentMinerId, node->timestamp, node->miner, node->height, node->score, node->flags);
    } else {
        sprintf(outbuf,
                "{\"parent_index\": %lu, \"timestamp\": %lf, \"miner\": %u, \"height\": %lu, \"score\": %u, \"ancestorsMined\": %u, \"flags\": %d}",
                node->parent_index, node->timestamp, node->miner, node->height, node->score, node->ancestorsMined, node->flags);
    }
}

void printChainLevel(const struct ChainLevel *level) {
    printf("size: %lu\n", level->size);
    printf("Nodes:\n");
    for (size_t i = 0; i < level->size; i++) {
        printf("%lu:\n", i);
        printChainNode(&level->nodes[i]);
        printf("\n");
    }
}

void sprintChainLevel(const struct ChainLevel *level, char *outbuf) {
    sprintf(outbuf, "{\"size\":%lu,", level->size);
    outbuf += strlen(outbuf);
    sprintf(outbuf, "\"Nodes\":[");
    outbuf += strlen(outbuf);
    for (size_t i = 0; i < level->size; i++) {
        sprintChainNode(&level->nodes[i], outbuf);
        outbuf += strlen(outbuf);
        sprintf(outbuf, ",");
        outbuf += strlen(outbuf);
    }
    if (level->size)
        outbuf -= 1;
    outbuf[0] = ']';
    outbuf[1] = '}';
    outbuf[2] = '\0';
}

void printChain(const struct Blockchain *chain) {
    printf("\nmain_chain_index\t%lu\nheight\t%lu\nmax_height\t%lu\n", chain->main_chain_index, chain->height,
           chain->max_height);
    printf("Levels:\n");
    for (size_t i = chain->min_height; i < chain->max_height; i++) {
        printf("[%lu] ", i);
        printChainLevel(getChainLevel(chain, i));
    }
}

void sprintChain(const struct Blockchain *chain, char *outbuf) {
    sprintf(outbuf, "{\"main_chain_index\":%lu,\"height\":%lu,\"max_height\":%lu,", chain->main_chain_index,
            chain->height, chain->max_height);
    outbuf += strlen(outbuf);
    sprintf(outbuf, "\"Levels\":[");
    outbuf += strlen(outbuf);
    for (size_t i = chain->min_height; i < chain->max_height; i++) {
        sprintChainLevel(getChainLevel(chain, i), outbuf);
        outbuf += strlen(outbuf);
        sprintf(outbuf, ",");
        outbuf += strlen(outbuf);
    }
    if (chain->max_height)
        outbuf -= 1;
    outbuf[0] = ']';
    outbuf[1] = '}';
    outbuf[2] = '\0';
}

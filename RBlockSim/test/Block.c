#include "../src/Block.h"
#include <ROOT-Sim/random.h>
#include <assert.h>
#include <string.h>

#define isOrphan(ChainNode_ptr) ((ChainNode_ptr)->flags & CHAIN_NODE_FLAG_ORPHAN)

extern struct ChainNode genesis_block;

extern void printChainNode(struct ChainNode *node);

extern void sprintChainNode(struct ChainNode *node, char *outbuf);

extern void printChainLevel(struct ChainLevel *level);

extern void sprintChainLevel(struct ChainLevel *level, char *outbuf);

extern void printChain(struct Blockchain *chain);

extern void sprintChain(struct Blockchain *chain, char *outbuf);

extern simtime_t getNextGenDelay(struct rng_t *rng, double hashPowerPortion);

extern void initChainLevel(struct ChainLevel *chainLevel);

extern void
applyChainNode(struct Blockchain *blockchain, struct TransactionState *transactionState, struct ChainNode *node);

extern void revertAppliedChainNode(struct Blockchain *blockchain, struct TransactionState *transactionState,
                                   struct ChainNode *node);

extern void initBlockchain(struct Blockchain *chain);

extern void initBlockchainState(struct BlockchainState *state, struct rng_t *rng);

extern void afterInitBlockchainState(struct BlockchainState *state);

extern void scheduleNextBlockGeneration(simtime_t now, struct rng_t *rng, struct BlockchainState *state);

extern struct ChainNode *getMainChain(const struct Blockchain *blockchain);

extern struct ChainNode *getChainNode(const struct Blockchain *blockchain, size_t height, size_t index);

extern simtime_t validateBlock(const struct Block *block, bool *is_valid);

extern void populateChainNode(const struct Block *block, struct ChainNode *chainNode);

extern struct ChainNode *chainNodeMax(struct ChainNode *nodeA, struct ChainNode *nodeB);

extern struct ChainNode *
unorphanBlocks(struct Blockchain *chain, struct ChainNode *parent, size_t parent_index, size_t child_height,
               size_t *orphan_index);

extern void
switchChains(struct Blockchain *chain, struct TransactionState *transactionState, struct ChainNode *new_chain_node,
             size_t new_chain_index);

extern void
maybeSwitchChains(struct Blockchain *chain, struct TransactionState *transactionState, struct ChainNode *new_chain_node,
                  size_t new_chain_index);

extern struct ChainNode *addBlock(simtime_t now, struct Blockchain *chain, struct TransactionState *transactionState,
                                  const struct Block *block);

extern struct Block *
generateBlock(node_id_t me, simtime_t now, struct BlockchainState *state, struct TransactionState *transactionState);

extern simtime_t receiveBlock(simtime_t now, struct BlockchainState *state, struct TransactionState *transactionState,
                              const struct Block *block, bool *updatedMainChain, bool *foundParent);

extern void deinitBlockchainState(struct BlockchainState *state);

extern void deinitChainNode(struct ChainNode *node);

extern void deinitChainLevel(struct ChainLevel *level);

extern void deinitBlockChain(struct Blockchain *chain);

extern struct ChainLevel *getChainLevel(const struct Blockchain *blockchain, size_t height);

struct rng_t *init_rng() {
    struct rng_t *rng = malloc(sizeof(struct rng_t));
    initialize_stream(0, rng);
    return rng;
}

bool chainNodeEquals(struct ChainNode *a, struct ChainNode *b) {
    return a->flags == b->flags && a->score == b->score && a->height == b->height && a->miner == b->miner &&
           a->transactionData == b->transactionData && a->timestamp == b->timestamp &&
           a->parent_index == b->parent_index;
}

struct ChainNode *
addBlockWrapper(struct Blockchain *chain, struct TransactionState *transactionState, simtime_t timestamp,
                node_id_t prev_block_miner, node_id_t miner,
                size_t height, size_t *index) {
    // Generic block
    struct Block *block = calloc(1, sizeof(struct Block));
    block->timestamp = timestamp;
    block->size = 10;
    block->prevBlockMiner = prev_block_miner;
    block->miner = miner;
    block->height = height;
    struct ChainNode *ptr = addBlock(block->timestamp + 10, chain, transactionState, block);
    struct ChainNode *chainNode = malloc(sizeof(struct ChainNode));
    memcpy(chainNode, ptr, sizeof(struct ChainNode));
    *index = getChainLevel(chain, height)->size - 1;
    free(block);
    return chainNode;
}

void testGetNextGenDelay() {
    printf("Testing getNextGenDelay... ");
    struct rng_t *rng = init_rng();
    getNextGenDelay(rng, 0.5);
    free(rng);
    printf("SUCCESS\n");
}

void testInitChainLevel() {
    printf("Testing initChainLevel... ");
    struct ChainLevel *cl = malloc(sizeof(struct ChainLevel));
    initChainLevel(cl);
    assert(cl->size == 0);
    assert(cl->nodes == NULL);
    free(cl);
    printf("SUCCESS\n");
}

void testInitBlockchain() {
    printf("Testing initBlockchain... ");
    struct Blockchain *chain = malloc(sizeof(struct Blockchain));
    initBlockchain(chain);
    assert(chain->old_levels);
    assert(chain->current_levels);
    assert(chain->old_levels[0].size == 1);
    assert(chain->old_levels[0].nodes != NULL);
    assert(chain->main_chain_index == 0);
    assert(!memcmp(getMainChain(chain), &genesis_block, sizeof(struct ChainNode)));
    assert(chain->height == 0);
    assert(chain->min_height == 0);
    assert(chain->max_height == 0);
    deinitBlockChain(chain);
    free(chain);
    printf("SUCCESS\n");
}

void testInitBlockchainState() {
    printf("Testing initBlockchainState... ");
    struct rng_t *rng = init_rng();
    struct BlockchainState *state = malloc(sizeof(struct BlockchainState));
    initBlockchainState(state, rng);
    assert(state->mined_by_me == 0);
    assert(state->miningState.hashPower > 0);
    assert(state->chain.old_levels);
    assert(state->chain.old_levels[0].size == 1);
    assert(state->chain.old_levels[0].nodes != NULL);
    assert(!memcmp(getMainChain(&state->chain), &genesis_block, sizeof(struct ChainNode)));
    assert(state->chain.main_chain_index == 0);
    assert(state->chain.height == 0);
    assert(state->chain.min_height == 0);
    assert(state->chain.max_height == 0);

    free(rng);
    deinitBlockchainState(state);
    free(state);
    printf("SUCCESS\n");
}

void testGenerateBlock() {
    printf("Testing generateBlock... ");
    struct rng_t *rng = init_rng();
    struct BlockchainState *state = malloc(sizeof(struct BlockchainState));
    initBlockchainState(state, rng);

    struct Block *block = generateBlock(0, 1, state, NULL);
    assert(block->transactionData.low == 0);
    assert(block->transactionData.high == 0);
    assert(block->timestamp == 1);
    assert(block->miner == 0);
    assert(block->height == 1);
    assert(block->prevBlockMiner == genesis_block.miner);
    struct ChainNode *n1 = getChainNode(&state->chain, block->height, 0);
    assert(n1->score == 1);
    assert(n1->ancestorsMined == 1);

    struct Block *block2 = generateBlock(0, 2, state, NULL);
    assert(block2->transactionData.low == 0);
    assert(block2->transactionData.high == 0);
    assert(block2->timestamp == 2);
    assert(block2->miner == 0);
    assert(block2->height == 2);
    assert(block2->prevBlockMiner == block->miner);
    struct ChainNode *n2 = getChainNode(&state->chain, block2->height, 0);
    assert(n2->score == 2);
    assert(n2->ancestorsMined == 2);

    size_t b3_index;
    struct ChainNode *n3 = addBlockWrapper(&state->chain, NULL, 6, block2->miner, 1, 3, &b3_index);
    assert(n3->height == 3);
    assert(!memcmp(getMainChain(&state->chain), n3, sizeof(struct ChainNode))); // New main chain

    struct Block *block4 = generateBlock(0, 9, state, NULL);
    assert(block4->transactionData.low == 0);
    assert(block4->transactionData.high == 0);
    assert(block4->timestamp == 9);
    assert(block4->miner == 0);
    assert(block4->height == 4);
    assert(block4->prevBlockMiner == n3->miner);
    struct ChainNode *n4 = getChainNode(&state->chain, block4->height, 0);
    assert(n4->score == 4);
    assert(n4->ancestorsMined == 3);

    size_t b4b_index;
    struct ChainNode *n4b = addBlockWrapper(&state->chain, NULL, 10, n3->miner, 1, 4, &b4b_index);
    assert(n4b->height == 4);
    assert(memcmp(getMainChain(&state->chain), n4b, sizeof(struct ChainNode)) != 0); // NOT new main chain

    struct Block *block5 = generateBlock(0, 20, state, NULL);
    assert(block5->transactionData.low == 0);
    assert(block5->transactionData.high == 0);
    assert(block5->timestamp == 20);
    assert(block5->miner == 0);
    assert(block5->height == 5);
    assert(block5->prevBlockMiner == block4->miner);
    struct ChainNode n5;
    populateChainNode(block5, &n5);
    struct ChainNode *mc = getMainChain(&state->chain);
    assert(mc->score == 5);
    assert(mc->ancestorsMined == 4);
    // memcmp won't work, as transactionData is reallocated for n5 and thus different
    assert(mc->timestamp == n5.timestamp);
    assert(mc->miner == n5.miner);
    assert(mc->height == n5.height);
    assert(mc->flags == n5.flags);
    assert(mc->transactionData->low == n5.transactionData->low);
    assert(mc->transactionData->high == n5.transactionData->high);

    free(rng);
    free(block);
    free(block2);
    free(block4);
    free(block5);
    deinitBlockchainState(state);
    free(state);
    free(n3);
    free(n4b);
    free(n5.transactionData);
    printf("SUCCESS\n");
}

void refreshChainNode(struct ChainNode *node, struct Blockchain *chain, size_t height, size_t index) {
    memcpy(node, getChainNode(chain, height, index), sizeof(struct ChainNode));
}

void testAddBlock() {
    printf("Testing addBlock... ");
    struct rng_t *rng = init_rng();
    struct BlockchainState *state = malloc(sizeof(struct BlockchainState));
    initBlockchainState(state, rng);
    struct Blockchain *chain = &state->chain;
    size_t s = sizeof(struct ChainNode);

    size_t genesis_block_index = 0;

    // Generic block
    size_t ch1_n1_index;
    struct ChainNode *ch1_n1 = addBlockWrapper(chain, NULL, 1, genesis_block.miner, 0, 1, &ch1_n1_index);
    assert(ch1_n1_index == 0);
    assert(!memcmp(getChainNode(chain, 1, ch1_n1_index), ch1_n1, s));
    assert(!isOrphan(ch1_n1));
    assert(ch1_n1->parent_index == genesis_block_index);
    assert(!memcmp(getMainChain(chain), ch1_n1, s));

    // Chain 1 height 2
    size_t ch1_n2_index;
    struct ChainNode *ch1_n2 = addBlockWrapper(chain, NULL, 2, ch1_n1->miner, 0, ch1_n1->height + 1,
                                               &ch1_n2_index);
    assert(ch1_n2_index == 0);
    assert(!memcmp(getChainNode(chain, 2, ch1_n2_index), ch1_n2, s));
    assert(!isOrphan(ch1_n2));
    assert(ch1_n2->parent_index == ch1_n1_index);
    assert(!memcmp(getMainChain(chain), ch1_n2, s));

    // Orphan at height 2
    size_t ch2_n2_index;
    struct ChainNode *ch2_n2 = addBlockWrapper(chain, NULL, 3, 1, 1, 2, &ch2_n2_index);
    assert(ch2_n2_index == 1);
    assert(!memcmp(getChainNode(chain, 2, ch2_n2_index), ch2_n2, s));
    assert(isOrphan(ch2_n2));
    assert(ch2_n2->parentMinerId == 1);
    assert(!memcmp(getMainChain(chain), ch1_n2, s)); // Main chain is still ch1_n2

    // Child of the orphan at height 2 (aka orphan at height 3)
    size_t ch2_n3_index;
    struct ChainNode *ch2_n3 = addBlockWrapper(chain, NULL, 4, ch2_n2->miner, 2, ch2_n2->height + 1,
                                               &ch2_n3_index);
    assert(ch2_n3_index == 0);
    assert(!memcmp(getChainNode(chain, 3, ch2_n3_index), ch2_n3, s));
    assert(isOrphan(ch2_n2));
    assert(isOrphan(ch2_n3));
    assert(ch2_n3->parentMinerId == ch2_n2->miner);
    assert(!memcmp(getMainChain(chain), ch1_n2, s)); // Main chain is still ch1_n2

    // Another block on mainchain
    size_t ch1_n3_index;
    struct ChainNode *ch1_n3 = addBlockWrapper(chain, NULL, 5, ch1_n2->miner, 5, ch1_n2->height + 1,
                                               &ch1_n3_index);
    assert(ch1_n3_index == 1);
    assert(!memcmp(getChainNode(chain, 3, ch1_n3_index), ch1_n3, s));
    assert(!isOrphan(ch1_n3));
    assert(ch1_n3->parent_index == ch1_n2_index);
    assert(!memcmp(getMainChain(chain), ch1_n3, s)); // Is new main chain

    // Parent of the two orphans. Also, chain2 becomes mainchain
    size_t ch2_n1_index;
    struct ChainNode *ch2_n1 = addBlockWrapper(chain, NULL, 1, genesis_block.miner, ch2_n2->parentMinerId,
                                               ch2_n2->height - 1, &ch2_n1_index);

    refreshChainNode(ch2_n2, chain, 2, ch2_n2_index);
    assert(!isOrphan(ch2_n2));
    refreshChainNode(ch2_n3, chain, 3, ch2_n3_index);
    assert(!isOrphan(ch2_n3));

    assert(ch2_n1_index == 1);
    assert(!memcmp(getChainNode(chain, ch2_n2->height - 1, ch2_n1_index), ch2_n1, s));
    assert(!isOrphan(ch2_n1));
    assert(ch2_n1->parent_index == genesis_block_index);
    assert(memcmp(getMainChain(chain), ch1_n3, s) != 0);
    assert(!memcmp(getMainChain(chain), ch2_n3, s)); // Is new main chain

    // Another chain with unorphaning but NOT becoming main chain
    size_t ch3_n3_index;
    struct ChainNode *ch3_n3 = addBlockWrapper(chain, NULL, 10, 9, 9, 3, &ch3_n3_index);
    assert(ch3_n3_index == 2);
    assert(!memcmp(getChainNode(chain, 3, ch3_n3_index), ch3_n3, s));
    assert(isOrphan(ch3_n3));
    assert(ch3_n3->parentMinerId == 9);
    assert(!memcmp(getMainChain(chain), ch2_n3, s));

    size_t ch3_n2_index;
    struct ChainNode *ch3_n2 = addBlockWrapper(chain, NULL, 3, 9, 9, 2, &ch3_n2_index);
    assert(ch3_n2_index == 2);
    assert(!memcmp(getChainNode(chain, 2, ch3_n2_index), ch3_n2, s));
    assert(isOrphan(ch3_n2));
    assert(isOrphan(ch3_n3));
    assert(ch3_n2->parentMinerId == ch3_n3->miner);
    assert(!memcmp(getMainChain(chain), ch2_n3, s));

    size_t ch3_n1_index;
    struct ChainNode *ch3_n1 = addBlockWrapper(chain, NULL, 1, genesis_block.miner, 9, 1, &ch3_n1_index);
    refreshChainNode(ch3_n2, chain, 2, ch3_n2_index);
    assert(!isOrphan(ch3_n2));
    refreshChainNode(ch3_n3, chain, 3, ch3_n3_index);
    assert(!isOrphan(ch3_n3));

    assert(ch3_n1_index == 2);
    assert(!memcmp(getChainNode(chain, 1, ch3_n1_index), ch3_n1, s));
    assert(!isOrphan(ch3_n1));
    assert(ch3_n1->parent_index == genesis_block_index);
    assert(!memcmp(getMainChain(chain), ch2_n3, s));
    assert(memcmp(getMainChain(chain), ch3_n3, s) != 0); // Is NOT new main chain

    // Chain 3 becomes main chain with fourth node
    size_t ch3_n4_index;
    struct ChainNode *ch3_n4 = addBlockWrapper(chain, NULL, 14, ch3_n3->miner, 9, ch3_n3->height + 1,
                                               &ch3_n4_index);
    assert(ch3_n4_index == 0);
    assert(!memcmp(getChainNode(chain, 4, ch3_n4_index), ch3_n4, s));
    assert(!isOrphan(ch3_n4));
    assert(ch3_n4->parent_index == ch3_n3_index);
    assert(!memcmp(getMainChain(chain), ch3_n4, s)); // Is new main chain

    // Fork of chain 3 from height 3. Not new main chain
    size_t ch3b_n4_index;
    struct ChainNode *ch3b_n4 = addBlockWrapper(chain, NULL, 15, ch3_n3->miner, 9, ch3_n3->height + 1,
                                                &ch3b_n4_index);
    assert(ch3b_n4_index == 1);
    assert(!memcmp(getChainNode(chain, 4, ch3b_n4_index), ch3b_n4, s));
    assert(!isOrphan(ch3b_n4));
    assert(ch3b_n4->parent_index == ch3_n3_index);
    assert(!memcmp(getMainChain(chain), ch3_n4, s));
    assert(memcmp(getMainChain(chain), ch3b_n4, s) != 0); // Is NOT new main chain

    // Fork of chain 3 from height 3. New main chain
    size_t ch3c_n4_index;
    struct ChainNode *ch3c_n4 = addBlockWrapper(chain, NULL, 12, ch3_n3->miner, 9, ch3_n3->height + 1,
                                                &ch3c_n4_index);
    assert(ch3c_n4_index == 2);
    assert(!memcmp(getChainNode(chain, 4, ch3c_n4_index), ch3c_n4, s));
    assert(!isOrphan(ch3c_n4));
    assert(ch3c_n4->parent_index == ch3_n3_index);
    assert(!memcmp(getMainChain(chain), ch3c_n4, s)); // Is new main chain

    for (size_t i = ch3c_n4->height + 1; i < DEPTH_TO_KEEP; i++) {
        size_t index;
        struct ChainNode *node = addBlockWrapper(chain, NULL, 10.0 * (int) i, ch3c_n4->miner, ch3c_n4->miner, i,
                                                 &index);
        assert(index == 0);
        assert(!memcmp(getChainNode(chain, i, index), node, s));
        assert(!isOrphan(node));
        assert(!memcmp(getMainChain(chain), node, s)); // Is new main chain
        assert(chain->min_height == 0);
        assert(chain->height == i);
        assert(chain->max_height == i);
        free(node);
    }

    for (size_t i = DEPTH_TO_KEEP; i < 2 * DEPTH_TO_KEEP; i++) {
        size_t index;
        struct ChainNode *node = addBlockWrapper(chain, NULL, 10.0 * (int) i, ch3c_n4->miner, ch3c_n4->miner, i,
                                                 &index);
        assert(index == 0);
        assert(!memcmp(getChainNode(chain, i, index), node, s));
        assert(!isOrphan(node));
        assert(!memcmp(getMainChain(chain), node, s)); // Is new main chain
        assert(chain->min_height == 0);
        assert(chain->height == i);
        assert(chain->max_height == i);
        free(node);
    }

    // Boundary update
    {
        size_t ht = 2 * DEPTH_TO_KEEP;
        size_t index;
        struct ChainNode *node = addBlockWrapper(chain, NULL, 10 * (int) ht, ch3c_n4->miner, ch3c_n4->miner, ht,
                                                 &index);
        assert(index == 0);
        assert(!memcmp(getChainNode(chain, ht, index), node, s));
        assert(!isOrphan(node));
        assert(!memcmp(getMainChain(chain), node, s)); // Is new main chain
        assert(chain->min_height == DEPTH_TO_KEEP);
        assert(chain->height == ht);
        assert(chain->max_height == ht);
        free(node);
    }

    free(rng);
    free(ch1_n1);
    free(ch1_n2);
    free(ch1_n3);
    free(ch2_n1);
    free(ch2_n2);
    free(ch2_n3);
    free(ch3_n1);
    free(ch3_n2);
    free(ch3_n3);
    free(ch3_n4);
    free(ch3b_n4);
    free(ch3c_n4);
    deinitBlockchainState(state);
    free(state);
    printf("SUCCESS\n");
}

void testApplyChainNode() {
    printf("Testing applyChainNode... ");
    struct rng_t *rng = init_rng();
    struct BlockchainState *state = malloc(sizeof(struct BlockchainState));
    initBlockchainState(state, rng);

    struct Block *block = generateBlock(0, 20, state, NULL);
    assert(block->transactionData.low == 0);
    assert(block->transactionData.high == 0);
    assert(block->timestamp == 20);
    assert(block->miner == 0);
    assert(block->height == 1);
    assert(block->prevBlockMiner == genesis_block.miner);
    struct ChainNode node;
    populateChainNode(block, &node);

    applyChainNode(&state->chain, NULL, &node);
    assert(state->chain.height == block->height);

    free(rng);
    free(block);
    deinitBlockchainState(state);
    free(node.transactionData);
    free(state);
    printf("SUCCESS\n");
}

void testGetChainNode() {
    printf("Testing getChainNode... ");
    struct rng_t *rng = init_rng();
    struct BlockchainState *state = malloc(sizeof(struct BlockchainState));
    initBlockchainState(state, rng);
    struct Blockchain *chain = &state->chain;
    size_t s = sizeof(struct ChainNode);

    assert(!memcmp(getChainNode(chain, 0, 0), &genesis_block, s));
    assert(!memcmp(getChainNode(chain, 0, 0), &chain->old_levels[0].nodes[0], s));

    struct Block *block = generateBlock(0, 20, state, NULL);
    assert(block->transactionData.low == 0);
    assert(block->transactionData.high == 0);
    assert(block->timestamp == 20);
    assert(block->miner == 0);
    assert(block->height == 1);
    assert(block->prevBlockMiner == genesis_block.miner);
    struct ChainNode node_e;
    populateChainNode(block, &node_e);
    node_e.parent_index = 0;

    // Check that the first node is still the genesis block
    assert(!memcmp(getChainNode(chain, 0, 0), &genesis_block, s));
    assert(!memcmp(getChainNode(chain, 0, 0), &chain->old_levels[0].nodes[0], s));
    assert(memcmp(getChainNode(chain, 1, 0), &genesis_block, s));
    assert(!memcmp(getChainNode(chain, 1, 0), &chain->old_levels[1].nodes[0], s));

    // Add a block and add it at position DEPTH_TO_KEEP - 1
    size_t index;
    size_t ht_a = DEPTH_TO_KEEP - 1;
    struct ChainNode *node_a = addBlockWrapper(chain, NULL, 100, 0, 0,
                                               ht_a, &index);
    // Retrieve the created node with getChainNode
    assert(!memcmp(getChainNode(chain, ht_a, index), node_a, s));
    assert(!memcmp(&chain->old_levels[ht_a].nodes[0], node_a, s));
    assert(&chain->old_levels[ht_a].nodes[0] == getChainNode(chain, ht_a, index));

    // Create another block add it to the chain at position DEPTH_TO_KEEP
    size_t ht_b = DEPTH_TO_KEEP;
    struct ChainNode *node_b = addBlockWrapper(chain, NULL, 102, 0, 0, ht_b, &index);
    // Retrieve the created node with getChainNode
    assert(!memcmp(getChainNode(chain, ht_b, 0), node_b, s));
    assert(!memcmp(&chain->current_levels[ht_b - DEPTH_TO_KEEP].nodes[0], node_b, s));
    assert(&chain->current_levels[ht_b - DEPTH_TO_KEEP].nodes[0] == getChainNode(chain, ht_b, 0));

    // Create another block add it to the chain at position 2 * DEPTH_TO_KEEP - 1
    size_t ht_c = 2 * DEPTH_TO_KEEP - 1;
    struct ChainNode *node_c = addBlockWrapper(chain, NULL, 120, 0, 0, ht_c, &index);
    // Retrieve the created node with getChainNode
    assert(!memcmp(getChainNode(chain, ht_c, 0), node_c, s));
    assert(!memcmp(&chain->current_levels[ht_c - DEPTH_TO_KEEP].nodes[0], node_c, s));
    assert(&chain->current_levels[ht_c - DEPTH_TO_KEEP].nodes[0] ==getChainNode(chain, ht_c, 0));

    // Now we trigger the advancement of the buffer by going to 2 * DEPTH_TO_KEEP. Triggers swap of level arrays
    size_t ht_d = 2 * DEPTH_TO_KEEP;
    struct ChainNode *node_d = addBlockWrapper(chain, NULL, 122, 0, 0, ht_d, &index);
    // Retrieve the created node with getChainNode
    assert(!memcmp(getChainNode(chain, ht_d, 0), node_d, s));
    assert(!memcmp(&chain->current_levels[0].nodes[0], node_d, s));
    assert(&chain->current_levels[0].nodes[0] == getChainNode(chain, ht_d, 0));

    // Now node_a is not on the chain anymore. We can compare cause node_a is a copy of the original that was inserted in the chain.
    assert(memcmp(&chain->old_levels[ht_a].nodes[0], node_a, s));
    assert(!memcmp(&chain->current_levels[ht_a].nodes[0], node_a, s)); // node_a is still there! But ignored
    assert(chain->current_levels[ht_a].size == 0);

    // Similarly, check that node_b and node_c are now swapped into old_levels
    assert(!memcmp(getChainNode(chain, ht_b, 0), node_b, s));
    assert(!memcmp(&chain->old_levels[ht_b - DEPTH_TO_KEEP].nodes[0], node_b, s));
    assert(memcmp(&chain->current_levels[ht_b - DEPTH_TO_KEEP].nodes[0], node_b, s));

    assert(!memcmp(getChainNode(chain, ht_c, 0), node_c, s));
    assert(!memcmp(&chain->old_levels[ht_c - DEPTH_TO_KEEP].nodes[0], node_c, s));
    assert(memcmp(&chain->current_levels[ht_c - DEPTH_TO_KEEP].nodes[0], node_c, s));

    free(node_a);
    free(node_b);
    free(node_c);
    free(node_d);
    free(rng);
    free(block);
    free(node_e.transactionData);
    deinitBlockchainState(state);
    free(state);
    printf("SUCCESS\n");
}

void testRevertAppliedChainNode() {
    printf("Testing revertAppliedChainNode... ");

    struct rng_t *rng = init_rng();
    struct BlockchainState *state = malloc(sizeof(struct BlockchainState));
    initBlockchainState(state, rng);

    struct Block *block = generateBlock(0, 20, state, NULL);
    assert(block->transactionData.low == 0);
    assert(block->transactionData.high == 0);
    assert(block->timestamp == 20);
    assert(block->miner == 0);
    assert(block->height == 1);
    assert(block->prevBlockMiner == genesis_block.miner);
    struct ChainNode node;
    populateChainNode(block, &node);
    node.parent_index = 0;

    revertAppliedChainNode(&state->chain, NULL, &node);
    assert(state->chain.height == genesis_block.height);
    assert(chainNodeEquals(getMainChain(&state->chain), &genesis_block));

    applyChainNode(&state->chain, NULL, &node);
    assert(state->chain.height == block->height);

    free(rng);
    free(block);
    free(node.transactionData);
    deinitBlockchainState(state);
    free(state);
    printf("SUCCESS\n");
}

void testSwitchChains() {
    printf("Testing switchChains... ");

    struct rng_t *rng = init_rng();
    struct BlockchainState *state = malloc(sizeof(struct BlockchainState));
    initBlockchainState(state, rng);
    struct Blockchain *chain = &state->chain;

    struct TransactionState transactionState_e;
    struct TransactionState *transactionState = &transactionState_e;
    initTransactionState(transactionState);

    size_t s = sizeof(struct ChainNode);

    //printChain(chain);

    // Until height 3 single chain. Then fork that continues until height 10. Then switch between the two forks
    // Till height 3
    for (size_t i = 1; i < 4; i++) {
        size_t index;
        struct ChainNode *node = addBlockWrapper(chain, transactionState, (double) i * 2, getMainChain(chain)->miner, 0,
                                                 i, &index);
        assert(index == 0);
        assert(!memcmp(getChainNode(chain, i, index), node, s));
        assert(!isOrphan(node));
        assert(!memcmp(getMainChain(chain), node, s));
        free(node);
    }

    assert(chain->height == 3);

    //printChain(chain);

    // Fork at height 4
    for (size_t i = 4; i < 5; i++) {
        size_t index;
        node_id_t miner = getMainChain(chain)->miner;
        struct ChainNode *node = addBlockWrapper(chain, transactionState, (double) i * 2, miner, 0, i, &index);
        assert(index == 0);
        assert(!memcmp(getChainNode(chain, i, index), node, s));
        assert(!isOrphan(node));
        assert(!memcmp(getMainChain(chain), node, s));
        free(node);

        node = addBlockWrapper(chain, transactionState, (double) i * 2, miner, 1, i, &index);
        assert(index == 1);
        assert(!memcmp(getChainNode(chain, i, index), node, s));
        assert(!isOrphan(node));
        free(node);
    }

    //printChain(chain);

    // From height 5 to 10
    for (size_t i = 5; i < 11; i++) {
        fflush(stdout);
        size_t index;
        struct ChainNode *node = addBlockWrapper(chain, transactionState, (double) i * 2, 0, 0, i, &index);
        assert(index == 0);
        assert(!memcmp(getChainNode(chain, i, index), node, s));
        assert(!isOrphan(node));
        assert(!memcmp(getMainChain(chain), node, s));
        free(node);

        node = addBlockWrapper(chain, transactionState, (double) i * 2, 1, 1, i, &index);
        assert(index == 1);
        assert(!memcmp(getChainNode(chain, i, index), node, s));
        assert(!isOrphan(node));
        free(node);
    }


    //printChain(chain);

    /*
    char buf[100000];
    buf[0] = '\0';
    sprintChain(chain, buf);
    */

    // Switching to the same chain and same block should have no effect
    assert(getMainChain(chain) == getChainNode(chain, 10, 0));
    switchChains(chain, transactionState, getMainChain(chain), 0);
    assert(getMainChain(chain) == getChainNode(chain, 10, 0));

    // Switch to another chain
    switchChains(chain, transactionState, getChainNode(chain, 10, 1), 1);
    assert(getMainChain(chain) == getChainNode(chain, 10, 1));

    // Same chain, back by one
    switchChains(chain, transactionState, getChainNode(chain, 9, 1), 1);
    assert(getMainChain(chain) == getChainNode(chain, 9, 1));

    // Same chain, back by 3
    switchChains(chain, transactionState, getChainNode(chain, 6, 1), 1);
    assert(getMainChain(chain) == getChainNode(chain, 6, 1));

    // Forward again
    switchChains(chain, transactionState, getChainNode(chain, 10, 1), 1);
    assert(getMainChain(chain) == getChainNode(chain, 10, 1));

    // Other chain, back by 4
    switchChains(chain, transactionState, getChainNode(chain, 6, 0), 0);
    assert(getMainChain(chain) == getChainNode(chain, 6, 0));

    // Back to last on chain 0
    switchChains(chain, transactionState, getChainNode(chain, 10, 0), 0);
    assert(getMainChain(chain) == getChainNode(chain, 10, 0));

    // Back to complete start
    switchChains(chain, transactionState, getChainNode(chain, 0, 0), 0);
    assert(getMainChain(chain) == getChainNode(chain, 0, 0));

    // Again last on chain 0
    switchChains(chain, transactionState, getChainNode(chain, 10, 0), 0);
    assert(getMainChain(chain) == getChainNode(chain, 10, 0));

    deinitTransactionState(transactionState);
    deinitBlockchainState(state);
    free(state);
    free(rng);
    printf("SUCCESS! BUT TEST NEEDS BETTER GUARANTEES!\n");
    //printf("SUCCESS\n");
}

void testReceiveBlock() {
    printf("Testing receiveBlock... ");

    // Initialize BlockchainState and TransactionState
    struct rng_t *rng = init_rng();
    struct BlockchainState *state = malloc(sizeof(struct BlockchainState));
    initBlockchainState(state, rng);
    struct Blockchain *chain = &state->chain;

    struct TransactionState transactionState;
    initTransactionState(&transactionState);

    assert(TXN_NUMBER > 0);

    // Generate a block
    // Generic block
    struct Block *block = calloc(1, sizeof(struct Block));
    block->timestamp = 1;
    block->size = 10;
    block->prevBlockMiner = genesis_block.miner;
    block->miner = 0;
    block->height = 1;
    block->transactionData.low = 0;
    block->transactionData.high = TXN_DATA_MIN_TXNS;
    bitmap_initialize(block->transactionData.included_transactions, TXN_DATA_MIN_TXNS);
    for (int i = 0; i < TXN_DATA_MIN_TXNS; i += 2) {
        bitmap_set(block->transactionData.included_transactions, i);
    }

    // ReceiveBlock
    bool updated_main_chain = false;
    bool found_parent = false;
    receiveBlock(block->timestamp + 1, state, &transactionState, block, &updated_main_chain, &found_parent);

    // Validate that
    // 1. The block was applied properly
    assert(!memcmp(getMainChain(chain), &chain->old_levels[1].nodes[0], sizeof(struct ChainNode)));
    assert(transactionState.low == 0);
    assert(transactionState.high == block->transactionData.high);
    for (int i = 0; i < TXN_DATA_MIN_TXNS; i += 2) {
        assert(bitmap_check(transactionState.transactions_bitmap, i));
    }
    for (int i = 1; i < TXN_DATA_MIN_TXNS; i += 2) {
        assert(!bitmap_check(transactionState.transactions_bitmap, i));
    }

    // 2. The ChainNode is correctly generated from the Block.
    struct ChainNode *chainNode = getChainNode(chain, 1, 0);
    assert(chainNode->height == block->height);
    assert(chainNode->timestamp == block->timestamp + 1);
    assert(chainNode->miner == block->miner);
    assert(chainNode->parent_index == 0);
    assert(chainNode->score == 1);
    assert(chainNode->flags == 0);
    assert(!memcmp(chainNode->transactionData, &block->transactionData, sizeof(struct TransactionData)));

    free(rng);
    free(block);
    deinitTransactionState(&transactionState);
    deinitBlockchainState(state);
    free(state);
    printf("SUCCESS\n");
}

void block_main() {
    printf("TESTING BLOCK\n");
    testGetNextGenDelay();
    testInitChainLevel();
    testInitBlockchain();
    testInitBlockchainState();
    testGetChainNode();
    testApplyChainNode();
    testRevertAppliedChainNode();
    testSwitchChains();
    testAddBlock();
    testGenerateBlock();
    testReceiveBlock();
    printf("FINISHED TESTING BLOCK, SUCCESS!\n");
}

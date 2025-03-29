#include "RBlockSim.h"
#include "Block.h"
#include "Network.h"
#include "Node.h"
#include "Transaction.h"
#include "Statistics.h"
#include "Attacks.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 * This is the main RBlockSim file
 * */

atomic_uint_fast64_t tot_mined = 0; // Used at end to compute total mined blocks
char old_stats_folder[256] = "results_0/";
char *old_stats_filename = "stats_%07d.json";
char *stats_extension = "_%07d.json";

char single_stats_fullpath[2048] = "";
char single_stats_filename[1024] = "stats_sz%lu_w%lu_bi%lf_a%s_h%lf_c%u_d%u_rng%u.json";
char stats_folder_long[1024] = "Results_sz%lu_w%lu_bi%lf_a%s_h%lf_c%u_d%u_rng%u_%d/";

char single_stats_metadata_fullpath[2048] = "";

__thread node_id_t currentNode = 0;

struct SelfishStatsState *selfishStats = NULL;

char *attack_metadata_filename = "attack_info.json";
char attack_metadata_format[] = "{\n\"attack_type\":\"%s\",\"attacker\":%u,\"attacker_hashpower\":%lf,\"depth\":%u,\"catchup_tolerance\":%u,\"failed_attacks\":%u,\"successful_conceals\":%u\n}\n";

void requestParent(lp_id_t requester, simtime_t request_time, struct NodeState *node_state, const struct Block *block) {
    struct request_block_evt *evt = malloc(sizeof(struct request_block_evt));
    evt->requester = requester;
    evt->miner = block->prevBlockMiner;
    evt->height = block->height - 1;
    ScheduleNewEvent(block->sender, request_time + getTransmissionDelay(requester, block->sender, 0, node_state->rng), REQUEST_BLOCK, evt, sizeof(struct request_block_evt));
    free(evt);
}

/**
 * @brief Propagates a block and N of its ancestors
 *
 * @param chain The Blockchain state
 * @param sender The ID of the node sending the block
 * @param send_time The time at which the block sending starts
 * @param block The Block itself
 * @param n_ancestors The number of ancestors to propagate
 * @param rng Random Number Generator state
 * */
void propagateBlockAndNAncestors(struct Blockchain *chain, node_id_t sender, simtime_t send_time, const struct Block *block, size_t n_ancestors, struct rng_t *rng) {
    // Get the block's ancestors and propagate them
    for (size_t i = 1; i < n_ancestors; i++) {
        size_t height = block->height - n_ancestors + i;
        struct Block *retrieved_block = retrieveBlock(chain, block->miner, height);
        if (!retrieved_block) {
            fprintf(stderr, "Block at height %lu (%luth ancestor of block at height %lu) not found in the chain\n", height, n_ancestors - i, block->height);
            abort();
        }
        retrieved_block->sender = sender;
        propagateBlock(sender, send_time, retrieved_block, rng);
        free(retrieved_block);
        send_time += 0.002;
    }
    propagateBlock(sender, send_time, block, rng);
}

void ProcessEvent(lp_id_t me, simtime_t now, unsigned event_type, const void *event_content, unsigned event_size,
                  void *v_state) {
    struct NodeState *state = (struct NodeState *) v_state;
    struct AttackerNodeState *attackerState = is_attacker(me) ? (struct AttackerNodeState *) v_state : NULL;
    currentNode = me;

    if (now > conf.termination_time && event_type != LP_FINI) {
        return;
    }

    switch (event_type) {
        case LP_INIT: {
            struct NodeState *new_state;
            if (is_attacker(me)) {
                new_state = rs_malloc(sizeof(struct AttackerNodeState));
                struct AttackerNodeState *new_state_attacker = (struct AttackerNodeState *) new_state;
                new_state_attacker->last_propagated_height = 0;
                new_state_attacker->isSelfishMining = false;
                new_state_attacker->finishedSelfishMining = false;
                new_state_attacker->failed_attacks = 0;
                new_state_attacker->successful_conceals = 0;
            } else {
                new_state = rs_malloc(sizeof(struct NodeState));
            }

            // Init RNG
            new_state->rng = rs_malloc(sizeof(struct rng_t));
            initialize_stream(RNG_SEED + me, new_state->rng);

            //printf("[N %lu] INIT - HashPower %lu\tTotalHP %lu\n", me, new_state->hashPower, totalHashPower.hashpower_atomic);

            initNetwork();
            if (is_attacker(me)) {
                attackerInitBlockchainState(&new_state->blockchainState, new_state->rng);
            } else {
                initBlockchainState(&new_state->blockchainState, new_state->rng);
            }
            initTransactionState(&new_state->transactionState);

            switch (statsType) {
                case STATS_NONE:
                    break;
                case STATS_DETAILED:
                    initDetailedStatisticsState(&new_state->statsState);
                    break;
                case STATS_SELFISH:
                    initSelfishStatisticsState(&new_state->statsState);
                    break;
                case STATS_FIFTY_ONE:
                    initFiftyOneStatisticsState(&new_state->statsState);
                    break;
                default:
                    perror("Unknown statistics type");
                    exit(1);
            }

            SetState(new_state);

            // Generate internal init event
            ScheduleNewEvent(me, now, RBLOCKSIM_INIT, NULL, 0);
            return;
        }
        case LP_FINI: {
            atomic_fetch_add_explicit(&(tot_mined), state->blockchainState.mined_by_me, memory_order_relaxed);
            if (me == N_NODES - 1) {
                printf("Total mined blocks: %lu\n", tot_mined); // This does not work because not all other LPs are guaranteed to have finished
                printf("Height: %lu\n", state->blockchainState.chain.height);
            }

            if (is_attacker(me) && (attackConfig.type == ATTACK_FIFTY_ONE || attackConfig.type == ATTACK_SELFISH_MINING)) {
                // Print attack metadata in json format
                char filename[2048];
                strcpy(filename, single_stats_metadata_fullpath);
                sprintf(filename + strlen(filename), "%s", attack_metadata_filename);
                FILE *file = fopen(filename, "w");
                if (!file) {
                    fprintf(stderr, "Could not open file %s for writing", filename);
                    perror("ERROR: ");
                    exit(1);
                }

                if (attackConfig.type == ATTACK_FIFTY_ONE) {
                    fprintf(file, attack_metadata_format, "51", me, state->blockchainState.miningState.hashPowerPortion,
                            0, attackConfig.fiftyOne.catchupTolerance, attackerState->failed_attacks,
                            attackerState->successful_conceals);
                } else if (attackConfig.type == ATTACK_SELFISH_MINING) {
                    fprintf(file, attack_metadata_format, "selfish", me,
                            state->blockchainState.miningState.hashPowerPortion, attackConfig.selfish.depth,
                            attackConfig.selfish.catchupTolerance, attackerState->failed_attacks,
                            attackerState->successful_conceals);
                }
            }

            if (statsType == STATS_SELFISH) {
                copySelfishStatisticsState(&selfishStats[me], &state->statsState.selfishStats);
            }

            rs_free(state->rng);
            deinitNetwork();
            deinitBlockchainState(&state->blockchainState);
            deinitTransactionState(&state->transactionState);
            deinitStatisticsState(&state->statsState);
            rs_free(state);
            return;
        }
        case RBLOCKSIM_INIT: {
            if (is_attacker(me)) {
                attackerAfterInitBlockchainState(&state->blockchainState);
            } else {
                afterInitBlockchainState(&state->blockchainState);
            }
            break;
        }
        case GENERATE_BLOCK: {
            struct Block *b = generateBlock(me, now, &state->blockchainState, &state->transactionState, &state->statsState);

            if (attackConfig.type == ATTACK_SELFISH_MINING && is_attacker(me) && !attackerState->finishedSelfishMining) {
                // If the parent of the block is not me, then either I am starting selfish mining or a chain switch occurred
                struct ChainNode *parent = findChainNode(&state->blockchainState.chain, b->prevBlockMiner, b->height - 1);
                if (attackerState->isSelfishMining && parent && parent->miner != me) {
                    // Chain switch occurred. Trigger a selfish mining reset and try again
                    attackerState->isSelfishMining = false;
                    // printf("Chain switch occurred at height %lu. Resetting selfish mining\n", b->height - 1);
                    attackerState->failed_attacks++;
                }

                if (now >= attackConfig.selfish.startTime && !attackerState->isSelfishMining) {
                    // Start selfish mining
                    attackerState->isSelfishMining = true;
                    attackerState->last_propagated_height = b->height - 1;
                }

                if (attackerState->isSelfishMining) {
                    // Check to see whether we should propagate the block or not
                    size_t concealed_blocks =
                            state->blockchainState.chain.height - attackerState->last_propagated_height;
                    if (concealed_blocks >= attackConfig.selfish.depth) {
                        // An attack is successful if the block is propagated and other nodes switch to the attacker's chain
                        attackerState->successful_conceals++;
                        if (statsType == STATS_SELFISH) {
                            statsSwitchToSelfishChain(
                                    &state->statsState); // For good measure. This statistic is no use for this node
                        }
                        // Tag the block!! And receivers that switch to the attacker's chain will see it and note it
                        b->is_attack_block = true;

                        // propagate the block and all its ancestors!
                        propagateBlockAndNAncestors(&state->blockchainState.chain, me, now, b, concealed_blocks,
                                                    state->rng);
                        attackerState->last_propagated_height = b->height;
                        attackerState->isSelfishMining = false;
                        // attackerState->finishedSelfishMining = true;
                    }
                }
            } else {
                propagateBlock(me, now, b, state->rng);
            }

            if (statsType == STATS_DETAILED) {
                statsMineBlockDetailed(&state->statsState, me, b->height, now);
            } else if (statsType == STATS_SELFISH) {
                statsMineBlockSelfish(&state->statsState);
            }
            break;
        }
        case RECEIVE_BLOCK: {
            struct Block *b = (struct Block *) event_content;
            //printf("[N %lu - t %lf] Block received. Miner %lu Depth %d\n", me, now, b->miner, b->height);
            struct ChainNode *seeked_node = findChainNode(&state->blockchainState.chain, b->miner, b->height);
            if (seeked_node) { // Block already received
                if (isOrphan(seeked_node)) { // If it is an orphan, request the parent
                    requestParent(me, now, state, b);
                }
                return;
            }
            if (statsType == STATS_DETAILED) {
                statsReceiveBlockDetailed(&state->statsState, b->miner, b->height, now);
            }

            bool updated_mainchain = false;
            bool found_parent = false;
            receiveBlock(now, &state->blockchainState, &state->transactionState, b, me, &state->statsState, &updated_mainchain, &found_parent);
            if (!found_parent) {
                // Request the parent block
                requestParent(me, now, state, b);
            }

            node_id_t old_sender = b->sender;
            b->sender = me;
            propagateBlock(me, now, b, state->rng);
            b->sender = old_sender;

            if (!updated_mainchain) {
                return;
            }

            if (b->is_attack_block) {
                // I received an attack block and switched to the attacker's chain
                if (statsType == STATS_SELFISH) {
                    statsSwitchToSelfishChain(&state->statsState);
                }
            }

            if (is_attacker(me)) {
                attackerState->last_propagated_height = b->height;
            }

            break;
        }
        case REQUEST_BLOCK: {
            // Another node has requested a block from us.
            // Identified by using height and miner
            struct request_block_evt *evt = (struct request_block_evt *) event_content;
            // Find the Block from the blockchain
            struct Block *block = retrieveBlock(&state->blockchainState.chain, evt->miner, evt->height);
            if (!block) return;
            block->sender = me;
            // Send the block
            sendSingleBlock(me, evt->requester, now, block, state->rng, RECEIVE_BLOCK);
            free(block);
            return;
        }
        default: {
            fprintf(stderr, "EVENT TYPE %u UNKNOWN", event_type);
            abort();
        }
    }

    scheduleNextBlockGeneration(now, state->rng, &state->blockchainState);
}

bool CanEnd(lp_id_t me, const void *snapshot) { return false; }

#ifndef TESTING
void handle_options(int argc, char **argv) {
    int opt;
    double opt_hashpower = -1.0;
    simtime_t opt_start_time = -1.0;
    size_t opt_attack_depth = 0;
    bool depth_set = false;
    size_t opt_catchup_tolerance = 0;
    bool catchup_tolerance_set = false;

    while ((opt = getopt(argc, argv, "a:c:d:h:i:o:r:s:w:")) != -1) {
        switch (opt) {
            case 'w':
            {
                conf.n_threads = atoi(optarg);
                printf("Threads set to: %d\n", conf.n_threads);
                break;
            }
            case 'i':
            {
                // Read BLOCK_INTERVAL from command line. It is a double
                BLOCK_INTERVAL = atof(optarg);
                printf("Block interval set to: %lf\n", BLOCK_INTERVAL);
                break;
            }
            case 'a':
            {
                // Read ATTACK_TYPE from command line. It is a string
                if (strcmp(optarg, "51") == 0) {
                    attackConfig.type = ATTACK_FIFTY_ONE;
                    setStatsType(STATS_SELFISH);
                    printf("Attack type set to: 51%%\n");
                } else if (strcmp(optarg, "selfish") == 0) {
                    attackConfig.type = ATTACK_SELFISH_MINING;
                    setStatsType(STATS_SELFISH);
                    printf("Attack type set to: Selfish mining\n");
                } else {
                    fprintf(stderr, "Unknown attack type: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 'h': {
                // Read attacker's portion of hash power from command line. It is a double
                opt_hashpower = atof(optarg);
                if (opt_hashpower < 0.0 || opt_hashpower > 1.0) {
                    fprintf(stderr, "Invalid attacker hash power value: %lf. Hash power is a percentage and must be between 0.0 and 1.0.\n", opt_hashpower);
                    exit(EXIT_FAILURE);
                }
                printf("Attacker hash power set to: %lf of network total hash power\n", opt_hashpower);
                break;
            }
            case 'd': {
                // Read attack's depth from command line. It is a size_t
                opt_attack_depth = strtoul(optarg, NULL, 10);
                depth_set = true;
                break;
            }
            case 'c': {
                // Read catchup tolerance from command line. It is a size_t
                opt_catchup_tolerance = strtoul(optarg, NULL, 10);
                catchup_tolerance_set = true;
                if (opt_catchup_tolerance > DEPTH_TO_KEEP) {
                    fprintf(stderr, "Invalid catchup tolerance value: %lu. Catchup tolerance must be less than MAX_DEPTH: %d.\n", opt_catchup_tolerance, DEPTH_TO_KEEP);
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 's': {
                // Read the start time of the attack from command line. It is a double
                opt_start_time = atof(optarg);
                if (opt_start_time < 0.0) {
                    fprintf(stderr, "Invalid start time value: %lf. Start time must be greater than 0.0.\n", opt_start_time);
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 'o':
            {
                // Allocate memory for the filename and a `.json` extension
                old_stats_filename = malloc(strlen(optarg) + strlen(stats_extension) + 1);
                // Read OUTPUT_FILE from command line. It is a string
                strcpy(old_stats_filename, optarg);
                char *extension = strrchr(old_stats_filename, '.');
                // If the extension is not .json, append it
                if (!extension || strcmp(extension, stats_extension) != 0) {
                    strcat(old_stats_filename, stats_extension);
                }
                //printf("Output file set to: %s\n", old_stats_filename);
                break;
            }
            case 'r':
            {
                // Read RNG_SEED from command line. It is an unsigned long
                rng_seed = strtoul(optarg, NULL, 10);
                printf("RNG seed set to: %u\n", rng_seed);
                break;
            }
            default:
            {
                fprintf(stderr, "Usage: %s [-w thread_count] [-i block_interval (seconds)] [-a attack_type in {51, selfish} [-h percentage of network total hash power for the attacker] [-d depth of attack for selfish mining] [-s start time of attack for selfish mining] [-c maximum depth the node can lag behind before switching chains to one on which it has mined fewer blocks]] [-o statistics_output_filename] [-r rng_seed]\n", argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }

    if (opt_hashpower == -1.0) { // Attacker hash power not specified
        switch (attackConfig.type) {
            case ATTACK_FIFTY_ONE:
                attackConfig.fiftyOne.hashPowerPortion = DEFAULT_FIFTY_ONE_HASHPOWER;
                printf("Attacker hash power not specified. Using default value for 51%% attack: %lf\n", attackConfig.fiftyOne.hashPowerPortion);
                break;
            case ATTACK_SELFISH_MINING:
                attackConfig.selfish.hashPowerPortion = DEFAULT_SELFISH_HASHPOWER;
                printf("Attacker hash power not specified. Using default value for selfish mining attack: %lf\n", attackConfig.selfish.hashPowerPortion);
                break;
            default:
                break;
        }
    } else {
        switch (attackConfig.type) {
            case ATTACK_FIFTY_ONE:
                attackConfig.fiftyOne.hashPowerPortion = opt_hashpower;
                break;
            case ATTACK_SELFISH_MINING:
                attackConfig.selfish.hashPowerPortion = opt_hashpower;
                break;
            default:
                fprintf(stderr, "Attacker hash power specified, but no attack selected! Specify the attack with [-a attack_type in {51, selfish}]\n");
                exit(EXIT_FAILURE);
        }
    }

    if (catchup_tolerance_set) {
        switch (attackConfig.type) {
            case ATTACK_FIFTY_ONE:
                attackConfig.fiftyOne.catchupTolerance = opt_catchup_tolerance;
                break;
            case ATTACK_SELFISH_MINING:
                attackConfig.selfish.catchupTolerance = opt_catchup_tolerance;
                break;
            default:
                fprintf(stderr, "Catchup tolerance specified, but no attack selected! Specify the attack with [-a attack_type in {51, selfish}]\n");
                exit(EXIT_FAILURE);
        }
    } else {
        switch (attackConfig.type) {
            case ATTACK_FIFTY_ONE:
                attackConfig.fiftyOne.catchupTolerance = DEFAULT_CATCHUP_TOLERANCE;
                printf("FiftyOne catchup tolerance not specified. Using default value: %lu\n", attackConfig.fiftyOne.catchupTolerance);
                break;
            case ATTACK_SELFISH_MINING:
                attackConfig.selfish.catchupTolerance = DEFAULT_CATCHUP_TOLERANCE;
                printf("Selfish catchup tolerance not specified. Using default value for selfish mining attack: %lu\n", attackConfig.selfish.catchupTolerance);
                break;
            default:
                break;
        }
    }

    if (depth_set) {
        switch (attackConfig.type) {
            case ATTACK_FIFTY_ONE:
                fprintf(stderr, "Attack depth specified, but 51%% attack does not use it!\n");
                exit(EXIT_FAILURE);
            case ATTACK_SELFISH_MINING:
                attackConfig.selfish.depth = opt_attack_depth;
                break;
            default:
                fprintf(stderr, "Attack depth specified, but no attack selected! Specify the attack with [-a attack_type in {51, selfish}]\n");
                exit(EXIT_FAILURE);
        }
    } else {
        if (attackConfig.type == ATTACK_SELFISH_MINING) {
            attackConfig.selfish.depth = DEFAULT_SELFISH_DEPTH;
            printf("Selfish mining attack depth not specified. Using default value: %lu\n", attackConfig.selfish.depth);
        }
    }

    if (opt_start_time != -1.0) {
        switch (attackConfig.type) {
            case ATTACK_FIFTY_ONE:
                fprintf(stderr, "WARN: Attack start time specified, but 51%% attack ignores it.\n");
                break;
            case ATTACK_SELFISH_MINING:
                attackConfig.selfish.startTime = opt_start_time;
                break;
            default:
                fprintf(stderr,
                        "Attack start time specified, but no attack selected! Specify the attack with [-a attack_type in {51, selfish}]\n");
        }
    } else if (attackConfig.type == ATTACK_SELFISH_MINING) {
        attackConfig.selfish.startTime = DEFAULT_SELFISH_START_TIME;
        printf("Selfish mining attack start time not specified. Using default value: %lf\n",
               attackConfig.selfish.startTime);
    }
}

size_t formatStatsFolder(char *dest, int folder_num, size_t maxlen) {
    switch (attackConfig.type) {
        case ATTACK_FIFTY_ONE:
            return snprintf(dest, maxlen, stats_folder_long, conf.lps, conf.n_threads, BLOCK_INTERVAL, "51", attackConfig.fiftyOne.hashPowerPortion, attackConfig.fiftyOne.catchupTolerance, 0, rng_seed, folder_num);
        case ATTACK_SELFISH_MINING:
            return snprintf(dest, maxlen, stats_folder_long, conf.lps, conf.n_threads, BLOCK_INTERVAL, "selfish", attackConfig.selfish.hashPowerPortion, attackConfig.selfish.catchupTolerance, attackConfig.selfish.depth, rng_seed, folder_num);
        default:
            return snprintf(dest, maxlen, stats_folder_long, conf.lps, conf.n_threads, BLOCK_INTERVAL, "none", 0.0, 0, 0, rng_seed, folder_num);
    }
}

size_t formatStatsFile(char *dest, size_t maxlen) {
    switch (attackConfig.type) {
        case ATTACK_FIFTY_ONE:
            return snprintf(dest, maxlen, single_stats_filename, conf.lps, conf.n_threads, BLOCK_INTERVAL, "51", attackConfig.fiftyOne.hashPowerPortion, attackConfig.fiftyOne.catchupTolerance, 0, rng_seed);
        case ATTACK_SELFISH_MINING:
            return snprintf(dest, maxlen, single_stats_filename, conf.lps, conf.n_threads, BLOCK_INTERVAL, "selfish", attackConfig.selfish.hashPowerPortion, attackConfig.selfish.catchupTolerance, attackConfig.selfish.depth, rng_seed);
        default:
            return snprintf(dest, maxlen, single_stats_filename, conf.lps, conf.n_threads, BLOCK_INTERVAL, "none", 0.0, 0, 0, rng_seed);
    }
}

int main(int argc, char **argv) {
    setStatsType(STATS_NONE);
    attackConfig.type = ATTACK_NONE;

    handle_options(argc, argv);

    if (statsType != STATS_NONE) {
        if (conf.lps > 1000000) {
            fprintf(stderr, "More than a million LPs. Stats files will not have leading zeros.\n");
        }

        // Find the next available folder
        size_t max_path_len = sizeof(old_stats_folder) / sizeof(*old_stats_folder);
        int folder_num = 0;
        char *write_ptr = single_stats_fullpath;

        write_ptr = single_stats_fullpath + formatStatsFolder(single_stats_fullpath, folder_num, max_path_len);

        while (access(single_stats_fullpath, F_OK) != -1) {
            folder_num++;
            write_ptr = single_stats_fullpath + formatStatsFolder(single_stats_fullpath, folder_num, max_path_len);
        }

        printf("Statistics will be saved in folder %s\n", single_stats_fullpath);

        // Create the folder
        if (mkdir(single_stats_fullpath, 0777) == -1) {
            perror("Could not create folder for statistics");
            exit(EXIT_FAILURE);
        }

        strcpy(single_stats_metadata_fullpath, single_stats_fullpath);

        formatStatsFile(write_ptr, sizeof(single_stats_fullpath) - (write_ptr - single_stats_fullpath));
        printf("Statistics will be saved in file %s\n", single_stats_fullpath);
    }

    struct rng_t rng;
    initialize_stream(rng_seed, &rng);

    switch (attackConfig.type) {
        case ATTACK_FIFTY_ONE:
        case ATTACK_SELFISH_MINING:
            initAttackers(1, &rng);
            break;
        default:
            initAttackers(0, NULL);
            break;

    }
    printAttackers();

    switch (statsType) {
        // For now only for selfish, since I only use this one.
        case STATS_SELFISH:
            // At the end of the simulation, each LP writes their statistics data. Later, we will dump the statistics in a JSON file
            selfishStats = malloc(sizeof(struct SelfishStatsState) * conf.lps);
            break;
        case STATS_FIFTY_ONE:
        case STATS_NONE:
        case STATS_DETAILED:
            break;
        default:
            perror("Unknown statistics type");
            exit(1);
    }

    generateTransactions(&rng);

    RootsimInit(&conf);
    RootsimRun();

    if (statsType == STATS_SELFISH) {
        // Dump the grouped together statistics.
        size_t buf_size = sizeof(char) * (conf.lps * 256 + 256);
        char *stats_buf = malloc(buf_size);
        char *write_ptr = stats_buf;
        char *buf_end = stats_buf + (buf_size - 1);

        write_ptr += sprintf(write_ptr, "{\n\"header\":");
        write_ptr += sprintSelfishStatsHeader(write_ptr, buf_end - write_ptr);
        write_ptr += sprintf(write_ptr, ",\n\"data\": [\n");
        for (node_id_t i = 0; i < conf.lps; i++) {
            write_ptr += sprintSelfishStats(&selfishStats[i], write_ptr, buf_end - write_ptr);
            write_ptr += sprintf(write_ptr, ",\n");
        }
        write_ptr -= 2; // Remove the last comma
        write_ptr += sprintf(write_ptr, "\n]\n}\n");

        FILE *stats_file = fopen(single_stats_fullpath, "w");
        fwrite(stats_buf, sizeof(char), write_ptr - stats_buf, stats_file);

        fclose(stats_file);
        free(selfishStats);
    }

    deinitAttackers();
    return 0;
}

#endif

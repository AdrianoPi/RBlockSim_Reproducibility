#include <string.h>
#include "Statistics.h"
#include "Attacks.h"

enum StatsType statsType = STATS_NONE;

void initDetailedStatisticsState(struct StatsState *state) {
    statsType = STATS_DETAILED;

    struct DetailedStatsState *receivedState = &state->detailedStats;
    receivedState->blockStatsCapacity = 1;
    receivedState->blockStatsSize = 0;
    receivedState->blockStats = rs_malloc(receivedState->blockStatsCapacity * sizeof(struct BlockStat));

    receivedState->minedBlockStatsCapacity = 1;
    receivedState->minedBlockStatsSize = 0;
    receivedState->minedBlockStats = rs_malloc(receivedState->minedBlockStatsCapacity * sizeof(struct MinedBlockStat));
}

void initFiftyOneStatisticsState(struct StatsState *state) {
    statsType = STATS_FIFTY_ONE;

    struct FiftyOneStatsState *fiftyOneState = &state->fiftyOneStats;
    fiftyOneState->attackerBlocksInMainChain = 0;
    fiftyOneState->totalBlocksInMainChain = 0;
}

void initSelfishStatisticsState(struct StatsState *state) {
    statsType = STATS_SELFISH;

    struct SelfishStatsState *selfishState = &state->selfishStats;
    selfishState->attackerBlocksInMainChain = 0;
    selfishState->totalBlocksInMainChain = 0;
    selfishState->totalBlocksMined = 0;
    selfishState->ownBlocksInMainChain = 0;
    selfishState->switchesToSelfishChain = 0;
}

void deinitReceivedStatisticsState(struct DetailedStatsState *state) {
    rs_free(state->blockStats);
    rs_free(state->minedBlockStats);
}

void deinitStatisticsState(struct StatsState *state) {
    switch (statsType) {
        case STATS_DETAILED:
            deinitReceivedStatisticsState(&state->detailedStats);
            break;
        case STATS_FIFTY_ONE:
        case STATS_SELFISH:
        case STATS_NONE:
            break;
        default:
            perror("deinitStatisticsState: unknown stats type");
            exit(1);
    }
}

void copyReceivedStatisticsState(struct DetailedStatsState *dest, const struct DetailedStatsState *src) {
    dest->blockStatsCapacity = src->blockStatsCapacity;
    dest->blockStatsSize = src->blockStatsSize;
    struct BlockStat *destBlockStat = rs_realloc(dest->blockStats, dest->blockStatsCapacity * sizeof(struct BlockStat));
    if (!destBlockStat) {
        perror("copyStatisticsState: could not realloc blockStats");
        exit(1);
    }
    dest->blockStats = destBlockStat;
    memcpy(dest->blockStats, src->blockStats, dest->blockStatsSize * sizeof(struct BlockStat));

    dest->minedBlockStatsCapacity = src->minedBlockStatsCapacity;
    dest->minedBlockStatsSize = src->minedBlockStatsSize;
    struct MinedBlockStat *destMinedBlockStat = rs_realloc(dest->minedBlockStats,
                                                        dest->minedBlockStatsCapacity * sizeof(struct MinedBlockStat));
    if (!destMinedBlockStat) {
        perror("copyStatisticsState: could not realloc minedBlockStats");
        exit(1);
    }
    dest->minedBlockStats = destMinedBlockStat;
    memcpy(dest->minedBlockStats, src->minedBlockStats, dest->minedBlockStatsSize * sizeof(struct MinedBlockStat));
}

void copyFiftyOneStatisticsState(struct FiftyOneStatsState *dest, const struct FiftyOneStatsState *src) {
    *dest = *src;
}

void copySelfishStatisticsState(struct SelfishStatsState *dest, const struct SelfishStatsState *src) {
    *dest = *src;
}

void copyStatisticsState(struct StatsState *dest, const struct StatsState *src) {
    switch (statsType) {
        case STATS_NONE:
            break;
        case STATS_DETAILED:
            copyReceivedStatisticsState(&dest->detailedStats, &src->detailedStats);
            break;
        case STATS_FIFTY_ONE:
            copyFiftyOneStatisticsState(&dest->fiftyOneStats, &src->fiftyOneStats);
            break;
        case STATS_SELFISH:
            copySelfishStatisticsState(&dest->selfishStats, &src->selfishStats);
            break;
        default:
            perror("copyStatsState: unknown stats type");
            exit(1);
    }
}

void statsReceiveBlockDetailed(struct StatsState *state, node_id_t miner, size_t height, simtime_t receivedTime) {
    if (statsType != STATS_DETAILED) {
        perror("statsReceiveBlockDetailed: state type is not DETAILED_STATS");
        exit(1);
    }

    struct DetailedStatsState *receivedState = &state->detailedStats;
    while (receivedState->blockStatsSize >= receivedState->blockStatsCapacity) {
        receivedState->blockStatsCapacity *= 2;
        struct BlockStat *newBlockStats = rs_realloc(receivedState->blockStats,
                                                  receivedState->blockStatsCapacity * sizeof(struct BlockStat));
        if (!newBlockStats) {
            perror("Could not realloc blockStats");
            exit(1);
        }
        receivedState->blockStats = newBlockStats;
    }

    struct BlockStat *blockStat = &receivedState->blockStats[receivedState->blockStatsSize++];
    blockStat->miner = miner;
    blockStat->height = height;
    blockStat->receivedTime = receivedTime;
}

void statsMineBlockDetailed(struct StatsState *state, node_id_t miner, size_t height, simtime_t minedTime) {
    if (statsType != STATS_DETAILED) {
        perror("statsReceiveBlockDetailed: state type is not DETAILED_STATS");
        exit(1);
    }

    struct DetailedStatsState *receivedState = &state->detailedStats;
    while (receivedState->minedBlockStatsSize >= receivedState->minedBlockStatsCapacity) {
        receivedState->minedBlockStatsCapacity *= 2;
        struct MinedBlockStat *newMinedStats = rs_realloc(receivedState->minedBlockStats,
                                                       receivedState->minedBlockStatsCapacity * sizeof(struct BlockStat));
        if (!newMinedStats) {
            perror("Could not realloc blockStats");
            exit(1);
        }
        receivedState->minedBlockStats = newMinedStats;
    }

    struct MinedBlockStat *minedStat = &receivedState->minedBlockStats[receivedState->minedBlockStatsSize++];
    minedStat->miner = miner;
    minedStat->height = height;
    minedStat->minedTime = minedTime;
}

void statsAddBlockFiftyOne(struct StatsState *state, node_id_t miner, node_id_t me) {
    if (statsType != STATS_FIFTY_ONE) {
        perror("statsAddBlockFiftyOne: stats type is not STATS_FIFTY_ONE");
        exit(1);
    }

    state->fiftyOneStats.totalBlocksInMainChain++;
    if (is_attacker(miner)) {
        state->fiftyOneStats.attackerBlocksInMainChain++;
    }
}

void statsRemoveBlockFiftyOne(struct StatsState *state, node_id_t miner, node_id_t me) {
    if (statsType != STATS_FIFTY_ONE) {
        perror("statsRemoveBlockFiftyOne: stats type is not STATS_FIFTY_ONE");
        exit(1);
    }

    state->fiftyOneStats.totalBlocksInMainChain--;
    if (is_attacker(miner)) {
        state->fiftyOneStats.attackerBlocksInMainChain--;
    }
}

void statsAddBlockSelfish(struct StatsState *state, node_id_t miner, node_id_t me) {
    if (statsType != STATS_SELFISH) {
        perror("statsAddBlockSelfish: stats type is not STATS_SELFISH");
        exit(1);
    }

    state->selfishStats.totalBlocksInMainChain++;
    if (miner == me) {
        state->selfishStats.ownBlocksInMainChain++;
    }
    if (is_attacker(miner)) {
        state->selfishStats.attackerBlocksInMainChain++;
    }
}

void statsRemoveBlockSelfish(struct StatsState *state, node_id_t miner, node_id_t me) {
    if (statsType != STATS_SELFISH) {
        perror("statsRemoveBlockSelfish: stats type is not STATS_SELFISH");
        exit(1);
    }

    state->selfishStats.totalBlocksInMainChain--;
    if (miner == me) {
        state->selfishStats.ownBlocksInMainChain--;
    }
    if (is_attacker(miner)) {
        state->selfishStats.attackerBlocksInMainChain--;
    }
}

void statsMineBlockSelfish(struct StatsState *state) {
    if (statsType != STATS_SELFISH) {
        perror("statsMineBlockSelfish: stats type is not STATS_SELFISH");
        exit(1);
    }

    state->selfishStats.totalBlocksMined++;
}

void statsSwitchToSelfishChain(struct StatsState *state) {
    if (statsType != STATS_SELFISH) {
        perror("statsSwitchToSelfishChain: stats type is not STATS_SELFISH");
        exit(1);
    }

    state->selfishStats.switchesToSelfishChain++;
}

void dumpStats(struct StatsState *state, const char *filename) {
    switch (statsType) {
        case STATS_NONE:
            break;
        case STATS_DETAILED:
            dumpDetailedStats(&state->detailedStats, filename);
            break;
        case STATS_FIFTY_ONE:
            dumpFiftyOneStats(&state->fiftyOneStats, filename);
            break;
        case STATS_SELFISH:
            dumpSelfishStats(&state->selfishStats, filename);
            break;
        default:
            perror("dumpStats: unknown stats type");
            exit(1);
    }
}

void dumpStatsBinary(struct StatsState *state, const char *filename) {
    switch (statsType) {
        case STATS_NONE:
            break;
        case STATS_DETAILED:
            dumpDetailedStatsBinary(&state->detailedStats, filename);
            break;
        case STATS_FIFTY_ONE:
            dumpFiftyOneStatsBinary(&state->fiftyOneStats, filename);
            break;
        case STATS_SELFISH:
            dumpSelfishStatsBinary(&state->selfishStats, filename);
            break;
        default:
            perror("dumpStatsBinary: unknown stats type");
            exit(1);
    }
}

void printStats(struct StatsState *state) {
    switch (statsType) {
        case STATS_NONE:
            break;
        case STATS_DETAILED:
            printDetailedStats(&state->detailedStats);
            break;
        case STATS_FIFTY_ONE:
            printFiftyOneStats(&state->fiftyOneStats);
            break;
        case STATS_SELFISH:
            printSelfishStats(&state->selfishStats);
            break;
        default:
            perror("printStats: unknown stats type");
            exit(1);
    }
}

void dumpDetailedStats(struct DetailedStatsState *state, const char *filename) {
    // Open the file
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Could not open file %s for writing", filename);
        perror("ERROR: ");
        exit(1);
    }

    // Write the block stats in json format
    fprintf(file, "{\n\"blockStats\":[\n");
    for (size_t i = 0; i < state->blockStatsSize; i++) {
        struct BlockStat *blockStat = &state->blockStats[i];
        fprintf(file, "{\"miner\":%d,\"height\":%lu,\"receivedTime\":%lf}", blockStat->miner, blockStat->height,
                blockStat->receivedTime);
        if (i < state->blockStatsSize - 1) {
            fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    fprintf(file, "],\n");

    // Write the mined block stats in json format
    fprintf(file, "\"minedBlockStats\":[\n");
    for (size_t i = 0; i < state->minedBlockStatsSize; i++) {
        struct MinedBlockStat *minedStat = &state->minedBlockStats[i];
        fprintf(file, "{\"miner\":%d,\"height\":%lu,\"minedTime\":%lf}", minedStat->miner, minedStat->height,
                minedStat->minedTime);
        if (i < state->minedBlockStatsSize - 1) {
            fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    fprintf(file, "]\n}\n");

    fclose(file);
}

void dumpDetailedStatsBinary(struct DetailedStatsState *state, const char *filename) {
    // Open file
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Could not open file %s for writing", filename);
        perror("ERROR: ");
        exit(1);
    }

    // Write block stats
    fwrite(&state->blockStatsSize, sizeof(size_t), 1, file);
    fwrite(state->blockStats, sizeof(struct BlockStat), state->blockStatsSize, file);

    // Write mined block stats
    fwrite(&state->minedBlockStatsSize, sizeof(size_t), 1, file);
    fwrite(state->minedBlockStats, sizeof(struct MinedBlockStat), state->minedBlockStatsSize, file);

    fclose(file);
}

void printDetailedStats(struct DetailedStatsState *state) {
    // Write the block stats in json format
    printf("{\n\"blockStats\":[\n");
    for (size_t i = 0; i < state->blockStatsSize; i++) {
        struct BlockStat *blockStat = &state->blockStats[i];
        printf("{\"miner\":%d,\"height\":%lu,\"receivedTime\":%lf}", blockStat->miner, blockStat->height,
               blockStat->receivedTime);
        if (i < state->blockStatsSize - 1) {
            printf(",");
        }
        printf("\n");
    }
    printf("],\n");

    // Write the mined block stats in json format
    printf("\"minedBlockStats\":[\n");
    for (size_t i = 0; i < state->minedBlockStatsSize; i++) {
        struct MinedBlockStat *minedStat = &state->minedBlockStats[i];
        printf("{\"miner\":%d,\"height\":%lu,\"minedTime\":%lf}", minedStat->miner, minedStat->height,
               minedStat->minedTime);
        if (i < state->minedBlockStatsSize - 1) {
            printf(",");
        }
        printf("\n");
    }
    printf("]\n}\n");
}

void dumpFiftyOneStats(struct FiftyOneStatsState *state, const char *filename) {
    // Open the file
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Could not open file %s for writing", filename);
        perror("ERROR: ");
        exit(1);
    }

    // Write the stats in json format
    fprintf(file, "{\n\"attackerBlocksInMainChain\":%lu,\"totalBlocksInMainChain\":%lu\n}\n",
            state->attackerBlocksInMainChain, state->totalBlocksInMainChain);

    fclose(file);
}

void dumpFiftyOneStatsBinary(struct FiftyOneStatsState *state, const char *filename) {
    // Open file
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Could not open file %s for writing", filename);
        perror("ERROR: ");
        exit(1);
    }

    // Write the stats
    fwrite(state, sizeof(struct FiftyOneStatsState), 1, file);

    fclose(file);
}

void printFiftyOneStats(struct FiftyOneStatsState *state) {
    // Write the stats in json format
    printf("{\n\"attackerBlocksInMainChain\":%lu,\"totalBlocksInMainChain\":%lu\n}\n",
           state->attackerBlocksInMainChain, state->totalBlocksInMainChain);
}

void dumpSelfishStatsVerbose(struct SelfishStatsState *state, const char *filename) {
    // Open the file
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Could not open file %s for writing", filename);
        perror("ERROR: ");
        exit(1);
    }

    // Write the stats in json format
    fprintf(file, "{\n\"attackerBlocksInMainChain\":%lu,\"totalBlocksInMainChain\":%lu,\"totalBlocksMined\":%lu,\"ownBlocksInMainChain\":%lu,\"switchesToSelfishChain\":%lu\n}\n",
            state->attackerBlocksInMainChain, state->totalBlocksInMainChain, state->totalBlocksMined, state->ownBlocksInMainChain, state->switchesToSelfishChain);

    fclose(file);
}

void dumpSelfishStats(struct SelfishStatsState *state, const char *filename) {
    // Open the file
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Could not open file %s for writing", filename);
        perror("ERROR: ");
        exit(1);
    }

    // Write the stats in json format
    fprintf(file, "{\n\"ABMC\":%lu,\"TBMC\":%lu,\"TBM\":%lu,\"OBIMC\":%lu,\"Sw\":%lu\n}\n",
            state->attackerBlocksInMainChain, state->totalBlocksInMainChain, state->totalBlocksMined, state->ownBlocksInMainChain, state->switchesToSelfishChain);

    fclose(file);
}

void dumpSelfishStatsBinary(struct SelfishStatsState *state, const char *filename) {
    // Open file
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Could not open file %s for writing", filename);
        perror("ERROR: ");
        exit(1);
    }

    // Write the stats
    fwrite(state, sizeof(struct SelfishStatsState), 1, file);

    fclose(file);
}

void printSelfishStats(struct SelfishStatsState *state) {
    // Write the stats in json format
    printf("{\n\"attackerBlocksInMainChain\":%lu,\"totalBlocksInMainChain\":%lu,\"totalBlocksMined\":%lu,\"ownBlocksInMainChain\":%lu,\"switchesToSelfishChain\":%lu\n}\n",
           state->attackerBlocksInMainChain, state->totalBlocksInMainChain, state->totalBlocksMined, state->ownBlocksInMainChain, state->switchesToSelfishChain);
}

size_t sprintSelfishStatsHeader(char *buffer, size_t buffer_size) {
    size_t written = 0;
    written += snprintf(buffer + written, buffer_size - written, "[\"attackerBlocksInMainChain\",\"totalBlocksInMainChain\",\"totalBlocksMined\",\"ownBlocksInMainChain\",\"switchesToSelfishChain\"]");
    return written;
}

size_t sprintSelfishStats(struct SelfishStatsState *state, char *buffer, size_t buffer_size) {
    size_t written = 0;
    written += snprintf(buffer + written, buffer_size - written, "[%lu,%lu,%lu,%lu,%lu]",
                        state->attackerBlocksInMainChain, state->totalBlocksInMainChain, state->totalBlocksMined, state->ownBlocksInMainChain, state->switchesToSelfishChain);
    if (written > buffer_size) {
        perror("sprintSelfishStats: buffer too small. Truncated output");
        exit(1);
    }
    return written;
}

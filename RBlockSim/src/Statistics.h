#pragma once

#include <ROOT-Sim.h>
#include "Typedefs.h"

/// Used to gather statistics about the blocks. Each node will have a list of these, one entry per block received.
struct BlockStat {
    node_id_t miner;
    size_t height;
    simtime_t receivedTime;
};

/// Used to gather statistics about the blocks mined locally. Each node will have a list of these, one entry per block mined.
struct MinedBlockStat {
    node_id_t miner;
    size_t height;
    simtime_t minedTime;
};

enum StatsType {
    STATS_NONE,
    STATS_DETAILED,
    STATS_FIFTY_ONE,
    STATS_SELFISH
};

extern enum StatsType statsType;

/// State variations for the statistics module

/// Tracks the blocks received and mined by the node
struct DetailedStatsState {
    // FIXME: This gets too big for managed memory.
    struct BlockStat *blockStats;
    size_t blockStatsSize;
    size_t blockStatsCapacity;

    // FIXME: too big for managed memory, same as blockStats
    struct MinedBlockStat *minedBlockStats;
    size_t minedBlockStatsSize;
    size_t minedBlockStatsCapacity;
};

/// For 51% attack: tracks the blocks mined by the attacker that are in the main chain
struct FiftyOneStatsState {
    size_t attackerBlocksInMainChain; ///< Number of blocks mined by the attacker that are in the main chain
    size_t totalBlocksInMainChain;    ///< Total number of blocks in the main chain
};

/// For selfish mining attack: tracks the blocks mined by the attacker that are in the local main chain and the ones mined by the node
struct SelfishStatsState {
    size_t attackerBlocksInMainChain; ///< Number of blocks mined by the attacker that are in the main chain
    size_t totalBlocksInMainChain;    ///< Total number of blocks in the main chain
    size_t totalBlocksMined;          ///< Total number of blocks mined by the node
    size_t ownBlocksInMainChain;      ///< Number of blocks mined by the node that are in the main chain
    size_t switchesToSelfishChain;    ///< Number of times the node switched chain after receiving selfish blocks
};

struct StatsState {
    union {
        struct DetailedStatsState detailedStats;
        struct FiftyOneStatsState fiftyOneStats;
        struct SelfishStatsState selfishStats;
    };
};

/**
 * @brief Set the type of statistics to be tracked
 *
 * @param type The type of statistics to be tracked
 * */
static inline void setStatsType(enum StatsType type) {
    statsType = type;
}

/**
 * @brief Initialize the LP state of the statistics module with detailed block reception and mining statistics
 *
 * @param state The state of the statistics module
 * */
void initDetailedStatisticsState(struct StatsState *state);

/**
 * @brief Initialize the LP state of the statistics module for 51% attacks
 *
 * @param state The state of the statistics module
 * */
void initFiftyOneStatisticsState(struct StatsState *state);

/**
 * @brief Initialize the LP state of the statistics module for selfish mining attacks
 *
 * @param state The state of the statistics module
 * */
void initSelfishStatisticsState(struct StatsState *state);

/**
 * @brief Deinitialize the LP state of the statistics module
 *
 * @param state The state of the statistics module
 *
 * The function carries out the cleanup based on the type of statistics being tracked.
 * */
void deinitStatisticsState(struct StatsState *state);

/**
 * @brief Deep copy the statistics state from one struct to another
 *
 * @param dest The destination struct
 * @param src The source struct
 *
 * @warning This overwrites the destination struct without releasing memory, task left to the caller.
 * */
void copyStatisticsState(struct StatsState *dest, const struct StatsState *src);

/**
 * @brief Deep copy the selfish statistics state from one struct to another
 *
 * @param dest The destination struct
 * @param src The source struct
 *
 * @warning This overwrites the destination struct without releasing memory, task left to the caller.
 * */
void copySelfishStatisticsState(struct SelfishStatsState *dest, const struct SelfishStatsState *src);

/**
 * @brief Track the reception of a block
 *
 * @param state The state of the statistics module
 * @param miner The ID of the miner that mined the block
 * @param height The height of the block
 * @param receivedTime The time at which the block was received
 * */
void statsReceiveBlockDetailed(struct StatsState *state, node_id_t miner, size_t height, simtime_t receivedTime);

/**
 * @brief Track the mining of a block
 *
 * @param state The state of the statistics module
 * @param miner The ID of the miner that mined the block
 * @param height The height of the block
 * @param receivedTime The time at which the block was received
 * */
void statsMineBlockDetailed(struct StatsState *state, node_id_t miner, size_t height, simtime_t minedTime);

/**
 * @brief Track the inclusion of a block in the main chain
 *
 * @param state The state of the statistics module
 * @param miner The ID of the miner that mined the block
 * */
void statsAddBlockFiftyOne(struct StatsState *state, node_id_t miner, node_id_t me);

/**
 * @brief Track the inclusion of a block in the main chain with selfish mining attacks
 *
 * @param state The state of the statistics module
 * @param miner The ID of the miner that mined the block
 * */
void statsAddBlockSelfish(struct StatsState *state, node_id_t miner, node_id_t me);

/**
 * @brief Track the removal of a block from the main chain with 51% attacks
 *
 * @param state The state of the statistics module
 * @param miner The ID of the miner that mined the block
 * @param me The ID of the caller node
 * */
void statsRemoveBlockFiftyOne(struct StatsState *state, node_id_t miner, node_id_t me);

/**
 * @brief Track the removal of a block from the main chain with selfish mining attacks
 *
 * @param state The state of the statistics module
 * @param miner The ID of the miner that mined the block
 * @param me The ID of the caller node
 * */
void statsRemoveBlockSelfish(struct StatsState *state, node_id_t miner, node_id_t me);

/**
 * @brief Track the mining of a block in selfish mining attacks
 *
 * @param state The state of the statistics module
 * */
void statsMineBlockSelfish(struct StatsState *state);

/**
 * @brief Record a switch to the selfish chain
 *
 * @param state The state of the statistics module
 * */
void statsSwitchToSelfishChain(struct StatsState *state);

/**
 * @brief Writes all the statistics to a file
 *
 * @param state The state of the statistics module
 * @param filename The path to the file where the statistics will be written
 * */
void dumpStats(struct StatsState *state, const char *filename);

/**
 * @brief Writes the statistics to a file, in binary format
 *
 * @param state The state of the statistics module
 * @param filename The path to the file where the statistics will be written
 * */
void dumpStatsBinary(struct StatsState *state, const char *filename);

/**
 * @brief Prints all the statistics to stdout
 *
 * @param state The state of the statistics module
 * */
void printStats(struct StatsState *state);

/**
 * @brief Writes all the statistics to a file
 *
 * @param state The state of the statistics module
 * @param filename The path to the file where the statistics will be written
 * */
void dumpDetailedStats(struct DetailedStatsState *state, const char *filename);

/**
 * @brief Writes the statistics to a file, in binary format
 *
 * @param state The state of the statistics module
 * @param filename The path to the file where the statistics will be written
 * */
void dumpFiftyOneStats(struct FiftyOneStatsState *state, const char *filename);

/**
 * @brief Writes the statistics to a file, in binary format
 *
 * @param state The state of the statistics module
 * @param filename The path to the file where the statistics will be written
 * */
void dumpSelfishStats(struct SelfishStatsState *state, const char *filename);

/**
 * @brief Writes the statistics to a file, in binary format
 *
 * @param state The state of the statistics module
 * @param filename The path to the file where the statistics will be written
 * */
void dumpDetailedStatsBinary(struct DetailedStatsState *state, const char *filename);

/**
 * @brief Writes the statistics to a file, in binary format
 *
 * @param state The state of the statistics module
 * @param filename The path to the file where the statistics will be written
 * */
void dumpFiftyOneStatsBinary(struct FiftyOneStatsState *state, const char *filename);

/**
 * @brief Writes the statistics to a file, in binary format
 *
 * @param state The state of the statistics module
 * @param filename The path to the file where the statistics will be written
 * */
void dumpSelfishStatsBinary(struct SelfishStatsState *state, const char *filename);

/**
 * @brief Prints all the statistics to stdout
 *
 * @param state The state of the statistics module
 * */
void printDetailedStats(struct DetailedStatsState *state);

/**
 * @brief Prints all the statistics to stdout
 *
 * @param state The state of the statistics module
 * */
void printFiftyOneStats(struct FiftyOneStatsState *state);

/**
 * @brief Prints all the statistics to stdout
 *
 * @param state The state of the statistics module
 * */
void printSelfishStats(struct SelfishStatsState *state);

/**
 * @brief Prints all the statistics to stdout
 *
 * @param state The state of the statistics module
 * @param buffer The buffer to write the statistics to
 * @param buffer_size The size of the buffer
 *
 * @returns The number of characters written to the buffer
 * */
size_t sprintSelfishStats(struct SelfishStatsState *state, char *buffer, size_t buffer_size);

/**
 * @brief Prints the header of the statistics to stdout
 *
 * @param buffer The buffer to write the statistics to
 * @param buffer_size The size of the buffer
 *
 * @returns The number of characters written to the buffer
 * */
size_t sprintSelfishStatsHeader(char *buffer, size_t buffer_size);
#pragma once

#include "RBlockSim.h"
#include "Topology.h"
#include "Block.h"
#include "Node.h"

/**
 * Responsible for everything network-related
 * Manages transmission delay calculation, geographical regions
 * */

/**
 * @brief Return the time required to transmit data from src to dst
 *
 * @param sender Sender node
 * @param receiver Destination node
 * @param data_size Size of data to transfer
 * @param rng Random Number Generator state
 *
 * @return Transmission delay
 */
simtime_t getTransmissionDelay(node_id_t sender, node_id_t receiver, size_t data_size, struct rng_t* rng);

/**
 * @brief Propagates the newly generated block with the chosen approach
 * @param sender The ID of the node generating the block
 * @param send_time The time at which the block generation is triggered
 * @param block Pointer to the Block to propagate
 * @param rng Random Number Generator state
 *
 * Takes care of propagating the block to OTHER NODES using the chosen propagation algorithm
 */
void propagateBlock(node_id_t sender, simtime_t send_time, const struct Block *block, struct rng_t *rng);

/**
 * @brief Sends a single block from \a sender to \a receiver
 * @param sender The ID of the node sending the block
 * @param receiver The ID of the node receiving the block
 * @param send_time The time at which the block sending starts
 * @param block Pointer to the Block to send
 * @param rng Random Number Generator state
 * @param evt_type The type of event to schedule
 */
void sendSingleBlock(node_id_t sender, node_id_t receiver, simtime_t send_time, struct Block *block, struct rng_t *rng, enum rblocksim_event evt_type);

/**
 * @brief Initializes the network
 */
void initNetwork();

/**
 * @brief Deinitializes the network
 */
void deinitNetwork();

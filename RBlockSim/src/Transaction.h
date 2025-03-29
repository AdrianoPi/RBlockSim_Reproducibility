#pragma once

#include "RBlockSim.h"
#include "../external/bitmap.h"

struct Transaction {
    simtime_t timestamp;
    size_t size; // In Bytes
    double fee;  // Or reward
    txn_id_t id;
    node_id_t sender;
    // double gasPrice; // The cost of gas
    // double gasUsed;  // The cost, in gas, of executing the transaction. Received currency is gasUsed * gasPrice
};

extern atomic_bool txns_initialized;
extern struct Transaction transactions[TXN_NUMBER]; // FIXME INITIALIZE!!

#define TXN_DATA_MIN_TXNS 16
#define TXN_DATA_MIN_BITMAP_SIZE bitmap_required_size(TXN_DATA_MIN_TXNS)

struct TransactionData {
    int low;
    int high;
    block_bitmap included_transactions[TXN_DATA_MIN_BITMAP_SIZE];
    block_bitmap included_transactions_ext[];
};

/// Struct holding the relevant information for transaction management
struct TransactionState {
    block_bitmap *transactions_bitmap;  ///< Bitmap tracking transaction execution. 1 Means available, 0 means used
    int low;                            ///< index of the first transaction still available
    int high;                           ///< index of the latest seen transaction
};

/**
 * @brief Calculates the size in Bytes of an instance of TransactionData
 *
 * @param[in] data the TransactionData instance of which the size is required
 */
size_t sizeofTransactionData(const struct TransactionData *data);

/**
 * @brief Calculates the size in Bytes of the additional buffer used in a TransactionData instance
 *
 * @param transactions_count the number of transactions to include
 *
 * @return the size of the additional memory required
 */
size_t sizeofAdditionalTransactionDataBuffer(size_t transactions_count);

/**
 * @brief Generates all transactions, for the entire simulation
 * TODO
 */
void generateTransactions(struct rng_t *rng);
/**
 * @brief Generates a transaction
 * TODO
 */
void generateTransaction();

/**
 * @brief Validates a transaction
 *
 * @param[in] transaction the transaction to validate
 * @param[out] is_valid set to True if the transaction is valid
 *
 * @return Transaction validation delay. @a is_valid is true if the transaction is valid
 */
simtime_t validateTransaction(struct Transaction *transaction, bool *is_valid);

/**
 * @brief Applies the effect of @a transactionData on @a transactionState
 *
 * @param[in] transactionState pointer to the TransactionState to apply the transactions to
 * @param[in/out] transactionData pointer to the block's transactionData
 */
void applyBlockTransactions(struct TransactionState *transactionState, struct TransactionData *transactionData);

/**
 * @brief Reverts the effects of applying @a transactionData from @a transactionState
 *
 * @param[in] transactionState pointer to the TransactionState to revert from
 * @param[in/out] transactionData pointer to the block's transactionData to revert
 */
void revertAppliedBlockTransactions(struct TransactionState *transactionState, struct TransactionData *transactionData);

/**
 * @brief Computes how long transaction execution will take
 *
 * @param[in] transaction the transaction to execute
 *
 * @return Transaction execution delay
 */
simtime_t executeTransaction(struct Transaction *transaction);

/**
 * @brief Selects transactions to include in a block and creates TransactionData object
 *
 * @param[in,out] state the transaction management state
 * @param now the current time
 * @param me the id of the node generating transactionData
 */
struct TransactionData *generateTransactionData(struct TransactionState *state, simtime_t now, node_id_t me);

/**
 * @brief Computes the transaction reward
 *
 * @param[in] transaction the transaction for which the reward is requested
 *
 * @return The transaction's reward
 *
 * TODO
 */
double getTransactionReward(struct Transaction *transaction);

/**
 * @brief Initializes a TransactionState
 *
 * @param state Pointer to the TransactionState to initialize
 */
void initTransactionState(struct TransactionState *state);

/**
 * @brief Deinitializes TransactionState \a state
 *
 * @param state Pointer to the TransactionState to initialize
 */
void deinitTransactionState(struct TransactionState *state);

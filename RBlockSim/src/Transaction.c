#include "Transaction.h"

#ifndef NDEBUG

#include "util.h"
#include "Network.h"

#endif

atomic_bool txns_initialized = false;
struct Transaction transactions[TXN_NUMBER];

size_t sizeofAdditionalTransactionDataBuffer(size_t transactions_count) {
    int b_size = bitmap_required_size(transactions_count);
    int res = b_size - (int) TXN_DATA_MIN_BITMAP_SIZE;
    return res > 0 ? res : 0;
}

size_t sizeofTransactionData(const struct TransactionData *data) {
    return sizeof(struct TransactionData) + sizeofAdditionalTransactionDataBuffer(data->high - data->low);
}

struct TransactionData *allocTransactionData(size_t transactions_count) {
    size_t size = sizeof(struct TransactionData) + sizeofAdditionalTransactionDataBuffer(transactions_count);
    struct TransactionData *res = rs_malloc(size);
    bitmap_initialize(res->included_transactions, transactions_count);
    return res;
}

void initTransactionState(struct TransactionState *state) {
    size_t b_size = bitmap_required_size(TXN_NUMBER);
    state->transactions_bitmap = rs_malloc(b_size);
    bitmap_initialize(state->transactions_bitmap, TXN_NUMBER);
    state->low = 0;
    state->high = 0;
#ifndef NDEBUG
    bitmap_check_aux(state->transactions_bitmap, 0);
#endif
}

void deinitTransactionState(struct TransactionState *state) {
    rs_free(state->transactions_bitmap);
}

void generateTransactions(struct rng_t *rng) {
    simtime_t increment = ((double) TXN_NUMBER) / TERMINATION_TIME;
    for (size_t i = 0; i < TXN_NUMBER; i++) {
        transactions[i].timestamp = (double) i * increment;
        transactions[i].sender = (node_id_t) Random(rng) * conf.lps;
        transactions[i].size = i;
        transactions[i].id = i;
        transactions[i].fee = (double) i;
    }
}

void markTransactionExecuted(block_bitmap *transactions_bitmap, txn_id_t transaction_id) {
    bitmap_set(transactions_bitmap, transaction_id);
}

void markTransactionAvailable(block_bitmap *transactions_bitmap, txn_id_t transaction_id) {
    bitmap_reset(transactions_bitmap, transaction_id);
}

simtime_t getTransactionDeliveryTime(txn_id_t transactionId, node_id_t receiver) {
    struct Transaction *txn = &transactions[transactionId];
    return txn->timestamp + getTransmissionDelay(txn->sender, receiver, txn->size, NULL);
}

/**
 * @brief Updates the node's local view of transactions
 *
 * @param[in,out] state the transactionState
 * @param now The current time of the view
 */
void deliverNewTransactions(struct TransactionState *state, simtime_t now) {
    while (bitmap_check(state->transactions_bitmap, state->low) && state->low < TXN_NUMBER) {
        state->low++;
    }

    int i = state->high;
    if (i < 0) i = 0;
    while (i < TXN_NUMBER) {
        if (transactions[i].timestamp > now) {
            break;
        }
        i++;
    }
    state->high = i;
}

bool printed = false;

struct TransactionData *generateTransactionData(struct TransactionState *state, simtime_t now, node_id_t me) {
    if (!state) return NULL;
    deliverNewTransactions(state, now);
    if (state->high <= state->low) return NULL; // No available transactions

    struct TransactionData *data = allocTransactionData(state->high - state->low); // Naive implementation for now
    data->low = state->low;
    data->high = data->low;
    for (int i = state->low, j = 0; i < state->high; i++, j++) {
        if (!bitmap_check(state->transactions_bitmap, i) &&
            (transactions[i].sender == me || getTransactionDeliveryTime(i, me) < now)) {
            data->high = i;
            bitmap_set(data->included_transactions, j);
            // No need to set state->transactions_bitmap as it is set when including the Block
        }
    }
    if (data->high != data->low)
        data->high++; // high is first unseen
    return data;
}

void applyBlockTransactions(struct TransactionState *state, struct TransactionData *data) {
    for (int i = data->low; i < data->high; i++) {
        if (bitmap_check(data->included_transactions, i - data->low)) {
            markTransactionExecuted(state->transactions_bitmap, i);
        }
    }
    state->high = state->high > data->high ? state->high : data->high;
}

void revertAppliedBlockTransactions(struct TransactionState *state, struct TransactionData *data) {
    for (int i = data->low; i < data->high; i++) {
        if (bitmap_check(data->included_transactions, i - data->low)) {
            markTransactionAvailable(state->transactions_bitmap, i);
        }
    }
    state->low = state->low < data->low ? state->low : data->low;
}

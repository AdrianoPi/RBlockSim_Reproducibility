#include <stdio.h>
#include <ROOT-Sim/random.h>
#include <assert.h>
#include <string.h>
#include "../src/Transaction.h"

extern struct rng_t *init_rng();

extern struct TransactionData *allocTransactionData(size_t transactions_count);

extern void markTransactionExecuted(block_bitmap *transactions_bitmap, txn_id_t transaction_id);

extern void markTransactionAvailable(block_bitmap *transactions_bitmap, txn_id_t transaction_id);

extern simtime_t getTransactionDeliveryTime(txn_id_t transactionId, node_id_t receiver);

extern void deliverNewTransactions(struct TransactionState *state, simtime_t now);

extern void initNetwork();

void testInitTransactionState() {
    printf("Testing testInitTransactionState... ");
    fflush(stdout);

    struct TransactionState s;
    initTransactionState(&s);
    assert(s.transactions_bitmap);
    assert(!s.transactions_bitmap[0]);
    assert(!s.low);
    assert(!s.high);

    deinitTransactionState(&s);
    printf("SUCCESS\n");
}

void testGenerateTransactions() {
    printf("Testing testGenerateTransactions... ");
    fflush(stdout);

    struct rng_t *rng = init_rng();
    memset(&transactions, 0, sizeof(struct Transaction) * TXN_NUMBER);
    generateTransactions(rng);
    for (size_t i = 0; i < TXN_NUMBER; i++) {
        struct Transaction *txn = &transactions[i];
        assert(txn->id == i);
        assert(txn->timestamp >= 0.0);
        assert(txn->fee >= 0.0);
    }
    free(rng);

    printf("SUCCESS\n");
}

void testMarkTransactionExecuted() {
    printf("Testing testMarkTransactionExecuted... ");
    fflush(stdout);
    size_t max = 50;
    block_bitmap *bitmap = malloc(bitmap_required_size(max));
    bitmap_initialize(bitmap, max);
    for (size_t i = 0; i < max; i++) {
        markTransactionExecuted(bitmap, i);

        size_t j = 0;
        while (j <= i) {
            assert(bitmap_check(bitmap, j));
            j++;
        }
        while (j < max) {
            assert(!bitmap_check(bitmap, j));
            j++;
        }
    }

    free(bitmap);
    printf("SUCCESS\n");
}

void testMarkTransactionAvailable() {
    printf("Testing testMarkTransactionAvailable... ");
    fflush(stdout);

    size_t max = 50;
    block_bitmap *bitmap = malloc(bitmap_required_size(max));
    memset(bitmap, 0xFF, sizeof(block_bitmap) * bitmap_required_size(max));
    for (size_t i = max - 1; i < max; i--) {
        markTransactionAvailable(bitmap, i);

        size_t j = 0;
        while (j < i) {
            assert(bitmap_check(bitmap, j));
            j++;
        }
        while (j < max) {
            assert(!bitmap_check(bitmap, j));
            j++;
        }
    }

    free(bitmap);
    printf("SUCCESS\n");
}

void testGetTransactionDeliveryTime() {
    printf("Testing testGetTransactionDeliveryTime... ");
    fflush(stdout);

    initNetwork();

    assert(transactions[0].timestamp <= getTransactionDeliveryTime(0, 0));

    printf("SUCCESS\n");
}

void testDeliverNewTransactions() {
    printf("Testing testDeliverNewTransactions... ");
    fflush(stdout);

    int testct = 10;
    // Populate array of transactions in specific way
    assert(TXN_NUMBER > testct);
    for (size_t i = 0; i < TXN_NUMBER; i++) {
        transactions[i].timestamp = (double) i;
    }

    // Initialize transactionState
    struct TransactionState state;
    initTransactionState(&state);
    assert(state.low == 0);
    assert(state.high == 0);

    // Edge case: no transactions received
    deliverNewTransactions(&state, -.1);
    assert(state.low == 0);
    assert(state.high == 0);
    for (int i = 0; i < testct; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    // Deliver transactions passing a value of now such that I know which transactions will be delivered
    int latest = 3;

    // Deliver transactions up to txn 3
    deliverNewTransactions(&state, .1 + latest);

    assert(state.low == 0);
    assert(state.high == latest + 1);
    for (int i = 0; i < latest; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    // No change in TransactionState
    deliverNewTransactions(&state, .1 + latest);
    assert(state.low == 0);
    assert(state.high == latest + 1);
    for (int i = 0; i < latest; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    // Deliver up to txn 4
    deliverNewTransactions(&state, .1 + ++latest);
    assert(state.low == 0);
    assert(state.high == latest + 1);
    for (int i = 0; i < latest; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    // Mark txn 2 as executed. deliverNewTransactions should NOT update low and high
    markTransactionExecuted(state.transactions_bitmap, 2);
    deliverNewTransactions(&state, .1 + latest);
    assert(state.low == 0);
    assert(state.high == latest + 1);
    for (int i = 0; i < 2; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }
    assert(bitmap_check(state.transactions_bitmap, 2));
    for (int i = 3; i < latest; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    // Mark txn 0 as executed. deliverNewTransactions should update low to move over txn 0
    markTransactionExecuted(state.transactions_bitmap, 0);
    deliverNewTransactions(&state, .1 + latest);
    assert(state.low == 1);
    assert(state.high == latest + 1);
    assert(bitmap_check(state.transactions_bitmap, 0));
    assert(!bitmap_check(state.transactions_bitmap, 1));
    assert(bitmap_check(state.transactions_bitmap, 2));
    for (int i = 3; i < latest; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    // Mark txn 1 as executed. deliverNewTransactions should update low to txn 3, as all before it are executed
    markTransactionExecuted(state.transactions_bitmap, 1);
    deliverNewTransactions(&state, .1 + latest);
    assert(state.low == 3);
    assert(state.high == latest + 1);
    for (int i = 0; i < 3; i++) {
        assert(bitmap_check(state.transactions_bitmap, i));
    }
    for (int i = 3; i < latest; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    // Edge case: last transaction
    deliverNewTransactions(&state, .1 + TXN_NUMBER);
    assert(state.high == TXN_NUMBER);

    deinitTransactionState(&state);
    printf("SUCCESS\n");
}

void testGenerateTransactionData() {
    printf("Testing testGenerateTransactionData... ");
    fflush(stdout);

    assert(!generateTransactionData(NULL, 0, 0));
    assert(!generateTransactionData(NULL, 3, 0));

    // Generate transactions
    int testct = 10;
    // Populate array of transactions in specific way
    assert(TXN_NUMBER > testct);
    for (size_t i = 0; i < TXN_NUMBER; i++) {
        transactions[i].timestamp = (double) i;
        transactions[i].sender = 0;
    }

    // Initialize transactionState
    struct TransactionState state;
    initTransactionState(&state);
    assert(state.low == 0);
    assert(state.high == 0);
    assert(!generateTransactionData(NULL, -1.0, 0));

    // Generate TransactionsData
    struct TransactionData *txData = generateTransactionData(&state, 3.0, 0);
    assert(txData);
    assert(txData->low == 0);
    assert(txData->high == 4);
    for (int i = 0; i <= 3; i++) {
        assert(bitmap_check(txData->included_transactions, i));
        assert(!bitmap_check(state.transactions_bitmap, i));
    }
    assert(!bitmap_check(txData->included_transactions, 4));
    free(txData);


    // Reset transaction state
    deinitTransactionState(&state);
    initTransactionState(&state);
    // Node 1 has not generated transaction with ID 3, as such it will have some delay. It should not use it.
    txData = generateTransactionData(&state, 3.0, 1);
    assert(txData);
    assert(txData->low == 0);
    assert(txData->high == 3);
    free(txData);

    // Set transactions so that they are seen out of order
    transactions[2].timestamp = 2.0;
    transactions[2].sender = 1;
    transactions[3].timestamp = 2.001;
    transactions[3].sender = 0;
    // Reset transaction state
    deinitTransactionState(&state);
    initTransactionState(&state);
    // Node 0 has not generated transaction with ID 2, as such it will have some delay. It should use txn 3 but not txn 2
    txData = generateTransactionData(&state, 2.002, 0);
    assert(txData);
    assert(txData->low == 0);
    assert(txData->high == 4);
    assert(bitmap_check(txData->included_transactions, 0));
    assert(bitmap_check(txData->included_transactions, 1));
    assert(!bitmap_check(txData->included_transactions, 2));
    assert(bitmap_check(txData->included_transactions, 3));
    for (int i = 0; i <= 3; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }
    free(txData);

    // Final test: transaction is last of all transactions
    txData = generateTransactionData(&state, TXN_NUMBER + 10, 0);
    assert(bitmap_check(txData->included_transactions, TXN_NUMBER - 1));
    assert(!bitmap_check(txData->included_transactions, TXN_NUMBER)); // Technically out of bounds, but the bitmap will likely have some trailing unused bits
    assert(!txData->low);
    assert(txData->high == TXN_NUMBER);
    for (int i = 0; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
        assert(bitmap_check(txData->included_transactions, i));
    }
    free(txData);

    deinitTransactionState(&state);
    printf("SUCCESS\n");
}

void testApplyBlockTransactions() {
    printf("Testing applyBlockTransactions... ");
    fflush(stdout);

    struct TransactionState state;
    struct TransactionData *data;
    const int tcount = 10;
    assert(TXN_NUMBER > tcount);

    // TEST 1: contiguous
    // Initialize a TransactionState
    initTransactionState(&state);
    assert(state.low == 0);
    assert(state.high == 0);

    // Generate an artificially populated TransactionData
    data = allocTransactionData(tcount);
    data->low = 0;
    data->high = tcount;
    for (int i = 0; i < tcount; i++) {
        bitmap_set(data->included_transactions, i);
    }

    // Apply the TransactionData to the state
    applyBlockTransactions(&state, data);

    // Verify the application was successful
    for (size_t i = 0; i < tcount; i++) {
        assert(bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = tcount; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    deinitTransactionState(&state);
    free(data);


    // TEST 2: non-contiguous transactions
    initTransactionState(&state);
    assert(state.low == 0);
    assert(state.high == 0);

    data = allocTransactionData(tcount);
    data->low = 0;
    data->high = tcount;
    for (int i = 0; i < tcount; i += 2) {
        bitmap_set(data->included_transactions, i);
    }

    applyBlockTransactions(&state, data);

    for (size_t i = 0; i < tcount; i += 2) {
        assert(bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = 1; i < tcount; i += 2) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = tcount; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    deinitTransactionState(&state);
    free(data);


    // TEST 3: no transactions selected
    initTransactionState(&state);
    assert(state.low == 0);
    assert(state.high == 0);

    data = allocTransactionData(tcount);
    data->low = 0;
    data->high = tcount;

    applyBlockTransactions(&state, data);

    for (size_t i = 0; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    deinitTransactionState(&state);
    free(data);

    // TEST 4: Apply one, then another, then revert first
    initTransactionState(&state);
    assert(state.low == 0);
    assert(state.high == 0);

    data = allocTransactionData(tcount);
    data->low = 0;
    data->high = tcount;
    for (int i = 0; i < tcount; i += 2) {
        bitmap_set(data->included_transactions, i);
    }

    applyBlockTransactions(&state, data);

    for (size_t i = 0; i < tcount; i += 2) {
        assert(bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = 1; i < tcount; i += 2) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = tcount; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    struct TransactionData *data2 = allocTransactionData(tcount);
    data2->low = 0;
    data2->high = tcount;
    for (int i = 1; i < tcount; i += 2) {
        bitmap_set(data2->included_transactions, i);
    }

    applyBlockTransactions(&state, data2);

    for (size_t i = 0; i < tcount; i++) {
        assert(bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = tcount; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    deinitTransactionState(&state);
    free(data);
    free(data2);

    printf("SUCCESS\n");
}

void testRevertAppliedBlockTransactions() {
    printf("Testing revertAppliedBlockTransactions... ");
    fflush(stdout);

    struct TransactionState state;
    struct TransactionData *data;
    const int tcount = 10;
    assert(TXN_NUMBER > tcount);

    // TEST 1: contiguous
    // Initialize a TransactionState
    initTransactionState(&state);
    assert(state.low == 0);
    assert(state.high == 0);

    // Generate an artificially populated TransactionData
    data = allocTransactionData(tcount);
    data->low = 0;
    data->high = tcount;
    for (int i = 0; i < tcount; i++) {
        bitmap_set(data->included_transactions, i);
    }

    // Apply the TransactionData to the state
    applyBlockTransactions(&state, data);

    // Verify the application was successful
    for (size_t i = 0; i < tcount; i++) {
        assert(bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = tcount; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    // Revert the application
    revertAppliedBlockTransactions(&state, data);

    // Verify the effects are reverted
    for (size_t i = 0; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    deinitTransactionState(&state);
    free(data);


    // TEST 2: non-contiguous transactions
    initTransactionState(&state);
    assert(state.low == 0);
    assert(state.high == 0);

    data = allocTransactionData(tcount);
    data->low = 0;
    data->high = tcount;
    for (int i = 0; i < tcount; i += 2) {
        bitmap_set(data->included_transactions, i);
    }

    applyBlockTransactions(&state, data);

    for (size_t i = 0; i < tcount; i += 2) {
        assert(bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = 1; i < tcount; i += 2) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = tcount; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    revertAppliedBlockTransactions(&state, data);

    for (size_t i = 0; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    deinitTransactionState(&state);
    free(data);


    // TEST 3: no transactions selected
    initTransactionState(&state);
    assert(state.low == 0);
    assert(state.high == 0);

    data = allocTransactionData(tcount);
    data->low = 0;
    data->high = tcount;

    applyBlockTransactions(&state, data);

    for (size_t i = 0; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    revertAppliedBlockTransactions(&state, data);

    // Verify the effects are reverted
    for (size_t i = 0; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    deinitTransactionState(&state);
    free(data);

    // TEST 4: Apply one, then another, then revert first
    initTransactionState(&state);
    assert(state.low == 0);
    assert(state.high == 0);

    data = allocTransactionData(tcount);
    data->low = 0;
    data->high = tcount;
    for (int i = 0; i < tcount; i += 2) {
        bitmap_set(data->included_transactions, i);
    }

    applyBlockTransactions(&state, data);

    for (size_t i = 0; i < tcount; i += 2) {
        assert(bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = 1; i < tcount; i += 2) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = tcount; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    struct TransactionData *data2 = allocTransactionData(tcount);
    data2->low = 0;
    data2->high = tcount;
    for (int i = 1; i < tcount; i += 2) {
        bitmap_set(data2->included_transactions, i);
    }

    applyBlockTransactions(&state, data2);

    for (size_t i = 0; i < tcount; i++) {
        assert(bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = tcount; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    revertAppliedBlockTransactions(&state, data);

    // Verify the effects are reverted
    for (size_t i = 0; i < tcount; i += 2) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = 1; i < tcount; i += 2) {
        assert(bitmap_check(state.transactions_bitmap, i));
    }

    for (size_t i = tcount; i < TXN_NUMBER; i++) {
        assert(!bitmap_check(state.transactions_bitmap, i));
    }

    deinitTransactionState(&state);
    free(data);
    free(data2);

    printf("SUCCESS\n");
}

void testAllocTransactionData() {
    printf("Testing allocTransactionData... ");
    fflush(stdout);

    struct TransactionData *data;

    // Empty
    data = allocTransactionData(0);
    data->low = 0;
    data->high = -1;
    for (int i = 0; i < TXN_DATA_MIN_TXNS; i++) {
        bitmap_set(data->included_transactions, i);
        assert(bitmap_check(data->included_transactions, i));
    }
    free(data);

    // Transactions without extra space
    data = allocTransactionData(TXN_DATA_MIN_TXNS);
    data->low = 0;
    data->high = TXN_DATA_MIN_TXNS - 1;
    for (int i = 0; i < TXN_DATA_MIN_TXNS; i++) {
        bitmap_set(data->included_transactions, i);
        assert(bitmap_check(data->included_transactions, i));
    }
    free(data);

    int min_txns_using_extra_space = TXN_DATA_MIN_TXNS;
    while (bitmap_required_size(min_txns_using_extra_space) == TXN_DATA_MIN_BITMAP_SIZE) {
        min_txns_using_extra_space++;
    }
    // Largest number of transactions without extra space
    data = allocTransactionData(min_txns_using_extra_space - 1);
    data->low = 0;
    data->high = min_txns_using_extra_space - 1;
    for (int i = 0; i < data->high; i++) {
        bitmap_set(data->included_transactions, i);
        assert(bitmap_check(data->included_transactions, i));
    }
#ifdef __SANITIZE_ADDRESS__
#ifndef OOB_BITMAP_1
    fprintf(stderr, "\nYou are building with ASAN. Consider building with `make OPTIONS='-DOOB_BITMAP_1'` to try and trigger an out-of-bounds access at %s:%d.\n", __FILE__, __LINE__ + 3);
#else
    printf("\nYou are building -DOOB_BITMAP_1 to try and trigger an out-of-bounds access at %s:%d. ASAN should now block on a heap-buffer-overflow.\n", __FILE__, __LINE__ + 1);
    bitmap_set(data->included_transactions, data->high); // Does nothing per se, is expected to trigger ASAN
    fprintf(stderr, "\nASAN did not trigger. What happened?\n");
    abort();
#endif
#endif
    free(data);

    // Smallest number of transactions using extra space
    data = allocTransactionData(min_txns_using_extra_space);
    data->low = 0;
    data->high = min_txns_using_extra_space;
    for (int i = 0; i < data->high; i++) {
        bitmap_set(data->included_transactions, i);
        assert(bitmap_check(data->included_transactions, i));
    }
    free(data);

    printf("SUCCESS\n");
}

void testSizeofTransactionData() {
    printf("Testing testSizeofTransactionData... ");
    fflush(stdout);

    struct TransactionData *data;

    // Empty
    data = allocTransactionData(0);
    data->low = 0;
    data->high = -1;
    assert(sizeofTransactionData(data) == sizeof(struct TransactionData));
    free(data);

    // Transactions without extra space
    data = allocTransactionData(TXN_DATA_MIN_TXNS);
    data->low = 0;
    data->high = TXN_DATA_MIN_TXNS - 1;
    assert(sizeofTransactionData(data) == sizeof(struct TransactionData));
    bitmap_set(data->included_transactions, data->high);
    free(data);

    int min_txns_using_extra_space = TXN_DATA_MIN_TXNS;
    while (bitmap_required_size(min_txns_using_extra_space) == TXN_DATA_MIN_BITMAP_SIZE) {
        min_txns_using_extra_space++;
    }
    // Largest number of transactions without extra space
    data = allocTransactionData(min_txns_using_extra_space - 1);
    data->low = 0;
    data->high = min_txns_using_extra_space - 1;
    assert(sizeofTransactionData(data) == sizeof(struct TransactionData));
    bitmap_set(data->included_transactions, data->high - 1);
#ifdef __SANITIZE_ADDRESS__
#ifndef OOB_BITMAP_2
    fprintf(stderr, "\nYou are building with ASAN. Consider building with `make OPTIONS='-DOOB_BITMAP_2'` to try and trigger an out-of-bounds access at %s:%d.\n", __FILE__, __LINE__ + 3);
#else
    printf("\nYou are building -DOOB_BITMAP_2 to try and trigger an out-of-bounds access at %s:%d. ASAN should now block on a heap-buffer-overflow.\n", __FILE__, __LINE__ + 1);
    bitmap_set(data->included_transactions, data->high); // Does nothing per se, is expected to trigger ASAN
    fprintf(stderr, "\nASAN did not trigger. What happened?\n");
    abort();
#endif
#endif
    free(data);

    // Smallest number of transactions using extra space
    data = allocTransactionData(min_txns_using_extra_space);
    data->low = 0;
    data->high = min_txns_using_extra_space;
    assert(sizeofTransactionData(data) > sizeof(struct TransactionData));
    bitmap_set(data->included_transactions, data->high - 1);
    free(data);

    printf("SUCCESS\n");
}


void transaction_main() {
    printf("TESTING TRANSACTION\n");
    testInitTransactionState();
    testGenerateTransactions();
    testMarkTransactionExecuted();
    testMarkTransactionAvailable();
    testGetTransactionDeliveryTime();
    testDeliverNewTransactions();
    testGenerateTransactionData();
    testApplyBlockTransactions();
    testRevertAppliedBlockTransactions();
    testAllocTransactionData();
    testSizeofTransactionData();
    printf("FINISHED TESTING TRANSACTION, SUCCESS!\n");
}

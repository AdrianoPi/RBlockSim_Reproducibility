#include "Attacks.h"
#include "Config.h"
#include "../external/bitmap.h"

node_id_t *attacker_ids = NULL;
block_bitmap *is_attacker_bitmap = NULL;
size_t num_attackers = 0;
struct attack_config attackConfig = {.type = ATTACK_NONE};

/**
 * @brief Randomly populates the attackers array with attackers_count unique IDs
 *
 * @brief node_count The number of nodes in the network
 * @brief attackers_count The number of attackers in this simulation
 * @brief rng The random number generator state
 * */
void generateAttackers(size_t node_count, size_t attackers_count, struct rng_t *rng) {
    node_id_t idx_nodes, idx_attackers = 0;

    for (idx_nodes = 0; idx_nodes < node_count && idx_attackers < attackers_count; ++idx_nodes) {
        size_t remaining_nodes = node_count - idx_nodes;
        size_t remaining_attackers = attackers_count - idx_attackers;
        if ((RandomU64(rng) % remaining_nodes) < remaining_attackers) {
            attacker_ids[idx_attackers++] = idx_nodes;
            bitmap_set(is_attacker_bitmap, idx_nodes);
        }
    }
}

void initAttackers(size_t attackers_count, struct rng_t *rng) {
    size_t b_size = bitmap_required_size(conf.lps);
    is_attacker_bitmap = malloc(b_size);
    bitmap_initialize(is_attacker_bitmap, conf.lps);

    if (attackConfig.type == ATTACK_NONE || attackers_count == 0) {
        return;
    }

    if (attacker_ids) {
        fprintf(stderr, "Attackers already initialized\n");
        exit(EXIT_FAILURE);
    }

    if (attackConfig.type == ATTACK_SELFISH_MINING || attackConfig.type == ATTACK_FIFTY_ONE) {
        if (attackers_count != 1) {
            fprintf(stderr, "Selfish mining and 51%% attacks only use 1 attacker\n");
            exit(EXIT_FAILURE);
        }
    }

    if (attackers_count >= conf.lps) {
        fprintf(stderr, "Attackers must be less than the number of nodes. (Requested %lu attackers on %lu nodes)\n",
                attackers_count, conf.lps);
        exit(EXIT_FAILURE);
    }

    num_attackers = attackers_count;
    attacker_ids = malloc(num_attackers * sizeof(node_id_t));
    if (!attacker_ids) {
        fprintf(stderr, "Failed to allocate memory for attackers\n");
        abort();
    }

    if (attackConfig.type == ATTACK_SELFISH_MINING || attackConfig.type == ATTACK_FIFTY_ONE) {
        attacker_ids[0] = RandomU64(rng) % conf.lps;
        bitmap_set(is_attacker_bitmap, attacker_ids[0]);
        return;
    }

    generateAttackers(conf.lps, num_attackers, rng);
}

void deinitAttackers() {
    free(attacker_ids);
    free(is_attacker_bitmap);
}

bool is_attacker(node_id_t node_id) {
    return bitmap_check(is_attacker_bitmap, node_id);
}

void printAttackers() {
    if (attackConfig.type == ATTACK_NONE) {
        printf("No attackers\n");
        return;
    }

    printf("Attackers: [");
    for (size_t i = 0; i < num_attackers; i++) {
        printf("%u, ", attacker_ids[i]);
    }
    printf("]\n");
}

#include "util.h"

/* Draws a number from a Normal distribution with `mean` mean and `std_dev` standard deviation */
double NormalExpanded(struct rng_t *ctx, double mean, double std_dev) {
    return Normal(ctx) * std_dev + mean;
}

int bitmap_check_aux(block_bitmap *bitmap, size_t bit_index) {
    return bitmap_check(bitmap, bit_index);
}
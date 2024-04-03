#pragma once

#include <stdint.h>

#include "jvs/jvs-bus.h"

struct najv_switch_state {
    uint8_t system;
    uint16_t p1;
};

struct najv_ops {
    void (*reset)(void *ctx);
    void (*write_gpio)(void *ctx, uint32_t state);
    void (*read_switches)(void *ctx, struct najv_switch_state *out);
    void (*read_analogs)(void *ctx, uint16_t *analogs, uint8_t nanalogs);
    void (*read_rotary)(void *ctx, uint16_t *rotary, uint8_t nrotary);
    void (*read_coin_counter)(void *ctx, uint8_t slot_no, uint16_t *out);
};

struct najv {
    struct jvs_node jvs;
    uint8_t addr;
    const struct najv_ops *ops;
    void *ops_ctx;
};

void najv_init(
        struct najv *najv,
        struct jvs_node *next,
        const struct najv_ops *ops,
        void *ops_ctx);

struct jvs_node *najv_to_jvs_node(struct najv *najv);

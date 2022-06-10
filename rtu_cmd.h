#pragma once

#include <stddef.h>

#include <modbus_c/rtu.h>
#include <modbus_c/rtu_memory.h>

#ifndef RTU_ADDR_BASE
#error "Please define RTU_ADDR_BASE"
#endif

typedef struct
{
    union
    {
        uint8_t data[sizeof(rtu_memory_t)];
        rtu_memory_t header;
    } rtu_memory;
    char tlog[TLOG_SIZE];
} rtu_memory_fields_t;

void rtu_memory_fields_clear(rtu_memory_fields_t *);
void rtu_memory_fields_init(rtu_memory_fields_t *);

uint8_t *rtu_pdu_cb(
    modbus_rtu_state_t *state,
    modbus_rtu_addr_t addr,
    modbus_rtu_fcode_t fcode,
    const uint8_t *begin, const uint8_t *end,
    const uint8_t *curr,
    uint8_t *dst_begin, const uint8_t *const dst_end,
    uintptr_t user_data);

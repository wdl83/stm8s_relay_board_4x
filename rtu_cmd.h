#pragma once

#include <stddef.h>

#include <drv/stm8_portx.h>

#include <modbus_c/rtu.h>
#include <modbus_c/rtu_memory.h>

#ifndef RTU_ADDR_BASE
#error "Please define RTU_ADDR_BASE"
#endif

#define RELAY_NUM 4

typedef struct
{
    uint8_t state[RELAY_NUM];
} relay_ctl_t;

typedef struct
{
    rtu_memory_header_t header;
    uint16_t fw_crc16;                                   // 0
    port_data_t portA;                                   // 2
    port_data_t portB;                                   // 7
    port_data_t portC;                                   // 12
    port_data_t portD;                                   // 17
    port_data_t portE;                                   // 22
    port_data_t portF;                                   // 27
    relay_ctl_t relay_ctl;                               // 32
    char tlog[TLOG_SIZE];                                // 36
} rtu_memory_fields_t;

#define RTU_MEMORY_OFFSET(base, field) \
    ( \
        (uint8_t *)(base)->field - \
        ((uint8_t *)(base)->header + sizeof(rtu_memory_header_t)))

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

#pragma once

#include <stddef.h>

#include <drv/stm8_portx.h>

#include <modbus_c/rtu.h>
#include <modbus_c/rtu_memory.h>

#ifndef RTU_ADDR_BASE
#error "Please define RTU_ADDR_BASE"
#endif

/* this affects RTU ABI! */
#define RELAY_NUM 4

typedef struct
{
    uint8_t state[RELAY_NUM];
} relay_ctl_t;

typedef union
{
    struct
    {
        uint8_t RnW     : 1;
        /* set to request io operation */
        uint8_t PENDING : 1;
        /* set when request was served */
        uint8_t READY   : 1;
        uint8_t         : 1;
        uint8_t OR      : 1;
        uint8_t AND     : 1;
        uint8_t         : 2;
    } bits;

    struct
    {
        uint8_t : 4;
        uint8_t op: 2;
        uint8_t : 2;
    } opcode;
} io_mode_t;

/* RTU ABI */
typedef struct
{
    /*------------------------------------------------------------------------*/
    // offsets are calculated relative to header end               offset (size)
    rtu_memory_header_t header;
    /*------------------------------------------------------------------------*/
    /* calculated CRC16 checksum from current FLASH content
     * recalculated on every boot                                             */
    uint16_t fw_crc16;                                                  // 0 (2)
    /*------------------------------------------------------------------------*/
    /* I/O on any address
     * io_mode:
     *     RnW: 0    *io_addr  =   io_data
     *     RnW: 1     io_data  =  *io_addr
     *     OR:  1    *io_addr |=   io_data
     *     AND: 1    *io_addr &=   io_data
     * io_addr: address of io operation
     * io_data: data read or to be written */
    uint16_t io_addr;                                                   // 2 (2)
    io_mode_t io_mode;                                                  // 4 (1)
    uint8_t io_data;                                                    // 5 (1)
    /*------------------------------------------------------------------------*/
    /* relay control interface                                                */
    relay_ctl_t relay_ctl;                                              // 6 (4)
    /*------------------------------------------------------------------------*/
    /* Trouble/Trace Log
     * keep it as last member so its size wont affect RTU MEM ABI             */
    char tlog[TLOG_SIZE];                                               // 10 ()
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

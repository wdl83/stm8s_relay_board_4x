#include <string.h>
#include <stddef.h>

#include <drv/tlog.h>

#include <modbus_c/crc.h>

#include "rtu_cmd.h"

/* FLASH layout: [HOME|GSINIT|GSTFINAL|CONST|CODE|INITIALIZER]
 * WARNING:
 * SDCC does not allow to access linker defined symbols in C so asm is used */

static
uint16_t fw_begin(void) __naked
{
    __asm__("ldw x,#s_HOME");
    __asm__("ret");
}

static
uint16_t fw_end(void) __naked
{
    __asm__("ldw x,#s_HOME");
    __asm__("addw x,#l_HOME");
    __asm__("addw x,#l_GSINIT");
    __asm__("addw x,#l_GSFINAL");
    __asm__("addw x,#l_CONST");
    __asm__("addw x,#l_CODE");
    __asm__("addw x,#l_INITIALIZER");
    __asm__("ret");
}

static
uint16_t calc_fw_checksum(void)
{
    uint16_t crc16 = UINT16_C(0xFFFF);
    uint8_t *begin = (uint8_t*)fw_begin();
    const uint8_t *const end = (uint8_t *)(fw_end());

    while(begin != end) crc16 = crc16_update(crc16, *begin++);
    return crc16;
}

void rtu_memory_fields_clear(rtu_memory_fields_t *mem)
{
    memset(mem, 0, sizeof(rtu_memory_fields_t));
}

void rtu_memory_fields_init(rtu_memory_fields_t *mem)
{
    mem->header.addr_begin = RTU_ADDR_BASE;
    mem->header.addr_end =
        RTU_ADDR_BASE + sizeof(rtu_memory_fields_t) - sizeof(rtu_memory_t);
    mem->fw_crc16 = calc_fw_checksum();
}

uint8_t *rtu_pdu_cb(
    modbus_rtu_state_t *state,
    modbus_rtu_addr_t addr,
    modbus_rtu_fcode_t fcode,
    const uint8_t *begin, const uint8_t *end,
    /* curr == begin + sizeof(addr_t) + sizeof(fcode_t) */
    const uint8_t *curr,
    uint8_t *dst_begin, const uint8_t *const dst_end,
    uintptr_t user_data)
{
    rtu_memory_fields_t *mem = (rtu_memory_fields_t *)user_data;

    TLOG_XPRINT16("S|F", ((uint16_t)addr << 8) | fcode);

    /* because crossing rtu_err_reboot_threashold will cause
     * reboot decrese error count if valid PDU received */
    if(state->err_cntr) --state->err_cntr;

    if(modbus_rtu_addr(state) != addr) goto exit;

    *dst_begin++ = addr;

    dst_begin =
        rtu_memory_pdu_cb(
            (rtu_memory_t *)&mem->header,
            fcode,
            begin + sizeof(addr), end,
            curr,
            dst_begin, dst_end);
exit:
    return dst_begin;
}

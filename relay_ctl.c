#include <drv/clk.h>
#include <drv/mem.h>
#include <drv/stm8.h>
#include <drv/tim4.h>
#include <drv/tlog.h>
#include <drv/uart1_async_rx.h>
#include <drv/uart1_async_tx.h>

#include <modbus_c/stm8s003f3/rtu_impl.h>
#include <modbus_c/rtu.h>

#include "rtu_cmd.h"

#define RTU_ADDR UINT8_C(128)

char buf[32];
rtu_memory_fields_t mem;
modbus_rtu_state_t state;

void main(void)
{
    /* at startup master clock source is HSI / 8 (16MHz / 8 = 2MHz) */
    CLK_SWCR |= M1(SWEN);
    CLK_SWR = HSE_SRC;

    /* wait for clock source to switch */
    while((CLK_SWCR & M1(SWBSY)) || HSE_SRC != CLK_CMSR) {}


    rtu_memory_fields_clear(&mem);
    rtu_memory_fields_init(&mem);
    tlog_init(mem.tlog);

    TLOG_XPRINT16("TLOG", &mem.tlog - &mem.rtu_memory);

    modbus_rtu_impl(&state, RTU_ADDR, NULL, NULL, rtu_pdu_cb, (uintptr_t)&mem);

    for(;;)
    {
        INTERRUPT_DISABLE();
        modbus_rtu_event(&state);
        INTERRUPT_ENABLE();
        WAIT_FOR_INTERRUPT();
    }
}
/*-----------------------------------------------------------------------------*/

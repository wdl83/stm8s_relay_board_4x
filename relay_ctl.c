#include <drv/clk.h>
#include <drv/mem.h>
#include <drv/portB.h>
#include <drv/portC.h>
#include <drv/stm8.h>
#include <drv/tim2.h>
#include <drv/tim4.h>
#include <drv/tlog.h>
#include <drv/uart1_async_rx.h>
#include <drv/uart1_async_tx.h>

#include <modbus_c/rtu.h>
#include <modbus_c/stm8s003f3/rtu_impl.h>

#include "rtu_cmd.h"

#define RTU_ADDR UINT8_C(128)

/* Pin Mapping
 * source: https://github.com/TG9541/stm8ef/wiki/Board-C0135
 * 1   PD4	LED (1:on)
 * 2   PD5	Tx on header J5.2
 * 3   PD6	Rx on header J5.3, diode-OR with RS485
 * 4   NRST Reset on header J13.2
 * 5   PA1	OSCIN crystal
 * 6   PA2	OSCOUT crystal
 * 7   VSS	GND
 * 8   Vcap C
 * 9   VDD	+3.3V
 * 10  PA3	Key "S2" (0:pressed)
 * 11  PB5	RS485 driver enable (DE-/RE, R21 10k pull-up)
 * 12  PB4	Relay1 (0:on)
 * 13  PC3	Relay2 (0:on)
 * 14  PC4	Relay3 (0:on)
 * 15  PC5	Relay4 (0:on)
 * 16  PC6	IN4 (TIM1_CH1)
 * 17  PC7	IN3 (TIM1_CH2)
 * 18  PD1	SWIM on header J13.3
 * 19  PD2	IN2 (AIN3, TIM2_CH3)
 * 20  PD3	IN1 (AIN4, TIM2_CH2)
 * */

static
void relay_init(void)
{
    // Relay 1
    PB_ODR |= M1(3); // relay off
    PB_DDR |= M1(3); // output
    // Relay 2, 3, 4
    PC_ODR |= M3(3, 4, 5); // relay off
    PC_DDR |= M3(3, 4, 5); // output
}

static
void handle_relay(rtu_memory_fields_t *mem)
{
    relay_ctl_t *relay_ctl = &mem->relay_ctl;

    if(!relay_ctl->flags.PENDING) return;

    /* sanitize user input */
    for(uint8_t i = 0; RELAY_NUM > i; ++i) relay_ctl->state[i] &= 1;

    PB_ODR = PB_ODR & ~M1(3) | (!relay_ctl->state[0]) << 3;
    PC_ODR =
        PB_ODR & ~M3(3, 4, 5)
        | (!relay_ctl->state[1]) << 3
        | (!relay_ctl->state[2]) << 4
        | (!relay_ctl->state[3]) << 5;

    relay_ctl->flags.PENDING = 0;
    relay_ctl->flags.READY = 1;
}

/* PIT: periodic interrupt */
typedef struct
{
    uint16_t cntr;
    /* main event loop should clear 'updated' bit on every PIT interrupt
     * if its busy with modbus events - count number of skipped updates */
    uint16_t skip_cntr;

    union
    {
        uint8_t value;
        struct
        {
            uint8_t updated : 1;
            uint8_t : 7;
        } bits;
    } status;
} pit_t;

static
void periodic_timer_cb(uintptr_t user_data)
{
    TIM2_INT_CLEAR();
    pit_t *pit = (pit_t *)user_data;

    pit->skip_cntr += pit->status.bits.updated;
    ++pit->cntr;
    pit->status.bits.updated = 1;
    /* log every ~11min */
    if(!pit->cntr) TLOG_XPRINT16("PIT", pit->skip_cntr);
}

static
void periodic_timer_init(pit_t *pit)
{
    tim2_cb(periodic_timer_cb, (uintptr_t)pit);

    TIM2_AUTO_RELOAD_PRELOAD_ENABLE();
    // 8MHz = 8 * 10^6Hz / 8K = 976.5625Hz = 1.024ms  ~ 1ms
    TIM2_CLK_DIV_8K();
    TIM2_WR_TOP(10); // every ~10ms
    TIM2_WR_CNTR(0);
    TIM2_INT_ENABLE();
    TIM2_INT_CLEAR();
    TIM2_ENABLE();
}

static
void handle_io(rtu_memory_fields_t *mem)
{
    if(!mem->io_mode.bits.PENDING) return;

    uint8_t *data = (uint8_t *)mem->io_addr;

    if(mem->io_mode.bits.RnW) mem->io_data = *data;
    else if(mem->io_mode.bits.OR) *data |= mem->io_data;
    else if(mem->io_mode.bits.AND) *data &= mem->io_data;
    mem->io_mode.bits.PENDING = 0;
    mem->io_mode.bits.READY = 1;

    const char *mode[] = {"RD", "W|", "W&"};
    TLOG_XPRINT16("IO", (uint16_t)data);
    TLOG_XPRINT8(mode[mem->io_mode.opcode.op], *data);
}

static
void exec(rtu_memory_fields_t *mem, pit_t *pit)
{
    pit->status.bits.updated = 0;
    handle_io(mem);
    handle_relay(mem);
}

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
    relay_init();
    TLOG_XPRINT16("TLOG", RTU_MEMORY_OFFSET(&mem, tlog));
    TLOG_XPRINT16("FWCRC", mem.fw_crc16);

    modbus_rtu_impl(&state, RTU_ADDR, NULL, NULL, rtu_pdu_cb, (uintptr_t)&mem);

    pit_t pit = {0, 0, {0}};
    periodic_timer_init(&pit);

    for(;;)
    {
        INTERRUPT_DISABLE();
        modbus_rtu_event(&state);
        if(modbus_rtu_idle(&state)) exec(&mem, &pit);
        INTERRUPT_ENABLE();
        WAIT_FOR_INTERRUPT();
    }
}
/*-----------------------------------------------------------------------------*/

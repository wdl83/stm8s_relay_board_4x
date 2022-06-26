#include <drv/clk.h>
#include <drv/mem.h>
#include <drv/portB.h>
#include <drv/portC.h>
#include <drv/stm8.h>
#include <drv/tim4.h>
#include <drv/tlog.h>
#include <drv/uart1_async_rx.h>
#include <drv/uart1_async_tx.h>

#include <modbus_c/rtu.h>
#include <modbus_c/stm8s003f3/rtu_impl.h>

#include "rtu_cmd.h"

#define RTU_ADDR UINT8_C(128)

rtu_memory_fields_t mem;
modbus_rtu_state_t state;

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
void relay_sync(rtu_memory_fields_t *mem)
{
    relay_ctl_t *relay_ctl = &mem->relay_ctl;
    relay_ctl->state[0] = !(PB_ODR & M1(3));
    relay_ctl->state[1] = !(PC_ODR & M1(3));
    relay_ctl->state[2] = !(PC_ODR & M1(4));
    relay_ctl->state[3] = !(PC_ODR & M1(5));
}

static
void port_sync(rtu_memory_fields_t *mem)
{
    stm8_port_copy(&mem->portA, PORT_A_BASE);
    stm8_port_copy(&mem->portB, PORT_B_BASE);
    stm8_port_copy(&mem->portC, PORT_C_BASE);
    stm8_port_copy(&mem->portD, PORT_D_BASE);
    stm8_port_copy(&mem->portE, PORT_E_BASE);
    stm8_port_copy(&mem->portF, PORT_F_BASE);
}

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
    port_sync(&mem);
    TLOG_XPRINT16("TLOG", RTU_MEMORY_OFFSET(&mem, tlog));
    TLOG_XPRINT16("FWCRC", mem.fw_crc16);

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

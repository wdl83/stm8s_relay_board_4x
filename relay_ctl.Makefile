DRV = stm8_drv
MODBUS_C = modbus_c
BUILD_DIR = build

CPPFLAGS += -I.
CPPFLAGS += -I$(DRV)

include $(DRV)/Makefile.defs

CFLAGS += \
		  -DMODBUS_RTU_MEMORY_RD_HOLDING_REGISTERS_DISABLED \
		  -DMODBUS_RTU_MEMORY_WR_REGISTERS_DISABLED \
		  -DMODBUS_RTU_MEMORY_WR_REGISTER_DISABLED \
		  -DRTU_ADDR_BASE=0x1000 \
		  -DRTU_ERR_REBOOT_THREASHOLD=128 \
		  -DSTM8S003F3 \
		  -DTLOG_SIZE=64 \
		  -DUART1_RX_NO_BUFFERING

TARGET = relay_ctl

CSRCS = \
		relay_ctl.c \
		rtu_cmd.c

LIBS_CSRCS = \
		$(DRV)/drv/tim4.c \
		$(DRV)/drv/tlog.c \
		$(DRV)/drv/uart1_async_rx.c \
		$(DRV)/drv/uart1_async_tx.c \
		$(DRV)/drv/uart1_rx.c \
		$(DRV)/drv/uart1_tx.c \
		$(DRV)/drv/util.c \
		$(MODBUS_C)/rtu.c \
		$(MODBUS_C)/rtu_memory.c \
		$(MODBUS_C)/stm8s003f3/crc.c \
		$(MODBUS_C)/stm8s003f3/rtu_impl.c

include $(DRV)/Makefile.rules

clean:
	rm $(BUILD_DIR) -rf

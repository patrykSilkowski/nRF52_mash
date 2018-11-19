PROJECT_NAME     := app_pca10056
TARGETS          := nrf52840_xxaa
OUTPUT_DIRECTORY := build

SDK_ROOT := ./nRF5_SDK_for_Thread_and_Zigbee_2.0.0_29775ac
PROJ_DIR := ./app

$(OUTPUT_DIRECTORY)/nrf52840_xxaa.out: \
  LINKER_SCRIPT  := $(SDK_ROOT)/external/openthread/linker_scripts/openthread_nrf52840.ld

# Source files common to all targets
SRC_FILES += \
  $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52840.S \
  $(SDK_ROOT)/components/boards/boards.c \
  $(PROJ_DIR)/main.c \
  $(SDK_ROOT)/components/libraries/button/app_button.c \
  $(SDK_ROOT)/components/libraries/util/app_error.c \
  $(SDK_ROOT)/components/libraries/util/app_error_handler_gcc.c \
  $(SDK_ROOT)/components/libraries/util/app_error_weak.c \
  $(SDK_ROOT)/components/libraries/scheduler/app_scheduler.c \
  $(SDK_ROOT)/components/libraries/timer/app_timer.c \
  $(SDK_ROOT)/components/libraries/util/app_util_platform.c \
  $(SDK_ROOT)/components/libraries/assert/assert.c \
  $(SDK_ROOT)/components/libraries/mem_manager/mem_manager.c \
  $(SDK_ROOT)/components/libraries/util/nrf_assert.c \
  $(SDK_ROOT)/components/libraries/atomic/nrf_atomic.c \
  $(SDK_ROOT)/components/libraries/balloc/nrf_balloc.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf_format.c \
  $(SDK_ROOT)/components/libraries/memobj/nrf_memobj.c \
  $(SDK_ROOT)/components/libraries/queue/nrf_queue.c \
  $(SDK_ROOT)/components/libraries/ringbuf/nrf_ringbuf.c \
  $(SDK_ROOT)/components/libraries/strerror/nrf_strerror.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_rtt.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_serial.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_default_backends.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_frontend.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_str_formatter.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_clock.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_rng.c \
  $(SDK_ROOT)/components/drivers_nrf/nrf_soc_nosd/nrf_nvic.c \
  $(SDK_ROOT)/modules/nrfx/hal/nrf_nvmc.c \
  $(SDK_ROOT)/components/drivers_nrf/nrf_soc_nosd/nrf_soc.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_clock.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_gpiote.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_power_clock.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_rng.c \
  $(SDK_ROOT)/components/libraries/bsp/bsp.c \
  $(SDK_ROOT)/components/libraries/bsp/bsp_thread.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNConnectClient.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNConnectServer.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNDeserializePublish.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNPacket.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNSearchClient.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNSearchServer.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNSerializePublish.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNSubscribeClient.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNSubscribeServer.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNUnsubscribeClient.c \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet/MQTTSNUnsubscribeServer.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_Syscalls_GCC.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_printf.c \
  $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52840.c \
  $(SDK_ROOT)/components/thread/mqtt_sn/mqtt_sn_client/mqttsn_client.c \
  $(SDK_ROOT)/components/thread/mqtt_sn/mqtt_sn_client/mqttsn_gateway_discovery.c \
  $(SDK_ROOT)/components/thread/mqtt_sn/mqtt_sn_client/mqttsn_packet_fifo.c \
  $(SDK_ROOT)/components/thread/mqtt_sn/mqtt_sn_client/mqttsn_packet_receiver.c \
  $(SDK_ROOT)/components/thread/mqtt_sn/mqtt_sn_client/mqttsn_packet_sender.c \
  $(SDK_ROOT)/components/thread/mqtt_sn/mqtt_sn_client/mqttsn_platform.c \
  $(SDK_ROOT)/components/thread/mqtt_sn/mqtt_sn_client/mqttsn_transport_ot.c \
  $(SDK_ROOT)/components/thread/utils/thread_utils.c \

# Include folders common to all targets
INC_FOLDERS += \
  $(SDK_ROOT)/components \
  $(SDK_ROOT)/modules/nrfx/mdk \
  $(SDK_ROOT)/external/nrf_cc310/include \
  $(SDK_ROOT)/components/libraries/scheduler \
  $(SDK_ROOT)/components/libraries/queue \
  $(SDK_ROOT)/components/libraries/timer \
  $(SDK_ROOT)/components/libraries/strerror \
  $(SDK_ROOT)/external/paho/mqtt-sn/mqttsn_packet \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/components/libraries/mem_manager \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/external/openthread/include \
  ./config \
  $(SDK_ROOT)/components/libraries/balloc \
  $(SDK_ROOT)/components/libraries/ringbuf \
  $(SDK_ROOT)/modules/nrfx/hal \
  $(SDK_ROOT)/components/libraries/bsp \
  $(SDK_ROOT)/components/libraries/log \
  $(SDK_ROOT)/components/libraries/button \
  $(SDK_ROOT)/modules/nrfx \
  $(SDK_ROOT)/components/libraries/experimental_section_vars \
  $(SDK_ROOT)/integration/nrfx/legacy \
  $(PROJ_DIR) \
  $(SDK_ROOT)/components/libraries/delay \
  $(SDK_ROOT)/external/segger_rtt \
  $(SDK_ROOT)/components/drivers_nrf/nrf_soc_nosd \
  $(SDK_ROOT)/components/libraries/atomic \
  $(SDK_ROOT)/components/boards \
  $(SDK_ROOT)/components/libraries/memobj \
  $(SDK_ROOT)/components/thread/mqtt_sn/mqtt_sn_client \
  $(SDK_ROOT)/integration/nrfx \
  $(SDK_ROOT)/modules/nrfx/drivers/include \
  $(SDK_ROOT)/components/thread/utils \
  $(SDK_ROOT)/external/fprintf \
  $(SDK_ROOT)/components/libraries/log/src \

# Libraries common to all targets
LIB_FILES += \
  $(SDK_ROOT)/external/openthread/lib/gcc/libopenthread-cli-mtd.a \
  $(SDK_ROOT)/external/openthread/lib/gcc/libopenthread-diag.a \
  $(SDK_ROOT)/external/openthread/lib/gcc/libopenthread-mtd.a \
  $(SDK_ROOT)/external/openthread/lib/gcc/libopenthread-platform-utils.a \
  $(SDK_ROOT)/external/openthread/lib/gcc/libmbedcrypto.a \
  $(SDK_ROOT)/external/openthread/lib/gcc/libopenthread-nrf52840-sdk.a \
  $(SDK_ROOT)/external/nrf_cc310/lib/libnrf_cc310_0.9.10.a \

# Optimization flags
OPT = -O3 -g3
# Uncomment the line below to enable link time optimization
#OPT += -flto

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += -DBOARD_PCA10056
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DFLOAT_ABI_HARD
CFLAGS += -DNRF52840_XXAA
CFLAGS += -DOPENTHREAD_MTD=1
CFLAGS += -DSWI_DISABLE0
CFLAGS += -DUART_ENABLED=0
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -Wall -Werror
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fshort-enums

# C++ flags common to all targets
CXXFLAGS += $(OPT)

# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DBOARD_PCA10056
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DFLOAT_ABI_HARD
ASMFLAGS += -DNRF52840_XXAA
ASMFLAGS += -DOPENTHREAD_MTD=1
ASMFLAGS += -DSWI_DISABLE0
ASMFLAGS += -DUART_ENABLED=0

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs

nrf52840_xxaa: CFLAGS += -D__HEAP_SIZE=0
nrf52840_xxaa: CFLAGS += -D__STACK_SIZE=8192
nrf52840_xxaa: ASMFLAGS += -D__HEAP_SIZE=0
nrf52840_xxaa: ASMFLAGS += -D__STACK_SIZE=8192

# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm -lstdc++


.PHONY: default help

# Default target - first one defined
default: nrf52840_xxaa

# Print all targets that can be built
help:
	@echo following targets are available:
	@echo		nrf52840_xxaa
	@echo		sdk_config - starting external tool for editing sdk_config.h
	@echo		flash      - flashing binary

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc


include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

.PHONY: flash erase

# Flash the program
flash: default
	@echo Flashing: $(OUTPUT_DIRECTORY)/nrf52840_xxaa.hex
	nrfjprog -f nrf52 --program $(OUTPUT_DIRECTORY)/nrf52840_xxaa.hex --sectorerase
	nrfjprog -f nrf52 --reset

erase:
	nrfjprog -f nrf52 --eraseall

SDK_CONFIG_FILE := ./config/sdk_config.h
CMSIS_CONFIG_TOOL := $(SDK_ROOT)/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar
sdk_config:
	java -jar $(CMSIS_CONFIG_TOOL) $(SDK_CONFIG_FILE)

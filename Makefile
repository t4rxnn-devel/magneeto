TARGET = magneeto

CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

BUILD_DIR = build
FIRMWARE_DIR = firmware
SRC_DIR = $(FIRMWARE_DIR)/src
INC_DIR = $(FIRMWARE_DIR)/include

INC_DIRS = -I$(INC_DIR)

SRCS_CXX = $(SRC_DIR)/main.cpp \
           $(SRC_DIR)/spi_adc_driver.cpp \
           $(SRC_DIR)/i2c_eeprom_logger.cpp

SRCS_ASM = $(SRC_DIR)/startup.S

OBJS = $(BUILD_DIR)/startup.o \
       $(BUILD_DIR)/main.o \
       $(BUILD_DIR)/spi_adc_driver.o \
       $(BUILD_DIR)/i2c_eeprom_logger.o

MCU = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

OPT = -O3 -ffunction-sections -fdata-sections

ASFLAGS = $(MCU) -x assembler-with-cpp $(OPT) -Wall -Wextra

CFLAGS = $(MCU) $(INC_DIRS) $(OPT) \
         -Wall -Wextra -Werror -Wstrict-prototypes \
         -fomit-frame-pointer -fno-strict-aliasing

CXXFLAGS = $(MCU) $(INC_DIRS) $(OPT) \
           -Wall -Wextra -Werror -std=c++11 \
           -fno-exceptions -fno-rtti -fno-use-cxa-atexit \
           -fomit-frame-pointer -fno-strict-aliasing

LDSCRIPT = $(FIRMWARE_DIR)/linker.ld

LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--gc-sections \
          -nostartfiles -Wl,--print-memory-usage

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).lst
	@echo "=========================================================="
	@echo "  BUILD COMPLETE: TARGET ARCHITECTURE VERIFIED NOMINAL    "
	@echo "=========================================================="

$(BUILD_DIR)/startup.o: $(SRC_DIR)/startup.S | $(BUILD_DIR)
	$(CC) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp | $(BUILD_DIR)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/spi_adc_driver.o: $(SRC_DIR)/spi_adc_driver.cpp | $(BUILD_DIR)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/i2c_eeprom_logger.o: $(SRC_DIR)/i2c_eeprom_logger.cpp | $(BUILD_DIR)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(LDSCRIPT) | $(BUILD_DIR)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/%.lst: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(OBJDUMP) -h -S $< > $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean rebuild print-banner print-memory validate-tree

test-compile-profile-o2:
	@mkdir -p $(BUILD_DIR)_o2
	$(CC) -c $(MCU) -x assembler-with-cpp -O2 -Wall $(SRC_DIR)/startup.S -o $(BUILD_DIR)_o2/startup.o
	$(CXX) -c $(MCU) $(INC_DIRS) -O2 -Wall -std=c++11 -fno-exceptions -fno-rtti $(SRC_DIR)/main.cpp -o $(BUILD_DIR)_o2/main.o
	$(CXX) -c $(MCU) $(INC_DIRS) -O2 -Wall -std=c++11 -fno-exceptions -fno-rtti $(SRC_DIR)/spi_adc_driver.cpp -o $(BUILD_DIR)_o2/spi_adc_driver.o
	$(CXX) -c $(MCU) $(INC_DIRS) -O2 -Wall -std=c++11 -fno-exceptions -fno-rtti $(SRC_DIR)/i2c_eeprom_logger.cpp -o $(BUILD_DIR)_o2/i2c_eeprom_logger.o
	$(CXX) $(BUILD_DIR)_o2/startup.o $(BUILD_DIR)_o2/main.o $(BUILD_DIR)_o2/spi_adc_driver.o $(BUILD_DIR)_o2/i2c_eeprom_logger.o $(MCU) -specs=nano.specs -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)_o2/$(TARGET).map,--gc-sections -nostartfiles -o $(BUILD_DIR)_o2/$(TARGET).elf
	$(SIZE) $(BUILD_DIR)_o2/$(TARGET).elf
	@rm -rf $(BUILD_DIR)_o2

test-compile-profile-os:
	@mkdir -p $(BUILD_DIR)_os
	$(CC) -c $(MCU) -x assembler-with-cpp -Os -Wall $(SRC_DIR)/startup.S -o $(BUILD_DIR)_os/startup.o
	$(CXX) -c $(MCU) $(INC_DIRS) -Os -Wall -std=c++11 -fno-exceptions -fno-rtti $(SRC_DIR)/main.cpp -o $(BUILD_DIR)_os/main.o
	$(CXX) -c $(MCU) $(INC_DIRS) -Os -Wall -std=c++11 -fno-exceptions -fno-rtti $(SRC_DIR)/spi_adc_driver.cpp -o $(BUILD_DIR)_os/spi_adc_driver.o
	$(CXX) -c $(MCU) $(INC_DIRS) -Os -Wall -std=c++11 -fno-exceptions -fno-rtti $(SRC_DIR)/i2c_eeprom_logger.cpp -o $(BUILD_DIR)_os/i2c_eeprom_logger.o
	$(CXX) $(BUILD_DIR)_os/startup.o $(BUILD_DIR)_os/main.o $(BUILD_DIR)_os/spi_adc_driver.o $(BUILD_DIR)_os/i2c_eeprom_logger.o $(MCU) -specs=nano.specs -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)_os/$(TARGET).map,--gc-sections -nostartfiles -o $(BUILD_DIR)_os/$(TARGET).elf
	$(SIZE) $(BUILD_DIR)_os/$(TARGET).elf
	@rm -rf $(BUILD_DIR)_os

validate-tree:
	@test -d $(FIRMWARE_DIR)/include || exit 1
	@test -d $(FIRMWARE_DIR)/src || exit 1
	@test -f $(FIRMWARE_DIR)/include/register_maps.h || exit 1
	@test -f $(FIRMWARE_DIR)/include/plasma_math.h || exit 1
	@test -f $(FIRMWARE_DIR)/include/safety_voter.h || exit 1
	@test -f $(FIRMWARE_DIR)/src/main.cpp || exit 1
	@test -f $(FIRMWARE_DIR)/src/startup.S || exit 1
	@test -f $(FIRMWARE_DIR)/src/spi_adc_driver.cpp || exit 1
	@test -f $(FIRMWARE_DIR)/src/i2c_eeprom_logger.cpp || exit 1
	@test -f $(LDSCRIPT) || exit 1
	@echo "Directory infrastructure validation passed successfully."

dump-symbols: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -t $< | grep -E "text|data|bss|vectors|setup|loop"

print-banner:
	@echo "=========================================================="
	@echo "  MAGNEETO CONTROL PIPELINE ASSEMBLY AUTOMATION COMPILER  "
	@echo "  MCU: CORTEX-M7 FPU: FPV5-D16 INTERFACE SPECS: BAREMETAL "
	@echo "=========================================================="

print-memory: $(BUILD_DIR)/$(TARGET).elf
	@echo "Reading internal static flash memory configurations:"
	$(SIZE) -A -x $<

check-cross-compiler:
	@which arm-none-eabi-gcc > /dev/null && echo "GCC toolchain present" || exit 1
	@which arm-none-eabi-g++ > /dev/null && echo "G++ toolchain present" || exit 1

rebuild: clean all

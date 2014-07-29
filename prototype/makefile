NAME := palas
HEX := $(NAME).hex
OUT := $(NAME).out
MAP := $(NAME).map
SOURCES := $(wildcard *.c) $(wildcard */*.c)
HEADERS := $(wildcard *.h) $(wildcard */*.h)
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

MCU := atmega88p
MCU_AVRDUDE := m88
MCU_FREQ := 8000000UL

CC := avr-gcc
OBJCOPY := avr-objcopy
SIZE := avr-size -C --mcu=$(MCU)
DOXYGEN := doxygen

CFLAGS := -Wall -pedantic -mmcu=$(MCU) -std=c99 -g -Os -DF_CPU=$(MCU_FREQ)

all: $(HEX)

clean:
	rm -f $(HEX) $(OUT) $(MAP) $(OBJECTS)
	rm -rf doc/html

flash: $(HEX)
	avrdude -c usbtiny -p $(MCU_AVRDUDE) -U flash:w:$(HEX)

fuses:
	 avrdude -c usbtiny -p $(MCU_AVRDUDE) -U lfuse:w:0x8e:m -U hfuse:w:0xde:m

$(HEX): $(OUT)
	$(OBJCOPY) -R .eeprom -O ihex $< $@

$(OUT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ -Wl,-Map,$(MAP) $^
	@echo
	@$(SIZE) $@
	@echo

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

%.pp: %.c
	$(CC) $(CFLAGS) -E -o $@ $<

%.ppo: %.c
	$(CC) $(CFLAGS) -E $<

doc: $(HEADERS) $(SOURCES) Doxyfile
	$(DOXYGEN) Doxyfile

.PHONY: all clean flash doc


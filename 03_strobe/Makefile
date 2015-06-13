# This is a simple Makefile for building an AVR hello-world program using
# an arduino programmed in C.

#############
# TOOLCHAIN #
#############

CC =		avr-gcc
LD =		avr-ld
STRIP =		avr-strip
OBJCOPY =	avr-objcopy
SIZE =		avr-size


######################
# TARGET AND SOURCES #
######################

TARGET =	strobe
SOURCES =


####################
# BUILD PARAMETERS #
####################

MCU =		atmega328
F_CPU =		16000000
BAUD =		9600
CFLAGS =	-Wall -Werror -Os -DF_CPU=$(F_CPU) -I. -mmcu=$(MCU) \
		-DBAUD=$(BAUD)
BINFORMAT =	ihex


##########################
# PROGRAMMING PARAMETERS #
##########################

PROGRAMMER =	arduino
PART =		m328p
PORT =		$(shell ls /dev/ttyACM? | head -1)
AVRDUDE =	avrdude -v -p $(PART) -c $(PROGRAMMER) -P $(PORT)
AVRDUDE_FLASH =	-U flash:w:$(TARGET).hex


.PHONY: all
all: $(TARGET).hex

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O  $(BINFORMAT) -R .eeprom $(TARGET).elf $(TARGET).hex
	$(SIZE) -C --mcu=$(MCU) $(TARGET).elf

$(TARGET).elf: $(TARGET).c $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(TARGET).c
	$(STRIP) $(TARGET).elf

.PHONY: program
program: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLASH)

.PHONY: clean
clean:
	rm -f *.hex *.elf *.eeprom


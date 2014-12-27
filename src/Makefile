MCU          = attiny13a
MCU_AVR_CORE = __AVR_ATtiny13A__
ARCH         = AVR8
BOARD        = NONE
F_CPU        = 1200000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = main
SRC          = $(TARGET).cpp
LUFA_PATH    = include/LUFA
DEFS         = -D$(MCU_AVR_CORE)
CC_FLAGS     = $(DEFS)

OBJDIR       = build
AVRDUDE_PROGRAMMER = usbtiny
AVRDUDE_PORT = usb
AVRDUDE_MCU  = t13
AVRDUDE_FLAGS = -U lfuse:w:0x69:m -U hfuse:w:0xff:m

# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk

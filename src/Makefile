MCU          = attiny13a
MCU_AVR_CORE = __AVR_ATTiny13A__
ARCH         = AVR8
BOARD        = NONE
F_CPU        = 1000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = main
SRC          = $(TARGET).S

LUFA_PATH    = include/LUFA
DEFS         = -D$(MCU_AVR_CORE)
ASM_FLAGS     = $(DEFS) -Iinclude/
LD_FLAGS     =

OBJDIR		 = build
AVRDUDE_PROGRAMMER = usbtiny
AVRDUDE_PORT = usb
AVRDUDE_MCU  = t13

# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk

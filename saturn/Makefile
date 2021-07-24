ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

SH_PROGRAM:= sonic1
SH_SRCS:= \
	main.c \
	../src/GM_Sega.c \
	../src/Nemesis.c ../src/PaletteCycle.c ../src/Palette.c ../src/PLC.c

SH_LIBRARIES:=
SH_CFLAGS+= -O2 -I. -I../src -I../src/Backend -I../src/Object  -save-temps=obj -Wno-unused-parameter -Wno-unused-variable

IP_VERSION:= V0.001
IP_RELEASE_DATE:= 20180214
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= SONIC1
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001000
IP_1ST_READ_ADDR:= 0x06004000

M68K_PROGRAM:=
M68K_OBJECTS:=

include $(YAUL_INSTALL_ROOT)/share/post.common.mk
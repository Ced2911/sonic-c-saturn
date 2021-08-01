ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

SH_PROGRAM:= sonic1
SH_SRCS:= \
	main.c \
	vdp2.c \
	vdp1.c \
	../src/Object.c \
	../src/Object/BuzzBomber.c  \
	../src/Object/Chopper.c \
	../src/Object/Crabmeat.c    \
	../src/Object/Credits.c \
	../src/Object/Explosion.c   \
	../src/Object/GameOverCard.c    \
	../src/Object/GHZBridge.c   \
	../src/Object/GHZEdge.c \
	../src/Object/GHZRock.c \
	../src/Object/HUD.c \
	../src/Object/Monitor.c \
	../src/Object/Motobug.c \
	../src/Object/Newtron.c \
	../src/Object/PSB.c \
	../src/Object/Ring.c    \
	../src/Object/ShieldInvincibility.c \
	../src/Object/Signpost.c    \
	../src/Object/Sonic.c   \
	../src/Object/Sonic.h   \
	../src/Object/SpecialSonic.c    \
	../src/Object/Spikes.c  \
	../src/Object/Spring.c  \
	../src/Object/TitleCard.c   \
	../src/Object/TitleSonic.c  \
	../src/Demo.c \
	../src/GM_Level.c \
	../src/GM_Sega.c \
	../src/GM_Special.c \
	../src/GM_SSRG.c \
	../src/GM_Title.c \
	../src/HUD.c \
	../src/Kosinski.c \
	../src/Level.c \
	../src/LevelCollision.c \
	../src/LevelDraw.c \
	../src/LevelScroll.c \
	../src/MathUtil.c \
	../src/Nemesis.c \
	../src/Object.c \
	../src/Palette.c \
	../src/PaletteCycle.c \
	../src/PLC.c \
	../src/SpecialStage.c
	
SH_LIBRARIES:=
SH_CFLAGS+= -O2 -g -I. -I../src -I../src/Backend -I../src/Object  -save-temps=obj \
	-Wno-unused-parameter -Wno-unused-variable -Wno-implicit-fallthrough -Wno-sequence-point \
	-Wno-discarded-qualifiers -Wno-unused-function \
	-DSCP_FIX_BUGS -DSCP_BIG_ENDIAN -DSCP_JP -DSCP_REV01

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
# 20210512 manual makefile for edger group build server

# Verbose mode : make V=1  
Q = @
ifdef V
	ifneq ($(V),0)
		Q =
	endif 
endif

# check os
UNAME=$(shell uname -s)
ifeq ($(UNAME),Linux)
	OS=Linux
	KERNEL_VER=$(shell uname -v)
	ifneq (,$(findstring Microsoft,$(KERNEL_VER)))
		# windows 10 ubuntu bash shell
		OS=Windows
		FILE_EXT=.exe
$(error Windows 10 WSL terminal is not supported.)
	endif
else
	# windows terminal env (cygwin, msys, baboon, ...)
	OS=Windows
	FILE_EXT=.exe
endif 

ifeq ($(OS),Linux)
	ifneq (,$(wildcard /opt/cppcheck/cppcheck))
		CPPCHECK=/opt/cppcheck/cppcheck
	else
		CPPCHECK=/usr/bin/cppcheck
	endif
	ifneq (,$(wildcard /opt/nsiqcollector/nsiqcollector))
		NSIQ=/opt/nsiqcollector/nsiqcollector
	endif 

	ifeq ($(CROSS_COMPILE),)
		# gnu arm embedded toolchain 10-2020-q4-major
		CROSS_COMPILE=/opt/gcc-arm-none-eabi-10-2020-q4-major/bin/arm-none-eabi-

		# ac6 SystemWorkbench toolchain
		#CROSS_COMPILE=/home/saibi/SystemWorkbench/plugins/fr.ac6.mcu.externaltools.arm-none.linux64_1.17.0.201812190825/tools/compiler/bin/arm-none-eabi-
	endif
endif

ifeq ($(OS),Windows)
	CPPCHECK=
	NSIQ=
	ifeq ($(CROSS_COMPILE),)
		# set toolchain path
		#CROSS_COMPILE=/cygdrive/C/Program Files (x86)/Atollic/TrueSTUDIO for STM32 9.0.0/ARMTools/bin/arm-atollic-eabi-
		#CROSS_COMPILE=/cygdrive/C/Program Files (x86)/CodeSourcery/Sourcery_CodeBench_Lite_for_ARM_EABI/bin/arm-none-eabi-
		#CROSS_COMPILE=/mnt/c/Program Files (x86)/Atollic/TrueSTUDIO for STM32 9.0.0/ARMTools/bin/arm-atollic-eabi-
	endif
endif

ifeq (,$(wildcard build_count.txt))
$(shell echo "1" > build_count.txt)
endif

date_str=$(shell date +%Y%m%d)
time_str=$(shell date +%H%M%S)
build_count_str=$(shell touch build_count.txt;cat build_count.txt)
VERSION_INFO_FILE=./tester/version.c
version_str=$(shell ./extract_ver.sh $(VERSION_INFO_FILE))

CC=$(CROSS_COMPILE)gcc$(FILE_EXT)
LD=$(CROSS_COMPILE)ld$(FILE_EXT)
AS=$(CROSS_COMPILE)as$(FILE_EXT)
AR=$(CROSS_COMPILE)ar$(FILE_EXT)
OC=$(CROSS_COMPILE)objcopy$(FILE_EXT)

TARGET=w5500_udp_test


IOLIB_DIR=ioLibrary_Driver
INTERNET_DIR=$(IOLIB_DIR)/Internet
MQTT_DIR=$(INTERNET_DIR)/MQTT
MQTTPACKET_DIR=$(MQTT_DIR)/MQTTPacket/src
FTPSERVER_DIR=$(INTERNET_DIR)/FTPServer
FTPCLIENT_DIR=$(INTERNET_DIR)/FTPClient
DNS_DIR=$(INTERNET_DIR)/DNS
DHCP_DIR=$(INTERNET_DIR)/DHCP
ETHER_DIR=$(IOLIB_DIR)/Ethernet
W5500_DIR=$(ETHER_DIR)/W5500
W5300_DIR=$(ETHER_DIR)/W5300 
W5200_DIR=$(ETHER_DIR)/W5200 
W5100S_DIR=$(ETHER_DIR)/W5100S
W5100_DIR=$(ETHER_DIR)/W5100
APP_DIR=$(IOLIB_DIR)/Application
LOOPBACK_DIR=$(APP_DIR)/loopback

HAL_DIR=Drivers/STM32F7xx_HAL_Driver
CMSIS_DIR=Drivers/CMSIS

FATFS_DIR=Middlewares/Third_Party/FatFs/src

SRC_DIRS+=Src support comm tester
SRC_DIRS+=$(HAL_DIR)/Src 
SRC_DIRS+=$(DHCP_DIR)
SRC_DIRS+=$(ETHER_DIR) 
SRC_DIRS+=$(W5500_DIR) 

SRCS=$(foreach var,$(SRC_DIRS),$(wildcard $(var)/*.c))
STARTUP_OBJ=startup/startup_stm32f767xx.o
OBJS=$(SRCS:%.c=%.o) $(STARTUP_OBJ)

HAL_INC=$(HAL_DIR)/Inc $(HAL_DIR)/Inc/Legacy
CMSIS_INC=$(CMSIS_DIR)/Include $(CMSIS_DIR)/Device/ST/STM32F7xx/Include 
IOLIB_INC=$(INTERNET_DIR) $(APP_DIR)
INC=Inc 
INC_DIR=$(HAL_INC) $(CMSIS_INC) $(INC) $(IOLIB_INC) $(SRC_DIRS) 
INC_FLAGS=$(INC_DIR:%=-I%) 

DEF_FLAGS=-DUSE_HAL_DRIVER -DSTM32F767xx -D__weak=__attribute__\(\(weak\)\) -D__packed=__attribute__\(\(__packed__\)\) 
DEF_FLAGS+=-DDATE_STR=\"$(date_str)\" -DBUILD_COUNT_STR=\"$(build_count_str)\" -DTIME_STR=\"$(time_str)\"
DEF_FLAGS+=-DZERO_ARRAY_IDX=1

ARM_FLAGS=-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb 

COPTS+=-Og -g3 -Wall -fmessage-length=0 
COPTS+=-ffunction-sections -fdata-sections -Wall # -fstack-usage  generate stack usage file
COPTS+=-Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -pipe
COPTS+=-Wmissing-declarations -Winline -Wshadow -Wreturn-type -Wundef -Wpointer-arith -Wbad-function-cast #-Wcast-align -Wsign-compare -Wunused-function

CFLAGS=$(ARM_FLAGS) $(COPTS) $(DEF_FLAGS) $(INC_FLAGS)

LINKER_SCRIPT=STM32F767ZITx_FLASH.ld
LDFLAGS=$(ARM_FLAGS) -specs=nosys.specs -specs=nano.specs -T$(LINKER_SCRIPT) -Wl,--gc-sections -lm 

# LD flag for float printf : -u _printf_float


BIN_DIR=./bin
DIST_FILE=udp_test_v$(version_str)_$(date_str).BIN


all: checktoolchain msg dist dep 
	$(Q)./build_count.sh ./build_count.txt

flashing:
	if [ -f ../openocd/run.sh ]; then \
		../openocd/run.sh --file "run.cfg" -c "program bin/$(TARGET).elf verify reset exit"; \
	fi

checktoolchain:
	@if [ ! -x "$(CC)" ]; then \
		echo ; \
		echo "Cross compiler [$(CC)] not found." ; \
		echo "Define CROSS_COMPILE environment variable correctly." ; \
		echo "e.g. CROSS_COMPILE=/path/to/arm-none-eabi- " ; \
		echo ; \
		exit 1; \
	fi

msg:
	@echo
	@echo Build $(version_str) Build $(build_count_str)
	@echo

dist: build
	$(Q)cp $(BIN_DIR)/$(TARGET).bin magic.bin
	$(Q)chmod 0666 magic.bin
	$(Q)echo " nucleo:V$(version_str)_$(date_str)_$(build_count_str):" >> magic.bin
	$(Q)./align_ver_bin.sh magic.bin
	$(Q)mv magic.bin $(BIN_DIR)/$(DIST_FILE)
	@if [ -f /usr/local/bin/md5_generator ]; then \
		md5_generator $(BIN_DIR)/$(DIST_FILE) ; \
	fi
	@if [ -f /usr/local/bin/bin2bhc ]; then \
		bin2bhc $(BIN_DIR)/$(DIST_FILE) ; \
	fi

makedir:
	@if [ ! -d $(BIN_DIR) ]; then \
		mkdir $(BIN_DIR); \
	fi

.PHONY: refresh_build_str
refresh_build_str:
	$(Q)$(CC) $(CFLAGS) -c $(VERSION_INFO_FILE) -o $(VERSION_INFO_FILE:%.c=%.o)

build: makedir refresh_build_str $(OBJS)
	@echo build elf 
	$(Q)$(CC) $(LDFLAGS) -o $(BIN_DIR)/$(TARGET).elf -Wl,-Map,$(BIN_DIR)/$(TARGET).map $(OBJS) 
	$(Q)$(OC) -O binary $(BIN_DIR)/$(TARGET).elf $(BIN_DIR)/$(TARGET).bin
ifeq ($(A),1)
	$(Q)rm -f a.out
endif 

.s.o:
	@echo "assemble $<"
	$(Q)$(CC) -x assembler-with-cpp $(CFLAGS) -c $< -o $@

.c.o:
	@echo "compile $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@
ifeq ($(A),1)
	$(Q)$(CC) $(CFLAGS) -S -fverbose-asm $< -o $(<:%.c=%.asm)
	$(Q)$(AS) -alhnd $(<:%.c=%.asm) > $(<:%.c=%.casm)  
endif

dep:
	@echo generate dependency file
	$(Q)$(CC) -MM $(CFLAGS) $(SRCS) > depend

clean:
	@echo clean
	$(Q)rm -f $(BIN_DIR)/*
	$(Q)rm -f $(OBJS) 
	$(Q)find . -name *.su -exec rm {} \;

allclean: clean
	$(Q)find . -name *.o -exec rm {} \;
	

cleanasm:
	@echo clean asm 
	@find . -name '*.asm' -exec rm {} \;
	@find . -name '*.casm' -exec rm {} \;

cppcheck:
	$(Q)if [ -f $(CPPCHECK) ]; then \
		$(CPPCHECK) -q --enable=all --inline-suppr -iDrivers -ibin . ; \
	fi

nsiq:
	$(Q)if [ -f $(NSIQ) ]; then \
		$(NSIQ) -f nsiq_filter.txt . ; \
	fi


# dependency
-include depend

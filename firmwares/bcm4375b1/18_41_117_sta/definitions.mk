NEXMON_CHIP=CHIP_VER_BCM4375b1
NEXMON_CHIP_NUM=`$(NEXMON_ROOT)/buildtools/scripts/getdefine.sh $(NEXMON_CHIP)`
NEXMON_FW_VERSION=FW_VER_18_41_117_sta
NEXMON_FW_VERSION_NUM=`$(NEXMON_ROOT)/buildtools/scripts/getdefine.sh $(NEXMON_FW_VERSION)`

NEXMON_ARCH=armv7-r

RAM_FILE=bcmdhd_sta.bin_b1
RAMSTART=0x170000
RAMSIZE=0x190000

ROM_FILE=rom.bin
ROMSTART=0x0
ROMSIZE=0x150000

WLC_UCODE_WRITE_BL_HOOK_ADDR=0x271442
HNDRTE_RECLAIM_0_END_PTR=0x1D42E0
HNDRTE_RECLAIM_0_END=0x2AB9A4
TEMPLATERAMSTART_PTR=0x0

PATCHSIZE=0x4000
PATCHSTART=$$(($(HNDRTE_RECLAIM_0_END) - $(PATCHSIZE)))

# original ucode start and size
UCODE1START=0x28A010
UCODE1SIZE=0xFC30
UCODE1START_PTR=0x271488
UCODE1SIZE_PTR=0x271484

UCODE2START=0x299C44
UCODE2SIZE=0xD750
UCODE2START_PTR=0x271480
UCODE2SIZE_PTR=0x27147C

# original template ram start and size
TEMPLATERAMSTART0_PTR=0x289E50
TEMPLATERAMSTART0=0x2A7398
TEMPLATERAMSIZE0=0x148C
TEMPLATERAMSTART1_PTR=0x289E54
TEMPLATERAMSTART1=0x2A9770
TEMPLATERAMSIZE1=0x2234
TEMPLATERAMSTART2_PTR=0x289E58
TEMPLATERAMSTART2=0x2A8824
TEMPLATERAMSIZE2=0xF4C

# original vasip start and size
VASIPSTART_PTR=0x0
VASIPSTART=0x0
VASIPSIZE=0x0

FP_CONFIG_ORIGBASE=0x2AB9A4
FP_CONFIG_ORIGEND=0x2AC9A4
FP_CONFIG_BASE_PTR_1=0x2581EC
FP_CONFIG_END_PTR_1=0x2581F0
FP_CONFIG_BASE_PTR_2=0x25837C
FP_CONFIG_END_PTR_2=0x258380

FP_CONFIG_SIZE=0x1000
FP_CONFIG_BASE=$$(($(PATCHSTART) - $(FP_CONFIG_SIZE)))
FP_DATA_BASE=0x180000



#ifndef SPECIAL_PURPOSE_REGISTER_H_
#define SPECIAL_PURPOSE_REGISTER_H_

/* Special purpose register definitions */

// Francesco's definitions
#define SPR_TXE0_PHY_CTL_1      spr08a
#define SPR_TXE0_PHY_CTL_2      spr08b
#define SPR_SHM_DMA_CTL         spr230
#define SPR_SHM_DMA_TXADDR      spr231
#define SPR_SHM_DMA_SHMADDR     spr232
#define SPR_SHM_DMA_WORDCOUNT   spr233
#define SPR_TX_SERCTL           spr320
#define SPR_TX_PLCP_LS0         spr321
#define SPR_TX_PLCP_LS1         spr322
#define SPR_TX_PLCP_HT0         spr323
#define SPR_TX_PLCP_HT1         spr324
#define SPR_TX_PLCP_HT2         spr325
#define SPR_TX_SHM_BYTELEN      spr329

// Old definitions, for backwards compatibility when assembling old microcode.
#define SPR_TXE0_0x10               spr088
#define SPR_TXE0_0x12               spr089
#define SPR_TXE0_0x14               spr08a
#define SPR_TXE0_0x16               spr08b
#define SPR_TXE0_0x22               spr091
#define SPR_TXE0_0x24               spr092
#define SPR_TXE0_0x26               spr093
#define SPR_TXE0_0x28               spr094
#define SPR_TXE0_0x2a               spr095
#define SPR_TXE0_0x2c               spr096
#define SPR_TXE0_0x2e               spr097
#define SPR_TXE0_0x30               spr098
#define SPR_TXE0_0x32               spr099
#define SPR_TXE0_0x34               spr09a
#define SPR_TXE0_0x36               spr09b
#define SPR_TXE0_0x38               spr09c
#define SPR_TXE0_0x3a               spr09d
#define SPR_TXE0_0x3c               spr09e
#define SPR_TXE0_0x3e               spr09f
#define SPR_TXE0_0x52               spr0a9
#define SPR_TXE0_0x54               spr0aa
#define SPR_TXE0_0x56               spr0ab
#define SPR_TXE0_0x58               spr0ac
#define SPR_IFS_0x1c                spr14e
#define SPR_IFS_0x1e                spr14f
#define SPR_IFS_0x2e                spr157
#define SPR_IFS_0x30                spr158
#define SPR_IFS_0x34                spr15a
#define SPR_IFS_0x36                spr15b
#define SPR_IFS_0x38                spr15c
#define SPR_IFS_0x3a                spr15d
#define SPR_IFS_0x3c                spr15e
#define SPR_IFS_0x3e                spr15f
#define SPR_IFS_0x40                spr160
#define SPR_IFS_0x42                spr161
#define SPR_IFS_0x44                spr162
#define SPR_IFS_0x46                spr163
#define SPR_IFS_0x48                spr164
#define SPR_IFS_0x70                spr178
#define SPR_IFS_0x72                spr179

#define SPR_RXE_0x00                spr000
#define SPR_RXE_Copy_Offset         spr001
#define SPR_RXE_Copy_Length         spr002
#define SPR_RXE_FIFOCTL0            spr003
#define SPR_RXE_FIFOCTL1            spr004
#define  RXE_FIFOCTL1_STARTCOPY         0 /* bit0: Start copying the data into the FIFO */
#define  RXE_FIFOCTL1_SUSPEND           2 /* bit2: Suspend the RX engine. Used during TX */
#define  RXE_FIFOCTL1_HAVEPAD           5 /* bit5: Padding for RX frame present */
#define SPR_Received_Frame_Count        spr005
#define SPR_RXE_0x0c                spr006
#define SPR_RXE_RXHDR_OFFSET            spr007
#define SPR_RXE_RXHDR_LEN           spr008
#define SPR_RXE_PHYRXSTAT0          spr009
#define SPR_RXE_PHYRXSTAT1          spr00a
#define SPR_RXE_0x16                spr00b
#define SPR_RXE_FRAMELEN            spr00c
#define SPR_RXE_0x1a                spr00d
#define  RXE_0x1a_OVERFLOW          15 /* bit15: Indicate FIFO overflow */
#define SPR_RXE_ENCODING            spr00e
#define  RXE_ENCODING_OFDM          13 /* bit13: Frame encoding was OFDM */
#define SPR_RXE_0x1e                spr00f
#define SPR_RCM_Control             spr010
#define SPR_RCM_Match_Data          spr011
#define SPR_RCM_Match_Mask          spr012
#define SPR_RCM_Match_Delay         spr013
#define SPR_RCM_Condition_Mask_Low      spr014
#define SPR_RCM_Condition_Mask_High     spr015
#define SPR_RCM_Condition_Delay         spr016
#define SPR_RXE_0x2e                spr017
#define SPR_Ext_IHR_Address         spr018
#define SPR_Ext_IHR_Data            spr019
#define SPR_RXE_PHYRXSTAT2          spr01a
#define SPR_RXE_PHYRXSTAT3          spr01b
#define SPR_PHY_Mode                spr01c
#define SPR_RCM_TA_Control          spr01d
#define SPR_RCM_TA_Size             spr01e
#define SPR_RCM_TA_Address_0            spr01f
#define SPR_RCM_TA_Address_1            spr020
#define SPR_RCM_TA_Address_2            spr021
#define SPR_RXE_0x44                spr022
#define SPR_RXE_0x46                spr023
#define SPR_RXE_0x48                spr024
#define SPR_RXE_0x4a                spr025
#define SPR_RXE_0x4c                spr026
#define SPR_RXE_0x4e                spr027
#define SPR_RXE_0x50                spr028
#define SPR_RXE_0x52                spr029
#define SPR_RXE_0x54                spr02a
#define SPR_RXE_0x56                spr02b
#define SPR_RXE_0x58                spr02c
#define SPR_RXE_0x5a                spr02d
#define SPR_RXE_0x5c                spr02e
#define SPR_RXE_0x5e                spr02f
#define SPR_RXE_0x60                spr030
#define SPR_RXE_0x62                spr031
#define SPR_RXE_0x64                spr032
#define SPR_RXE_0x66                spr033
#define SPR_RXE_0x68                spr034
#define SPR_RXE_0x6a                spr035
#define SPR_RXE_0x6c                spr036
#define SPR_RXE_0x6e                spr037
#define SPR_RXE_0x70                spr038
#define SPR_RXE_0x72                spr039
#define SPR_RXE_0x74                spr03a
#define SPR_RXE_0x76                spr03b
#define SPR_RXE_0x78                spr03c
#define SPR_RXE_0x7a                spr03d
#define SPR_RXE_0x7c                spr03e
#define SPR_RXE_0x7e                spr03f
#define SPR_MAC_MAX_NAP             spr040
#define SPR_MAC_CTLHI               spr041
#define SPR_MAC_IRQLO               spr042
#define SPR_MAC_IRQHI               spr043
#define SPR_MAC_IRQMASKLO           spr044
#define SPR_MAC_IRQMASKHI           spr045
#define SPR_PSM_0x0c                spr046
#define SPR_MAC_CMD             spr047
#define  MACCMD_BEAC0               0 /* bit0: Beacon 0 busy/valid */
#define  MACCMD_BEAC1               1 /* bit1: Beacon 1 busy/valid */
#define  MACCMD_DFQ             2 /* bit2: Directed frame queue valid */
#define  MACCMD_CCA             3 /* bit3: Channel clear assessment */
#define  MACCMD_BGNOISE             4 /* bit4: BG-noise measurement request */
#define SPR_BRC                 spr048
#define  BRC_TXMOREFRAGS            4 /* bit4: Set if there are more TX frags to come */
#define SPR_PHY_HDR_Parameter           spr049
#define SPR_Postcard                spr04a
#define SPR_Postcard_Location_Low       spr04b
#define SPR_Postcard_Location_High      spr04c
#define SPR_GPIO_IN             spr04d
#define SPR_GPIO_OUT                spr04e
#define SPR_GPIO_OUTEN              spr04f
#define SPR_BRED0               spr050
#define SPR_BRED1               spr051
#define SPR_BRED2               spr052
#define SPR_BRED3               spr053
#define SPR_BRCL0               spr054
#define SPR_BRCL1               spr055
#define SPR_BRCL2               spr056
#define SPR_BRCL3               spr057
#define SPR_BRPO0               spr058
#define SPR_BRPO1               spr059
#define SPR_BRPO2               spr05a
#define SPR_BRPO3               spr05b
#define SPR_BRWK0               spr05c
#define SPR_BRWK1               spr05d
#define SPR_BRWK2               spr05e
#define SPR_BRWK3               spr05f
#define SPR_BASE0               spr060 /* Offset Register 0 */
#define SPR_BASE1               spr061 /* Offset Register 1 */
#define SPR_BASE2               spr062 /* Offset Register 2 */
#define SPR_BASE3               spr063 /* Offset Register 3 */
#define SPR_BASE4               spr064 /* Offset Register 4 */
#define SPR_BASE5               spr065 /* Offset Register 5 */
#define SPR_BASE6               spr066 /* Do not use. It's broken! */
#define SPR_PSM_0x4e                spr067
#define SPR_PC0                 spr068 /* Link Register 0 */
#define SPR_PC1                 spr069 /* Link Register 1 */
#define SPR_PC2                 spr06a /* Link Register 2 */
#define SPR_PC3                 spr06b /* Link Register 3 */
#define SPR_PSM_COND                spr06c /* PSM external condition bits */
#define SPR_PSM_0x5a                spr06d
#define SPR_PSM_0x5c                spr06e
#define SPR_PSM_0x5e                spr06f
#define SPR_PSM_0x60                spr070
#define SPR_PSM_0x62                spr071
#define SPR_PSM_0x64                spr072
#define SPR_PSM_0x66                spr073
#define SPR_PSM_0x68                spr074
#define SPR_PSM_0x6a                spr075
#define SPR_PSM_0x6c                spr076
#define SPR_PSM_0x6e                spr077
#define SPR_PSM_0x70                spr078
#define SPR_PSM_0x72                spr079
#define SPR_PSM_0x74                spr07a
#define SPR_PSM_0x76                spr07b
#define SPR_PSM_0x78                spr07c
#define SPR_PSM_0x7a                spr07d
#define SPR_PSM_0x7c                spr07e
#define SPR_PSM_0x7e                spr07f
#define SPR_TXE0_CTL                spr080
#define  TXE_CTL_ENABLED            0 /* bit0: The engine is enabled */
#define  TXE_CTL_FCS                14 /* bit14: Generate the FCS */
#define SPR_TXE0_AUX                spr081
#define SPR_TXE0_TS_LOC             spr082
#define SPR_TXE0_TIMEOUT            spr083
#define SPR_TXE0_WM0                spr084
#define SPR_TXE0_WM1                spr085
#define SPR_TXE0_PHY_CTL            spr086
#define SPR_TXE0_STATUS             spr087
#define  TXE_STATUS_BUSY            7 /* bit7: TX engine busy */
#define  TXE_STATUS_MEND            10 /* bit10: TXE M end */

// old values
#define SPR_TXE0_0x10               spr088
#define SPR_TXE0_0x12               spr089
#define SPR_TXE0_0x14               spr08a
#define SPR_TXE0_0x16               spr08b

// New TX Engine Registers; Source: d11.h 460918 2014-03-10 15:05:05Z
#define SPR_TXE0_MMPLCP0        spr088
#define SPR_TXE0_MMPLCP1        spr089
#define SPR_TXE0_PHY_CTL1       spr08a
#define SPR_TXE0_PHY_CTL2       spr08b

#define SPR_TX_STATUS0              spr08c
#define SPR_TX_STATUS1              spr08d
#define SPR_TX_STATUS2              spr08e
#define SPR_TX_STATUS3              spr08f
#define SPR_TXE0_FIFO_Def           spr090

// old definitions
#define SPR_TXE0_0x22               spr091
#define SPR_TXE0_0x24               spr092
#define SPR_TXE0_0x26               spr093
#define SPR_TXE0_0x28               spr094
#define SPR_TXE0_0x2a               spr095
#define SPR_TXE0_0x2c               spr096

// New Transmit Control Registers; Source: d11.h 460918 2014-03-10 15:05:05Z
#define SPR_TXE0_FIFO_Frame_Count       spr091      /* Corerev >= 16 */
#define SPR_TXE0_FIFO_Byte_Count        spr092      /* Corerev >= 16 */
#define SPR_TXE0_FIFO_Head              spr093      /* Corerev >= 16 */
#define SPR_TXE0_FIFO_Read_Pointer      spr094      /* Corerev >= 16 */
#define SPR_TXE0_FIFO_Write_Pointer     spr095      /* Corerev >= 16 */
#define SPR_TXE0_FIFO_DEF1              spr096      /* Corerev >= 16 */

// old definitions
#define SPR_TXE0_0x2e               spr097
#define SPR_TXE0_0x30               spr098
#define SPR_TXE0_0x32               spr099
#define SPR_TXE0_0x34               spr09a
#define SPR_TXE0_0x36               spr09b
#define SPR_TXE0_0x38               spr09c
#define SPR_TXE0_0x3a               spr09d
#define SPR_TXE0_0x3c               spr09e
#define SPR_TXE0_0x3e               spr09f

// New AggFifo Registers; Source: d11.h 460918 2014-03-10 15:05:05Z
#define SPR_TXE0_AGGFIFO_CMD            spr097
#define SPR_TXE0_AGGFIFO_STAT           spr098
#define SPR_TXE0_AGGFIFO_CFG_Control    spr099
#define SPR_TXE0_AGGFIFO_CFG_Data       spr09a
#define SPR_TXE0_AGGFIFO_MPDUNUM        spr09b
#define SPR_TXE0_AGGFIFO_Length         spr09c
#define SPR_TXE0_AGGFIFO_BMP            spr09d
#define SPR_TXE0_AGGFIFO_ACKEDCNT       spr09e
#define SPR_TXE0_AGGFIFO_SEL            spr09f

#define SPR_TXE0_FIFO_CMD           spr0a0
#define  TXE_FIFO_CMD_TXDONE            13 /* bit13: Set after the current transmission finished */
#define  TXE_FIFO_CMD_COPY          14 /* bit14: Start copying of data */
#define SPR_TXE0_FIFO_FLUSH         spr0a1
#define SPR_TXE0_FIFO_THRES         spr0a2
#define SPR_TXE0_FIFO_RDY           spr0a3 /* FIFO-ready bitmask. bit-nr = FIFO-nr */
#define SPR_TXE0_FIFO_PRI_RDY           spr0a4
#define SPR_TXE0_FIFO_RQ_PRI            spr0a5
#define SPR_TXE0_Template_TX_Pointer        spr0a6
#define SPR_TXE0_0x4e               spr0a7
#define SPR_TXE0_Template_Pointer       spr0a8

// old definitions
#define SPR_TXE0_0x52               spr0a9
#define SPR_TXE0_0x54               spr0aa
#define SPR_TXE0_0x56               spr0ab
#define SPR_TXE0_0x58               spr0ac

// New Registers; Source: d11.h 460918 2014-03-10 15:05:05Z
#define SPR_TXE0_CLCT_STRPTR      spr0a9            /* Corerev >= 22 */
#define SPR_TXE0_CLCT_STPPTR      spr0aa            /* Corerev >= 22 */
#define SPR_TXE0_CLCT_CURPTR      spr0ab            /* Corerev >= 22 */
#define SPR_TXE0_AGGFIFO_Data     spr0ac

#define SPR_TXE0_0x5a               spr0ad
#define SPR_TXE0_0x5c               spr0ae
#define SPR_TXE0_0x5e               spr0af

#define SPR_TXE0_Template_Data_Low      spr0b0
#define SPR_TXE0_Template_Data_High     spr0b1
#define SPR_TXE0_0x64               spr0b2
#define SPR_TXE0_0x66               spr0b3
#define SPR_TXE0_SELECT             spr0b4
#define  TXE_SELECT_DST             0x0003 /* Destination code mask */
#define   TXE_SELECT_DST_SHM            0x0001 /* Destination code for the SHM */
#define   TXE_SELECT_DST_PHY            0x0002 /* Destination code for the PHY */
#define   TXE_SELECT_DST_DISCARD        0x0003 /* Destination code for discarding */
#define  TXE_SELECT_USE_TXCNT           2 /* bit2: Use the TX_COUNT register */
#define  TXE_SELECT_UNK0x20         5 /* bit5: ??FIXME: Unknown meaning. Seems to be PHY related. */
#define  TXE_SELECT_SRC             0x1F00 /* Source code mask */
#define  TXE_SELECT_SRC_SHIFT           8
#define   TXE_SELECT_SRC_FIFO0          0x0000 /* Source code for FIFO 0 */
#define   TXE_SELECT_SRC_FIFO1          0x0100 /* Source code for FIFO 1 */
#define   TXE_SELECT_SRC_FIFO2          0x0200 /* Source code for FIFO 2 */
#define   TXE_SELECT_SRC_FIFO3          0x0300 /* Source code for FIFO 3 */
#define   TXE_SELECT_SRC_FIFO4          0x0400 /* Source code for FIFO 4 */
#define   TXE_SELECT_SRC_FIFO5          0x0500 /* Source code for FIFO 5 */
#define   TXE_SELECT_SRC_FIFO6          0x0600 /* Source code for FIFO 6 */
#define   TXE_SELECT_SRC_FIFO7          0x0700 /* Source code for FIFO 7 */
#define   TXE_SELECT_SRC_TRAM           0x0800 /* Source code for Template Ram */
#define SPR_TXE0_TX_COUNT           spr0b5 /* Number of bytes to process */
#define SPR_TXE0_TX_SHM_ADDR            spr0b6 /* The SHM address to copy the TX header to (in bytes!) */
#define SPR_TXE0_0x6e               spr0b7
#define SPR_TXE0_0x70               spr0b8
#define SPR_TXE0_0x72               spr0b9
#define SPR_TXE0_0x74               spr0ba
#define SPR_TXE0_0x76               spr0bb
#define SPR_TXE0_0x78               spr0bc
#define SPR_TXE0_0x7a               spr0bd
#define SPR_TXE0_0x7c               spr0be
#define SPR_TXE0_0x7e               spr0bf
/* Transmit Modify Engine masks */
#define SPR_TME_MASK0               spr0c0
#define SPR_TME_MASK2               spr0c1
#define SPR_TME_MASK4               spr0c2
#define SPR_TME_MASK6               spr0c3
#define SPR_TME_MASK8               spr0c4
#define SPR_TME_MASK10              spr0c5
#define SPR_TME_MASK12              spr0c6
#define SPR_TME_MASK14              spr0c7
#define SPR_TME_MASK16              spr0c8
#define SPR_TME_MASK18              spr0c9
#define SPR_TME_MASK20              spr0ca
#define SPR_TME_MASK22              spr0cb
#define SPR_TME_MASK24              spr0cc
#define SPR_TME_MASK26              spr0cd
#define SPR_TME_MASK28              spr0ce
#define SPR_TME_MASK30              spr0cf
#define SPR_TME_MASK32              spr0d0
#define SPR_TME_MASK34              spr0d1
#define SPR_TME_MASK36              spr0d2
#define SPR_TME_MASK38              spr0d3
#define SPR_TME_MASK40              spr0d4
#define SPR_TME_MASK42              spr0d5
#define SPR_TME_MASK44              spr0d6
#define SPR_TME_MASK46              spr0d7
#define SPR_TME_MASK48              spr0d8
#define SPR_TME_MASK50              spr0d9
#define SPR_TME_MASK52              spr0da
#define SPR_TME_MASK54              spr0db
#define SPR_TME_MASK56              spr0dc
#define SPR_TME_MASK58              spr0dd
#define SPR_TME_MASK60              spr0de
#define SPR_TME_MASK62              spr0df
/* Transmit Modify Engine values */
#define SPR_TME_VAL0                spr0e0
#define SPR_TME_VAL2                spr0e1
#define SPR_TME_VAL4                spr0e2
#define SPR_TME_VAL6                spr0e3
#define SPR_TME_VAL8                spr0e4
#define SPR_TME_VAL10               spr0e5
#define SPR_TME_VAL12               spr0e6
#define SPR_TME_VAL14               spr0e7
#define SPR_TME_VAL16               spr0e8
#define SPR_TME_VAL18               spr0e9
#define SPR_TME_VAL20               spr0ea
#define SPR_TME_VAL22               spr0eb
#define SPR_TME_VAL24               spr0ec
#define SPR_TME_VAL26               spr0ed
#define SPR_TME_VAL28               spr0ee
#define SPR_TME_VAL30               spr0ef
#define SPR_TME_VAL32               spr0f0
#define SPR_TME_VAL34               spr0f1
#define SPR_TME_VAL36               spr0f2
#define SPR_TME_VAL38               spr0f3
#define SPR_TME_VAL40               spr0f4
#define SPR_TME_VAL42               spr0f5
#define SPR_TME_VAL44               spr0f6
#define SPR_TME_VAL46               spr0f7
#define SPR_TME_VAL48               spr0f8
#define SPR_TME_VAL50               spr0f9
#define SPR_TME_VAL52               spr0fa
#define SPR_TME_VAL54               spr0fb
#define SPR_TME_VAL56               spr0fc
#define SPR_TME_VAL58               spr0fd
#define SPR_TME_VAL60               spr0fe
#define SPR_TME_VAL62               spr0ff
#define SPR_TSF_0x00                spr100
#define SPR_TSF_0x02                spr101
#define SPR_TSF_CFP_Start_Low           spr102
#define SPR_TSF_CFP_Start_High          spr103
#define SPR_TSF_0x08                spr104
#define SPR_TSF_0x0a                spr105
#define SPR_TSF_0x0c                spr106
#define SPR_TSF_0x0e                spr107
#define SPR_TSF_0x10                spr108
#define SPR_TSF_CFP_PreTBTT         spr109
#define SPR_TSF_0x14                spr10a
#define SPR_TSF_0x16                spr10b
#define SPR_TSF_0x18                spr10c
#define SPR_TSF_0x1a                spr10d
#define SPR_TSF_0x1c                spr10e
#define SPR_TSF_0x1e                spr10f
#define SPR_TSF_0x20                spr110
#define SPR_TSF_0x22                spr111
#define SPR_TSF_0x24                spr112
#define SPR_TSF_0x26                spr113
#define SPR_TSF_0x28                spr114
#define SPR_TSF_0x2a                spr115
#define SPR_TX_FES_Time             spr116
#define SPR_TSF_0x2e                spr117
#define SPR_TSF_0x30                spr118
#define SPR_TSF_WORD0               spr119
#define SPR_TSF_WORD1               spr11a
#define SPR_TSF_WORD2               spr11b
#define SPR_TSF_WORD3               spr11c
#define SPR_TSF_0x3a                spr11d
#define SPR_TSF_0x3c                spr11e
#define SPR_TSF_0x3e                spr11f
#define SPR_TSF_0x40                spr120
#define SPR_TSF_0x42                spr121
#define SPR_TSF_0x44                spr122
#define SPR_TSF_GPT0_STAT           spr123
#define  GPT_STAT_EN                15 /* bit15: Enable the timer */
#define  GPT_STAT_8MHZ              14 /* bit14: Use 8MHz base (otherwise 88MHz) */
#define SPR_TSF_GPT1_STAT           spr124
#define SPR_TSF_GPT0_CNTLO          spr125
#define SPR_TSF_GPT1_CNTLO          spr126
#define SPR_TSF_GPT0_CNTHI          spr127
#define SPR_TSF_GPT1_CNTHI          spr128
#define SPR_TSF_GPT0_VALLO          spr129
#define SPR_TSF_GPT1_VALLO          spr12a
#define SPR_TSF_GPT0_VALHI          spr12b
#define SPR_TSF_GPT1_VALHI          spr12c
#define SPR_TSF_RANDOM              spr12d
#define SPR_TSF_0x5c                spr12e
#define SPR_TSF_0x5e                spr12f
#define SPR_TSF_0x60                spr130
#define SPR_TSF_0x62                spr131
#define SPR_TSF_0x64                spr132
#define SPR_TSF_GPT2_STAT           spr133
#define SPR_TSF_GPT2_CNTLO          spr134
#define SPR_TSF_GPT2_CNTHI          spr135
#define SPR_TSF_GPT2_VALLO          spr136
#define SPR_TSF_GPT2_VALHI          spr137
#define SPR_TSF_GPT_ALL_STAT            spr138
#define SPR_TSF_0x72                spr139
#define SPR_TSF_0x74                spr13a
#define SPR_TSF_0x76                spr13b
#define SPR_TSF_0x78                spr13c
#define SPR_TSF_0x7a                spr13d
#define SPR_TSF_0x7c                spr13e
#define SPR_TSF_0x7e                spr13f
#define SPR_IFS_sifs_rx_tx_tx           spr140
#define SPR_IFS_sifs_nav_tx         spr141
#define SPR_IFS_slot                spr142
#define SPR_IFS_0x06                spr143
#define SPR_IFS_CTL             spr144
#define SPR_IFS_BKOFFTIME           spr145 /* BackoffTime; in units of PHY slots */
#define SPR_IFS_0x0c                spr146
#define SPR_IFS_0x0e                spr147
#define SPR_IFS_STAT                spr148
#define SPR_IFS_med_busy_ctl            spr149
#define SPR_IFS_if_tx_duration          spr14a
#define SPR_IFS_0x16                spr14b
#define SPR_IFS_0x18                spr14c
#define SPR_IFS_0x1a                spr14d

// New Registers; Source: d11.h 460918 2014-03-10 15:05:05Z
/* EDCF support in dot11macs with corerevs >= 16 */
#define SPR_IFS_0x1c                spr14e
#define SPR_IFS_AIFSN               spr14e
#define SPR_IFS_0x1e                spr14f
#define SPR_IFS_CTL1                spr14f




#define SPR_SCC_Control             spr150
#define SPR_SCC_Timer_Low           spr151
#define SPR_SCC_Timer_High          spr152
#define SPR_SCC_Divisor             spr153
#define SPR_SCC_Fast_Powerup_Delay      spr154
#define SPR_SCC_Period              spr155
#define SPR_SCC_Period_Divisor          spr156

// New Registers; Source: d11.h 460918 2014-03-10 15:05:05Z
#define SPR_SCC_CAL_Timer_Low       spr157
#define SPR_SCC_CAL_Timer_High      spr158

#define SPR_IFS_0x2e                spr157
#define SPR_IFS_0x30                spr158

#define SPR_IFS_0x32                spr159

// old definitions
#define SPR_IFS_0x34                spr15a
#define SPR_IFS_0x36                spr15b
#define SPR_IFS_0x38                spr15c
#define SPR_IFS_0x3a                spr15d
#define SPR_IFS_0x3c                spr15e
#define SPR_IFS_0x3e                spr15f
#define SPR_IFS_0x40                spr160
#define SPR_IFS_0x42                spr161
#define SPR_IFS_0x44                spr162
#define SPR_IFS_0x46                spr163
#define SPR_IFS_0x48                spr164


// New Registers; Source: d11.h 460918 2014-03-10 15:05:05Z
/* BTCX block on corerev >=13 */
#define SPR_BTCX_Control                      spr15a
#define SPR_BTCX_Stat                         spr15b
#define SPR_BTCX_Transmit_Control             spr15c
#define SPR_BTCX_PRI_WIN                      spr15d
#define SPR_BTCX_TX_Conf_Timer                spr15e
#define SPR_BTCX_ANT_SW_Timer                 spr15f
#define SPR_BTCX_PRV_RFACT_Timer              spr160
#define SPR_BTCX_CUR_RFACT_Timer              spr161
#define SPR_BTCX_RFACT_DUR_Timer              spr162
#define SPR_IFS_CTL_SEL_PRICRS                spr163
#define SPR_IFS_CTL_SEL_SECCRS                spr164
/* ECI regs on corerev >=14 */
#define SPR_IFS_0x70                          spr178
#define SPR_BTCX_ECI_Address                  spr178
#define SPR_IFS_0x72                          spr179
#define SPR_BTCX_ECI_Data                     spr179

#define SPR_IFS_0x4a                spr165
#define SPR_IFS_0x4c                spr166
#define SPR_IFS_0x4e                spr167
#define SPR_IFS_0x50                spr168
#define SPR_IFS_0x52                spr169
#define SPR_IFS_0x54                spr16a
#define SPR_IFS_0x56                spr16b
#define SPR_IFS_0x58                spr16c
#define SPR_IFS_0x5a                spr16d
#define SPR_IFS_0x5c                spr16e
#define SPR_IFS_0x5e                spr16f
#define SPR_IFS_0x60                spr170
#define SPR_IFS_0x62                spr171
#define SPR_IFS_0x64                spr172
#define SPR_IFS_0x66                spr173
#define SPR_IFS_0x68                spr174
#define SPR_IFS_0x6a                spr175
#define SPR_IFS_0x6c                spr176
#define SPR_IFS_0x6e                spr177
#define SPR_IFS_0x74                spr17a
#define SPR_IFS_0x76                spr17b
#define SPR_IFS_0x78                spr17c
#define SPR_IFS_0x7a                spr17d
#define SPR_IFS_0x7c                spr17e
#define SPR_IFS_0x7e                spr17f

#define SPR_NAV_CTL             spr180
#define SPR_NAV_STAT                spr181
#define SPR_NAV_0x04                spr182
#define SPR_NAV_0x06                spr183
#define SPR_NAV_0x08                spr184
#define SPR_NAV_0x0a                spr185
#define SPR_NAV_ALLOCATION          spr186
#define SPR_NAV_0x0e                spr187
#define SPR_NAV_0x10                spr188
#define SPR_NAV_0x12                spr189
#define SPR_NAV_0x14                spr18a
#define SPR_NAV_0x16                spr18b
#define SPR_NAV_0x18                spr18c
#define SPR_NAV_0x1a                spr18d
#define SPR_NAV_0x1c                spr18e
#define SPR_NAV_0x1e                spr18f
#define SPR_NAV_0x20                spr190
#define SPR_NAV_0x22                spr191
#define SPR_NAV_0x24                spr192
#define SPR_NAV_0x26                spr193
#define SPR_NAV_0x28                spr194
#define SPR_NAV_0x2a                spr195
#define SPR_NAV_0x2c                spr196
#define SPR_NAV_0x2e                spr197
#define SPR_NAV_0x30                spr198
#define SPR_NAV_0x32                spr199
#define SPR_NAV_0x34                spr19a
#define SPR_NAV_0x36                spr19b
#define SPR_NAV_0x38                spr19c
#define SPR_NAV_0x3a                spr19d
#define SPR_NAV_0x3c                spr19e
#define SPR_NAV_0x3e                spr19f
#define SPR_NAV_0x40                spr1a0
#define SPR_NAV_0x42                spr1a1
#define SPR_NAV_0x44                spr1a2
#define SPR_NAV_0x46                spr1a3
#define SPR_NAV_0x48                spr1a4
#define SPR_NAV_0x4a                spr1a5
#define SPR_NAV_0x4c                spr1a6
#define SPR_NAV_0x4e                spr1a7
#define SPR_NAV_0x50                spr1a8
#define SPR_NAV_0x52                spr1a9
#define SPR_NAV_0x54                spr1aa
#define SPR_NAV_0x56                spr1ab
#define SPR_NAV_0x58                spr1ac
#define SPR_NAV_0x5a                spr1ad
#define SPR_NAV_0x5c                spr1ae
#define SPR_NAV_0x5e                spr1af
#define SPR_NAV_0x60                spr1b0
#define SPR_NAV_0x62                spr1b1
#define SPR_NAV_0x64                spr1b2
#define SPR_NAV_0x66                spr1b3
#define SPR_NAV_0x68                spr1b4
#define SPR_NAV_0x6a                spr1b5
#define SPR_NAV_0x6c                spr1b6
#define SPR_NAV_0x6e                spr1b7
#define SPR_NAV_0x70                spr1b8
#define SPR_NAV_0x72                spr1b9
#define SPR_NAV_0x74                spr1ba
#define SPR_NAV_0x76                spr1bb
#define SPR_NAV_0x78                spr1bc
#define SPR_NAV_0x7a                spr1bd
#define SPR_NAV_0x7c                spr1be
#define SPR_NAV_0x7e                spr1bf
#define SPR_WEP_0x00                spr1c0
#define SPR_WEP_0x02                spr1c1
#define SPR_WEP_0x04                spr1c2
#define SPR_WEP_0x06                spr1c3
#define SPR_WEP_0x08                spr1c4
#define SPR_WEP_0x0a                spr1c5
#define SPR_WEP_0x0c                spr1c6
#define SPR_WEP_0x0e                spr1c7
#define SPR_WEP_0x10                spr1c8
#define SPR_WEP_0x12                spr1c9
#define SPR_WEP_0x14                spr1ca
#define SPR_WEP_0x16                spr1cb
#define SPR_WEP_0x18                spr1cc
#define SPR_WEP_0x1a                spr1cd
#define SPR_WEP_0x1c                spr1ce
#define SPR_WEP_0x1e                spr1cf
#define SPR_WEP_0x20                spr1d0
#define SPR_WEP_0x22                spr1d1
#define SPR_WEP_0x24                spr1d2
#define SPR_WEP_0x26                spr1d3
#define SPR_WEP_0x28                spr1d4
#define SPR_WEP_0x2a                spr1d5
#define SPR_WEP_0x2c                spr1d6
#define SPR_WEP_0x2e                spr1d7
#define SPR_WEP_0x30                spr1d8
#define SPR_WEP_0x32                spr1d9
#define SPR_WEP_0x34                spr1da
#define SPR_WEP_0x36                spr1db
#define SPR_WEP_0x38                spr1dc
#define SPR_WEP_0x3a                spr1dd
#define SPR_WEP_0x3c                spr1de
#define SPR_WEP_0x3e                spr1df
#define SPR_WEP_CTL             spr1e0
#define SPR_WEP_IV_Location         spr1e1
#define SPR_WEP_IV_Key              spr1e2
#define SPR_WEP_WKey                spr1e3
#define SPR_WEP_0x48                spr1e4
#define SPR_WEP_0x4a                spr1e5
#define SPR_WEP_0x4c                spr1e6
#define SPR_WEP_0x4e                spr1e7
#define SPR_WEP_0x50                spr1e8
#define SPR_WEP_0x52                spr1e9
#define SPR_WEP_0x54                spr1ea
#define SPR_WEP_AES_Control         spr1eb
#define SPR_WEP_0x58                spr1ec
#define SPR_WEP_0x5a                spr1ed
#define SPR_WEP_0x5c                spr1ee
#define SPR_WEP_0x5e                spr1ef
#define SPR_PMQ_control_low         spr1f0
#define SPR_PMQ_control_high            spr1f1
#define SPR_PMQ_pat_0               spr1f2
#define SPR_PMQ_pat_1               spr1f3
#define SPR_PMQ_pat_2               spr1f4
#define SPR_PMQ_dat             spr1f5
#define SPR_PMQ_dat_or              spr1f6
#define SPR_PMQ_0x0e                spr1f7
#define SPR_PMQ_pat_h0              spr1f8
#define SPR_PMQ_pat_h1              spr1f9
#define SPR_PMQ_pat_h2              spr1fa
#define SPR_PMQ_dat_h               spr1fb
#define SPR_PMQ_0x18                spr1fc
#define SPR_PMQ_0x1a                spr1fd
#define SPR_PMQ_0x1c                spr1fe
#define SPR_PMQ_0x1e                spr1ff

// New 802.11ac Registers; Source: d11.h 460918 2014-03-10 15:05:05Z
/* AQM */
#define SPR_AQM_Config                  spr200
#define SPR_AQM_FIFO_Def                spr201
#define SPR_AQM_Max_IDX                 spr202
#define SPR_AQM_RCVD_BA0                spr203
#define SPR_AQM_RCVD_BA1                spr204
#define SPR_AQM_RCVD_BA2                spr205
#define SPR_AQM_RCVD_BA3                spr206
#define SPR_AQM_BASSN                   spr207
#define SPR_AQM_REFSN                   spr208
#define SPR_AQM_Max_Agg_Len_Low         spr209
#define SPR_AQM_Max_Agg_Len_High        spr20a
#define SPR_AQM_Agg_Params              spr20b
#define SPR_AQM_Min_MPDU_Length         spr20c
#define SPR_AQM_MAC_Adj_Length          spr20d
#define SPR_AQM_Debug_Bus_Control       spr20e
#define SPR_AQM_Agg_Stats               spr210
#define SPR_AQM_Agg_Len_Low             spr211
#define SPR_AQM_Agg_Len_High            spr212
#define SPR_AQM_IDX_FIFO                spr213
#define SPR_AQM_MPDU_Len_FIFO           spr214
#define SPR_AQM_TX_Control_FIFO         spr215
#define SPR_AQM_Upd_BA0                 spr216
#define SPR_AQM_Upd_BA1                 spr217
#define SPR_AQM_Upd_BA2                 spr218
#define SPR_AQM_Upd_BA3                 spr219
#define SPR_AQM_ACK_Control             spr21a
#define SPR_AQM_Cons_Control            spr21b
#define SPR_AQM_FIFO_Ready              spr21c
#define SPR_AQM_Start_Loc               spr21d
#define SPR_TDCCTL                      spr220
#define SPR_TDC_PLCP0                   spr221
#define SPR_TDC_PLCP1                   spr222
#define SPR_TDC_Frame_Length0           spr223
#define SPR_TDC_Frame_Length1           spr224
#define SPR_TDC_TX_Time                 spr225
#define SPR_TDC_VHT_Sig_B0              spr226
#define SPR_TDC_VHT_Sig_B1              spr227
#define SPR_TDC_VHT_L_Sig_Len           spr228
#define SPR_TDC_VHT_N_Sym0              spr229
#define SPR_TDC_VHT_N_Sym1              spr22a
#define SPR_TDC_VHT_PSDU_Len0           spr22b
#define SPR_TDC_VHT_PSDU_Len1           spr22c
#define SPR_TDC_VHT_MAC_PAD             spr22d
#define SPR_SHMDMA_Control              spr230
#define SPR_SHMDMA_TXDC_Address         spr231
#define SPR_SHMDMA_SHM_Address          spr232
#define SPR_SHMDMA_Xfer_Cnt             spr233
#define SPR_TXDC_Address                spr234
#define SPR_TXDC_Data                   spr235
/* RXE Register */
#define SPR_MHP_Status                  spr240
#define SPR_MHP_FC                      spr241
#define SPR_MHP_DUR                     spr242
#define SPR_MHP_SC                      spr243
#define SPR_MHP_QOS                     spr244
#define SPR_MHP_HTC_High                spr245
#define SPR_MHP_HTC_Low                 spr246
#define SPR_MHP_Addr1_High              spr247
#define SPR_MHP_Addr1_Mid               spr248
#define SPR_MHP_Addr1_Low               spr249
#define SPR_MHP_Addr2_High              spr250
#define SPR_MHP_Addr2_Mid               spr251
#define SPR_MHP_Addr2_Low               spr252
#define SPR_MHP_Addr3_High              spr253
#define SPR_MHP_Addr3_Mid               spr254
#define SPR_MHP_Addr3_Low               spr255
#define SPR_MHP_Addr4_High              spr256
#define SPR_MHP_Addr4_Mid               spr257
#define SPR_MHP_Addr4_Low               spr258
#define SPR_MHP_CFG                     spr259
#define SPR_DAGG_CTL2                   spr260
#define SPR_DAGG_BYTESLEFT              spr261
#define SPR_DAGG_SH_OFFSET              spr262
#define SPR_DAGG_STAT                   spr263
#define SPR_DAGG_LEN                    spr264
#define SPR_TXBA_Control                spr265
#define SPR_TXBA_Data_Select            spr266
#define SPR_TXBA_Data                   spr267
#define SPR_AMT_Control                 spr270
#define SPR_AMT_Status                  spr271
#define SPR_AMT_Limit                   spr272
#define SPR_AMT_Attr                    spr273
#define SPR_AMT_Match1                  spr274
#define SPR_AMT_Match2                  spr275
#define SPR_AMT_Table_Address           spr276
#define SPR_AMT_Table_Data              spr277
#define SPR_AMT_Table_Value             spr278
#define SPR_AMT_Debug_Select            spr279
#define SPR_ROE_Control                 spr280
#define SPR_ROE_Status                  spr281
#define SPR_ROE_IP_Checksum             spr282
#define SPR_ROE_TCPUDP_Checksum         spr283
#define SPR_PSO_Control                 spr290
#define SPR_PSO_RX_Words_Watermark      spr291
#define SPR_PSO_RX_Cnt_Watermark        spr292
#define SPR_OBFF_Control                spr298
#define SPR_OBFF_RX_Words_Watermark     spr299
#define SPR_OBFF_RX_Cnt_Watermark       spr29a
/* TOE */
#define SPR_TOE_Control                 spr300
#define SPR_TOE_Rst                     spr301
#define SPR_TOE_CSumNZ                  spr302
#define SPR_TX_Serial_Control           spr320
#define SPR_TX_PLCP_Sig0                spr321
#define SPR_TX_PLCP_Sig1                spr322
#define SPR_TX_PLCP_HT_Sig0             spr323
#define SPR_TX_PLCP_HT_Sig1             spr324
#define SPR_TX_PLCP_HT_Sig2             spr325
#define SPR_TX_PLCP_VHT_SigB0           spr326
#define SPR_TX_PLCP_VHT_SigB1           spr327
#define SPR_MAC_Header_From_SHM_Length  spr329
#define SPR_TX_PLCP_Length              spr32a
#define SPR_TX_BF_Rpt_Length            spr32c
#define SPR_TX_BF_Control               spr330
#define SPR_Bfm_Rpt_Offset              spr331
#define SPR_Bfm_Rpt_Length              spr332
#define SPR_TX_BF_BfeRptRdCnt           spr333

/* Named definitions for the Transmit Modify Engine MASK registers */
#define SPR_TME_M_PLCP0             SPR_TME_MASK0   /* PLCP header (low) */
#define SPR_TME_M_PLCP1             SPR_TME_MASK2   /* PLCP header (middle) */
#define SPR_TME_M_PLCP2             SPR_TME_MASK4   /* PLCP header (high) */
#define SPR_TME_M_FCTL              SPR_TME_MASK6   /* Frame control */
#define SPR_TME_M_DURID             SPR_TME_MASK8   /* Duration / ID */
#define SPR_TME_M_ADDR1_0           SPR_TME_MASK10  /* Address 1 (low) */
#define SPR_TME_M_ADDR1_1           SPR_TME_MASK12  /* Address 1 (middle) */
#define SPR_TME_M_ADDR1_2           SPR_TME_MASK14  /* Address 1 (high) */
#define SPR_TME_M_ADDR2_0           SPR_TME_MASK16  /* Address 2 (low) */
#define SPR_TME_M_ADDR2_1           SPR_TME_MASK18  /* Address 2 (middle) */
#define SPR_TME_M_ADDR2_2           SPR_TME_MASK20  /* Address 2 (high) */
#define SPR_TME_M_ADDR3_0           SPR_TME_MASK22  /* Address 3 (low) */
#define SPR_TME_M_ADDR3_1           SPR_TME_MASK24  /* Address 3 (middle) */
#define SPR_TME_M_ADDR3_2           SPR_TME_MASK26  /* Address 3 (high) */
#define SPR_TME_M_SEQ               SPR_TME_MASK28  /* Sequence control */
#define SPR_TME_M_ADDR4_0           SPR_TME_MASK30  /* Address 4 (low) */
#define SPR_TME_M_ADDR4_1           SPR_TME_MASK32  /* Address 4 (middle) */
#define SPR_TME_M_ADDR4_2           SPR_TME_MASK34  /* Address 4 (high) */

/* Named definitions for the Transmit Modify Engine VALUE registers */
#define SPR_TME_V_PLCP0             SPR_TME_VAL0    /* PLCP header (low) */
#define SPR_TME_V_PLCP1             SPR_TME_VAL2    /* PLCP header (middle) */
#define SPR_TME_V_PLCP2             SPR_TME_VAL4    /* PLCP header (high) */
#define SPR_TME_V_FCTL              SPR_TME_VAL6    /* Frame control */
#define SPR_TME_V_DURID             SPR_TME_VAL8    /* Duration / ID */
#define SPR_TME_V_ADDR1_0           SPR_TME_VAL10   /* Address 1 (low) */
#define SPR_TME_V_ADDR1_1           SPR_TME_VAL12   /* Address 1 (middle) */
#define SPR_TME_V_ADDR1_2           SPR_TME_VAL14   /* Address 1 (high) */
#define SPR_TME_V_ADDR2_0           SPR_TME_VAL16   /* Address 2 (low) */
#define SPR_TME_V_ADDR2_1           SPR_TME_VAL18   /* Address 2 (middle) */
#define SPR_TME_V_ADDR2_2           SPR_TME_VAL20   /* Address 2 (high) */
#define SPR_TME_V_ADDR3_0           SPR_TME_VAL22   /* Address 3 (low) */
#define SPR_TME_V_ADDR3_1           SPR_TME_VAL24   /* Address 3 (middle) */
#define SPR_TME_V_ADDR3_2           SPR_TME_VAL26   /* Address 3 (high) */
#define SPR_TME_V_SEQ               SPR_TME_VAL28   /* Sequence control */
#define SPR_TME_V_ADDR4_0           SPR_TME_VAL30   /* Address 4 (low) */
#define SPR_TME_V_ADDR4_1           SPR_TME_VAL32   /* Address 4 (middle) */
#define SPR_TME_V_ADDR4_2           SPR_TME_VAL34   /* Address 4 (high) */


/* Interrupts (SPR_MAC_IRQLO/HI) */
#define IRQLO_MAC_SUSPENDED 0x0001
#define IRQLO_BEACONTEMP_AVAIL  0x0002
#define IRQLO_TBTT_INDI     0x0004
#define IRQLO_BEACON_TX_OK  0x0008
#define IRQLO_BEACON_CANCEL 0x0010
#define IRQLO_ATIM_END      0x0020
#define IRQLO_PMQ       0x0040
#define IRQLO_UNDEFINED_0   0x0080
#define IRQLO_PIO_WORKAROUND    0x0100
#define IRQLO_MAC_TXERR     0x0200
#define IRQLO_UNDEFINED_2   0x0400
#define IRQLO_PHY_TXERR     0x0800
#define IRQLO_PMEVENT       0x1000
#define IRQLO_TIMER0        0x2000
#define IRQLO_TIMER1        0x4000
#define IRQLO_DMA       0x8000
#define IRQHI_TXFIFO_FLUSH_OK   0x0001
#define IRQHI_CCA_MEASURE_OK    0x0002
#define IRQHI_NOISESAMPLE_OK    0x0004
#define IRQHI_UNDEFINED_3   0x0008
#define IRQHI_UNDEFINED_4   0x0010
#define IRQHI_UNDEFINED_5   0x0020
#define IRQHI_UNDEFINED_6   0x0040
#define IRQHI_UNDEFINED_7   0x0080
#define IRQHI_UNDEFINED_8   0x0100
#define IRQHI_UNDEFINED_9   0x0200
#define IRQHI_UNDEFINED_10  0x0400
#define IRQHI_DEBUG     0x0800
#define IRQHI_RFKILL        0x1000
#define IRQHI_TX_OK     0x2000
#define IRQHI_PHY_G_CHANGED 0x4000
#define IRQHI_TIMEOUT       0x8000


/* MAC Control High bits */
#define MACCTL_BE       0 /* bit0: Big Endian mode */
#define MACCTL_INFRA        1 /* bit1: Infrastructure mode */
#define MACCTL_AP       2 /* bit2: AccessPoint mode */
#define MACCTL_RADIOLOCK    3 /* bit3: Radio lock */
#define MACCTL_BEACPROMISC  4 /* bit4: Beacon Promiscuous */
#define MACCTL_KEEP_BADPLCP 5 /* bit5: Keep frames with bad PLCP */
#define MACCTL_KEEP_CTL     6 /* bit6: Keep control frames */
#define MACCTL_KEEP_BAD     7 /* bit7: Keep bad frames (FCS) */
#define MACCTL_PROMISC      8 /* bit8: Promiscuous mode */
#define MACCTL_HWPS     9 /* bit9: Hardware Power Saving */
#define MACCTL_AWAKE        10 /* bit10: Device is awake */
#define MACCTL_CLOSEDNET    11 /* bit11: Closed net (no SSID bcast) */
#define MACCTL_TBTTHOLD     12 /* bit12: TBTT Hold */
#define MACCTL_DISCTXSTAT   13 /* bit13: Discard TX status */
#define MACCTL_DISCPMQ      14 /* bit14: Discard Power Management Queue */
#define MACCTL_GMODE        15 /* bit15: G Mode */


/* The FIFO queue numbers */
#define FIFO_BK         0 /* Background */
#define FIFO_BE         1 /* Best Effort */
#define FIFO_VI         2 /* Video */
#define FIFO_VO         3 /* Voice */
#define FIFO_MCAST      4 /* Broadcast / Multicast */
#define FIFO_ATIM       5 /* ATIM window info */

/* 802.11 Frame Types  */
//-----  management : Type of Bits (2, 3) and Subtype of Bits (4, 5, 6, 7)
#define MAC_SUBTYPE_MNGMNT_ASSOC_REQUEST    0x00
#define MAC_SUBTYPE_MNGMNT_ASSOC_RESPONSE   0x10
#define MAC_SUBTYPE_MNGMNT_REASSOC_REQUEST  0x20
#define MAC_SUBTYPE_MNGMNT_REASSOC_RESPONSE 0x30
#define MAC_SUBTYPE_MNGMNT_PROBE_REQUEST    0x40
#define MAC_SUBTYPE_MNGMNT_PROBE_RESPONSE   0x50
#define MAC_SUBTYPE_MNGMNT_BEACON           0x80
#define MAC_SUBTYPE_MNGMNT_ATIM             0x90
#define MAC_SUBTYPE_MNGMNT_DISASSOCIATION   0xA0
#define MAC_SUBTYPE_MNGMNT_AUTHENTICATION   0xB0
#define MAC_SUBTYPE_MNGMNT_DEAUTHENTICATION 0xC0

//-----  control : Type of Bits (2, 3) and Subtype of Bits (4, 5, 6, 7)
#define MAC_SUBTYPE_CONTROL_PSPOLL          0xA4
#define MAC_SUBTYPE_CONTROL_RTS             0xB4
#define MAC_SUBTYPE_CONTROL_CTS             0xC4
#define MAC_SUBTYPE_CONTROL_ACK             0xD4
#define MAC_SUBTYPE_CONTROL_CFEND           0xE4
#define MAC_SUBTYPE_CONTROL_CFEND_CFACK     0xF4

//-----  data : Type of Bits (2, 3) and Subtype of Bits (4, 5, 6, 7)
#define MAC_SUBTYPE_DATA                    0x08
#define MAC_SUBTYPE_DATA_CFACK              0x18
#define MAC_SUBTYPE_DATA_CFPOLL             0x28
#define MAC_SUBTYPE_DATA_CFACK_CFPOLL       0x38
#define MAC_SUBTYPE_DATA_NULL               0x48
#define MAC_SUBTYPE_DATA_CFACK_NULL         0x58
#define MAC_SUBTYPE_DATA_CFPOLL_NULL        0x68
#define MAC_SUBTYPE_DATA_CFACK_CFPOLL_NULL  0x78

#endif /* SPECIAL_PURPOSE_REGISTER_H_ */

// vim: syntax=b43 ts=8

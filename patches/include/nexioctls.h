/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2016 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

#define IOCTL_ERROR                     -23
#define IOCTL_SUCCESS                     0

// IOCTLs used by Nexmon
#define NEX_GET_CAPABILITIES            400
#define NEX_WRITE_TO_CONSOLE            401
#define NEX_CT_EXPERIMENTS              402
#define NEX_GET_CONSOLE                 403
#define NEX_GET_PHYREG                  404
#define NEX_SET_PHYREG                  405
#define NEX_READ_OBJMEM                 406
#define NEX_WRITE_OBJMEM                407
#define NEX_INJECT_FRAME                408
#define NEX_PRINT_TIMERS                409
#define NEX_GET_SECURITYCOOKIE          410
#define NEX_SET_SECURITYCOOKIE          411
#define NEX_GET_WL_CNT                  412
#define NEX_GET_VERSION_STRING          413
#define NEX_TEST_ARGPRINTF              414
#define NEX_GET_RSPEC_OVERRIDE          415
#define NEX_SET_RSPEC_OVERRIDE          416
#define NEX_CLEAR_CONSOLE               417
#define NEX_GET_CHANSPEC_OVERRIDE       418
#define NEX_SET_CHANSPEC_OVERRIDE       419
#define NEX_GET_AMPDU_TX				420
#define NEX_SET_AMPDU_TX				421
#define NEX_TRIGGER_EVENT               422
#define NEX_TRIGGER_TDLS_DISCOVER       423
#define NEX_TRIGGER_TDLS_SETUP          424
#define NEX_TRIGGER_TDLS_TEARDOWN       425
#define NEX_WRITE_TEMPLATE_RAM          426
#define NEX_SDR_START_TRANSMISSION      427
#define NEX_SDR_STOP_TRANSMISSION       428


// IOCTLs used in original firmware
#define WLC_GET_MAGIC                     0
#define WLC_GET_VERSION                   1
#define WLC_UP                            2
#define WLC_DOWN                          3
#define WLC_GET_LOOP                      4
#define WLC_SET_LOOP                      5
#define WLC_DUMP                          6
#define WLC_GET_MSGLEVEL                  7
#define WLC_SET_MSGLEVEL                  8
#define WLC_GET_PROMISC                   9
#define WLC_SET_PROMISC                  10
#define WLC_OVERLAY_IOCTL                11
#define WLC_GET_RATE                     12
#define WLC_GET_MAX_RATE                 13
#define WLC_GET_INSTANCE                 14
#define WLC_GET_FRAG                     15
#define WLC_SET_FRAG                     16
#define WLC_GET_RTS                      17
#define WLC_SET_RTS                      18
#define NEX_READ_D11_OBJMEM              15
#define WLC_GET_INFRA                    19
#define WLC_SET_INFRA                    20
#define WLC_GET_AUTH                     21
#define WLC_SET_AUTH                     22
#define WLC_GET_BSSID                    23
#define WLC_SET_BSSID                    24
#define WLC_GET_SSID                     25
#define WLC_SET_SSID                     26
#define WLC_RESTART                      27
#define WLC_TERMINATED                   28
#define WLC_GET_CHANNEL                  29
#define WLC_SET_CHANNEL                  30
#define WLC_GET_SRL                      31
#define WLC_SET_SRL                      32
#define WLC_GET_LRL                      33
#define WLC_SET_LRL                      34
#define WLC_GET_PLCPHDR                  35
#define WLC_SET_PLCPHDR                  36
#define WLC_GET_RADIO                    37
#define WLC_SET_RADIO                    38
#define WLC_GET_PHYTYPE                  39
#define WLC_DUMP_RATE                    40
#define WLC_SET_RATE_PARAMS              41
#define WLC_GET_FIXRATE                  42
#define WLC_SET_FIXRATE                  43
#define WLC_GET_KEY                      44
#define WLC_SET_KEY                      45
#define WLC_GET_REGULATORY               46
#define WLC_SET_REGULATORY               47
#define WLC_GET_PASSIVE_SCAN             48
#define WLC_SET_PASSIVE_SCAN             49
#define WLC_SCAN                         50
#define WLC_SCAN_RESULTS                 51
#define WLC_DISASSOC                     52
#define WLC_REASSOC                      53
#define WLC_GET_ROAM_TRIGGER             54
#define WLC_SET_ROAM_TRIGGER             55
#define WLC_GET_ROAM_DELTA               56
#define WLC_SET_ROAM_DELTA               57
#define WLC_GET_ROAM_SCAN_PERIOD         58
#define WLC_SET_ROAM_SCAN_PERIOD         59
#define WLC_EVM                          60
#define WLC_GET_TXANT                    61
#define WLC_SET_TXANT                    62
#define WLC_GET_ANTDIV                   63
#define WLC_SET_ANTDIV                   64
#define WLC_GET_TXPWR                    65
#define WLC_SET_TXPWR                    66
#define WLC_GET_CLOSED                   67
#define WLC_SET_CLOSED                   68
#define WLC_GET_MACLIST                  69
#define WLC_SET_MACLIST                  70
#define WLC_GET_RATESET                  71
#define WLC_SET_RATESET                  72
#define WLC_GET_LOCALE                   73
#define WLC_LONGTRAIN                    74
#define WLC_GET_BCNPRD                   75
#define WLC_SET_BCNPRD                   76
#define WLC_GET_DTIMPRD                  77
#define WLC_SET_DTIMPRD                  78
#define WLC_GET_SROM                     79
#define WLC_SET_SROM                     80
#define WLC_GET_WEP_RESTRICT             81
#define WLC_SET_WEP_RESTRICT             82
#define WLC_GET_COUNTRY                  83
#define WLC_SET_COUNTRY                  84
#define WLC_GET_PM                       85
#define WLC_SET_PM                       86
#define WLC_GET_WAKE                     87
#define WLC_SET_WAKE                     88
#define WLC_GET_D11CNTS                  89
#define WLC_GET_FORCELINK                90
#define WLC_SET_FORCELINK                91
#define WLC_FREQ_ACCURACY                92
#define WLC_CARRIER_SUPPRESS             93
#define WLC_GET_PHYREG                   94
#define WLC_SET_PHYREG                   95
#define WLC_GET_RADIOREG                 96
#define WLC_SET_RADIOREG                 97
#define WLC_GET_REVINFO                  98
#define WLC_GET_UCANTDIV                 99
#define WLC_SET_UCANTDIV                100
#define WLC_R_REG                       101
#define WLC_W_REG                       102
#define WLC_DIAG_LOOPBACK               103
#define WLC_RESET_D11CNTS               104
#define WLC_GET_MACMODE                 105
#define WLC_SET_MACMODE                 106
#define WLC_GET_MONITOR                 107
#define WLC_SET_MONITOR                 108
#define WLC_GET_GMODE                   109
#define WLC_SET_GMODE                   110
#define WLC_GET_LEGACY_ERP              111
#define WLC_SET_LEGACY_ERP              112
#define WLC_GET_RX_ANT                  113
#define WLC_GET_CURR_RATESET            114
#define WLC_GET_SCANSUPPRESS            115
#define WLC_SET_SCANSUPPRESS            116
#define WLC_GET_AP                      117
#define WLC_SET_AP                      118
#define WLC_GET_EAP_RESTRICT            119
#define WLC_SET_EAP_RESTRICT            120
#define WLC_SCB_AUTHORIZE               121
#define WLC_SCB_DEAUTHORIZE             122
#define WLC_GET_WDSLIST                 123
#define WLC_SET_WDSLIST                 124
#define WLC_GET_ATIM                    125
#define WLC_SET_ATIM                    126
#define WLC_GET_RSSI                    127
#define WLC_GET_PHYANTDIV               128
#define WLC_SET_PHYANTDIV               129
#define WLC_AP_RX_ONLY                  130
#define WLC_GET_TX_PATH_PWR             131
#define WLC_SET_TX_PATH_PWR             132
#define WLC_GET_WSEC                    133
#define WLC_SET_WSEC                    134
#define WLC_GET_PHY_NOISE               135
#define WLC_GET_BSS_INFO                136
#define WLC_GET_PKTCNTS                 137
#define WLC_GET_LAZYWDS                 138
#define WLC_SET_LAZYWDS                 139
#define WLC_GET_BANDLIST                140
#define WLC_GET_BAND                    141
#define WLC_SET_BAND                    142
#define WLC_SCB_DEAUTHENTICATE          143
#define WLC_GET_SHORTSLOT               144
#define WLC_GET_SHORTSLOT_OVERRIDE      145
#define WLC_SET_SHORTSLOT_OVERRIDE      146
#define WLC_GET_SHORTSLOT_RESTRICT      147
#define WLC_SET_SHORTSLOT_RESTRICT      148
#define WLC_GET_GMODE_PROTECTION        149
#define WLC_GET_GMODE_PROTECTION_OVERRIDE   150
#define WLC_SET_GMODE_PROTECTION_OVERRIDE   151
#define WLC_UPGRADE                     152
#define WLC_GET_MRATE                   153
#define WLC_SET_MRATE                   154
#define WLC_GET_IGNORE_BCNS             155
#define WLC_SET_IGNORE_BCNS             156
#define WLC_GET_SCB_TIMEOUT             157
#define WLC_SET_SCB_TIMEOUT             158
#define WLC_GET_ASSOCLIST               159
#define WLC_GET_CLK                     160
#define WLC_SET_CLK                     161
#define WLC_GET_UP                      162
#define WLC_OUT                         163
#define WLC_GET_WPA_AUTH                164
#define WLC_SET_WPA_AUTH                165
#define WLC_GET_UCFLAGS                 166
#define WLC_SET_UCFLAGS                 167
#define WLC_GET_PWRIDX                  168
#define WLC_SET_PWRIDX                  169
#define WLC_GET_TSSI                    170
#define WLC_GET_SUP_RATESET_OVERRIDE    171
#define WLC_SET_SUP_RATESET_OVERRIDE    172
#define WLC_SET_FAST_TIMER              173
#define WLC_GET_FAST_TIMER              174
#define WLC_SET_SLOW_TIMER              175
#define WLC_GET_SLOW_TIMER              176
#define WLC_DUMP_PHYREGS                177
#define WLC_GET_PROTECTION_CONTROL      178
#define WLC_SET_PROTECTION_CONTROL      179
#define WLC_GET_PHYLIST                 180
#define WLC_ENCRYPT_STRENGTH            181
#define WLC_DECRYPT_STATUS              182
#define WLC_GET_KEY_SEQ                 183
#define WLC_GET_SCAN_CHANNEL_TIME       184
#define WLC_SET_SCAN_CHANNEL_TIME       185
#define WLC_GET_SCAN_UNASSOC_TIME       186
#define WLC_SET_SCAN_UNASSOC_TIME       187
#define WLC_GET_SCAN_HOME_TIME          188
#define WLC_SET_SCAN_HOME_TIME          189
#define WLC_GET_SCAN_NPROBES            190
#define WLC_SET_SCAN_NPROBES            191
#define WLC_GET_PRB_RESP_TIMEOUT        192
#define WLC_SET_PRB_RESP_TIMEOUT        193
#define WLC_GET_ATTEN                   194
#define WLC_SET_ATTEN                   195
#define WLC_GET_SHMEM                   196
#define WLC_SET_SHMEM                   197
#define WLC_GET_GMODE_PROTECTION_CTS    198
#define WLC_SET_GMODE_PROTECTION_CTS    199
#define WLC_SET_WSEC_TEST               200
#define WLC_SCB_DEAUTHENTICATE_FOR_REASON   201
#define WLC_TKIP_COUNTERMEASURES        202
#define WLC_GET_PIOMODE                 203
#define WLC_SET_PIOMODE                 204
#define WLC_SET_ASSOC_PREFER            205
#define WLC_GET_ASSOC_PREFER            206
#define WLC_SET_ROAM_PREFER             207
#define WLC_GET_ROAM_PREFER             208
#define WLC_SET_LED                     209
#define WLC_GET_LED                     210
#define WLC_GET_INTERFERENCE_MODE       211
#define WLC_SET_INTERFERENCE_MODE       212
#define WLC_GET_CHANNEL_QA              213
#define WLC_START_CHANNEL_QA            214
#define WLC_GET_CHANNEL_SEL             215
#define WLC_START_CHANNEL_SEL           216
#define WLC_GET_VALID_CHANNELS          217
#define WLC_GET_FAKEFRAG                218
#define WLC_SET_FAKEFRAG                219
#define WLC_GET_PWROUT_PERCENTAGE       220
#define WLC_SET_PWROUT_PERCENTAGE       221
#define WLC_SET_BAD_FRAME_PREEMPT       222
#define WLC_GET_BAD_FRAME_PREEMPT       223
#define WLC_SET_LEAP_LIST               224
#define WLC_GET_LEAP_LIST               225
#define WLC_GET_CWMIN                   226
#define WLC_SET_CWMIN                   227
#define WLC_GET_CWMAX                   228
#define WLC_SET_CWMAX                   229
#define WLC_GET_WET                     230
#define WLC_SET_WET                     231
#define WLC_GET_PUB                     232
#define WLC_GET_KEY_PRIMARY             235
#define WLC_SET_KEY_PRIMARY             236
#define WLC_GET_VAR                     262     /* get value of named variable */
#define WLC_SET_VAR                     263     /* set named variable to value */

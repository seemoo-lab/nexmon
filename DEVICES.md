## BCM4325
802.11abg + BT 2.1

- Acer Liquid
- Apple iPhone 3GS
- Apple iPod 2G
- Ford Edge (it’s a car)
- HTC Droid Incredible
- HTC Touch Pro 2
- Motorola Devour
- Samsung Spica

## BCM4329
802.11abgn MIMO 1x1 + BT 2.1

- Apple iPad 2
- Apple iPad 3G
- Apple iPad Wi-Fi
- Apple iPhone 4
- Apple iPhone 4 Verizon
- Apple iPod 3G
- Apple Tv 2G
- Asus Transformer Prime
- HTC Droid Incredible 2
- HTC Evo 4G
- HTC Nexus One
- HTC ThunderBolt
- Kyocera Echo
- LG Revolution
- Malata ZPad
- Motorola Atrix
- Motorola Droid X2
- Motorola Xoom
- Nokia Lumina 800
- Pantech Breakout
- Samsung Fascinate
- Samsung Galaxy S 4G
- Samsung Galaxy Tab
- Samsung Nexus S
- Samsung Stratosphere
- Sony Ericsson Xperia Play

## BCM4330
802.11abgn MIMO 1x1 + BT 4.0

- Nexus 7 (2012)
- Samsung Note 1
- Samsung S2

## BCM4335
802.11abgn+ac MIMO 1x1 + BT 4.0

- LG G2
- HTC One (M7)
- HTC One Mini
- Samsung S4 (I9500)
- Samsung S4 Intl (I9505)

## BCM4339
(4339a0) 802.11abgn+ac MIMO 1x1 + BT 4.1

- LG G3
- LG Nexus 5
- Sony Experia Z2 sgp521 castor (Tablet)
- Sony Xperia Z3 D6603

## BCM4356
802.11abgn+ac MIMO 2x2 + BT 4.1

- Nexus 6
- Sony Xperia Z5 E6653

## BCM4358
802.11abgn+ac MIMO 2x2 + BT 4.1

- Huawei Nexus 6P
- Samsung S6

## BCM43596
(bcm43596a0) 802.11abgn+ac MIMO 2x2 + BT 4.1

- Samsung S7
- Samsung S7 Edge

## BCM43455
(43455c0) 802.11abgn+ac MIMO 1x1 + BT 4.1

- Huawei P9
- LG G5

```bash
dmesg | grep -i dhd  # Broadcom chip
```

## WCN36xx (Qualcomm)

- LG Nexus 4

```bash
dmesg | grep -i wcn  # WCNxxx chip (wcnss) Qualcomm
```

## QCA (Qualcomm Atheros)

- Oneplus 3/3T

```bash
dmesg | grep -i cnss  # Qualcomm Atheros (QCA6174 etc)
lspci -k   # 168c:003e cnss_wlan_pci  # Qualcomm Atheros
```

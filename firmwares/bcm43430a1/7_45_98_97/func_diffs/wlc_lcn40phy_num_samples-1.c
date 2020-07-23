
uint wlc_lcn40phy_num_samples(undefined4 pi,uint f_Hz,int phy_bw)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  if (f_Hz != 0) {
    uVar2 = 1;
    do {
      uVar3 = DAT_00024c70 * phy_bw * uVar2;
      uVar4 = (f_Hz ^ (int)f_Hz >> 0x1f) - ((int)f_Hz >> 0x1f);
      uVar1 = uVar3 / uVar4 & 0xffff;
      uVar2 = uVar2 + 1 & 0xffff;
    } while (uVar1 * uVar4 - uVar3 != 0);
    return uVar1;
  }
  return 2;
}


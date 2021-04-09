
void wlc_lcn40phy_tx_tone_samples
               (int pi,uint f_Hz,int max_val,uint *data_buf,int phy_bw,short num_samps)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  short sVar6;
  int local_28;
  uint local_24;
  
  sVar6 = 0;
  uVar4 = ((((f_Hz ^ (int)f_Hz >> 0x1f) - ((int)f_Hz >> 0x1f)) * 0x24) / (uint)(phy_bw * 1000) <<
          0x10) / 100;
  iVar5 = 0;
  local_28 = pi;
  local_24 = f_Hz;
  while (sVar6 != num_samps) {
    FUN_0002edd8(iVar5,&local_28);
    uVar2 = uVar4;
    if ((int)f_Hz < 1) {
      uVar2 = -uVar4;
    }
    iVar5 = iVar5 + uVar2;
    iVar1 = max_val * local_24;
    if (iVar1 < 0) {
      uVar2 = -((-iVar1 >> 0xf) + 1 >> 1);
    }
    else {
      uVar2 = (iVar1 >> 0xf) + 1 >> 1;
    }
    iVar1 = max_val * local_28;
    if (iVar1 < 0) {
      uVar3 = -((-iVar1 >> 0xf) + 1 >> 1);
    }
    else {
      uVar3 = (iVar1 >> 0xf) + 1 >> 1;
    }
    sVar6 = sVar6 + 1;
    *data_buf = uVar3 & 0x3ff | (uVar2 & 0x3ff) << 10;
    data_buf = data_buf + 1;
  }
  return;
}



void wlc_lcn40phy_start_tx_tone
               (int pi,uint f_Hz,int max_val,char iqcalmode,byte deaf_set_to_1,byte tx_pu_param)

{
  uint *puVar1;
  uint uVar2;
  char *pcVar3;
  uint *local_34;
  uint local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  
  puVar1 = (uint *)FUN_0002f6c6(*(byte **)(pi + 0xe4),0x400);
  if (puVar1 != (uint *)0x0) {
    *(uint *)(pi + 0x174) = f_Hz;
    phy_reg_mod(pi,0x49c,1,1);
    wlc_lcn40phy_deaf_mode(pi,(uint)deaf_set_to_1);
    uVar2 = wlc_lcn40phy_num_samples(pi,f_Hz,0x28);
    if (uVar2 < 0x101) {
      phy_reg_mod(pi,0x6d6,3,0);
      phy_reg_mod(pi,0x6da,8,8);
      FUN_00033272(pi,f_Hz,max_val,puVar1,0x28,(short)uVar2);
      local_2c = 0x15;
      local_24 = 0x20;
      local_28 = 0;
      local_34 = puVar1;
      local_30 = uVar2;
      FUN_0003243e(pi,&local_34);
      wlc_lcn40phy_run_samples(pi,(short)uVar2,0xffff,0,iqcalmode,tx_pu_param);
      pcVar3 = *(char **)(pi + 0xe4);
    }
    else {
      pcVar3 = *(char **)(pi + 0xe4);
    }
    FUN_0002f6e0(pcVar3);
  }
  return;
}


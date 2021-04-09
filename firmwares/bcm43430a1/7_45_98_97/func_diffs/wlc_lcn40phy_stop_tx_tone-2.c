
void wlc_lcn40phy_stop_tx_tone(int pi)

{
  uint uVar1;
  ushort addr;
  ushort mask;
  ushort val;
  int iVar2;
  
  *(undefined4 *)(pi + 0x174) = 0;
  phy_reg_read(pi,0x644);
  iVar2 = 10;
  do {
    uVar1 = phy_reg_read(pi,0x644);
    val = (ushort)(uVar1 & 1);
    if ((uVar1 & 1) == 0) {
      if ((int)(uVar1 << 0x1e) < 0) {
        addr = 0x453;
        mask = 0x8000;
        goto LAB_00035b08;
      }
    }
    else {
      FUN_0002feb4(pi,0,0);
      mask = 2;
      addr = 0x63f;
      val = 2;
LAB_00035b08:
      phy_reg_mod(pi,addr,mask,val);
    }
    func_0x008081f0(1);
    uVar1 = phy_reg_read(pi,0x644);
    iVar2 = iVar2 + -1;
    if ((iVar2 == 0) || ((uVar1 & 3) == 0)) {
      FUN_0002d9f0(pi,(ushort *)PTR_DAT_00035b48,0xb);
      wlc_lcn40phy_deaf_mode(pi,0);
      phy_reg_mod(pi,0x49c,1,0);
      return;
    }
  } while( true );
}


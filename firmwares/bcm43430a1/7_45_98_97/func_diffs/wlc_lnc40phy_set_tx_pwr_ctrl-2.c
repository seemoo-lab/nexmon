
void wlc_lcn40phy_set_tx_pwr_ctrl(int pi,uint mode)

{
  undefined2 uVar1;
  uint uVar2;
  ushort val;
  int iVar3;
  
  iVar3 = *(int *)(pi + 0xe4);
  uVar2 = phy_reg_read(pi,0x4a4);
  if ((uVar2 & 0xe000) == mode) {
    return;
  }
  phy_reg_mod(pi,0x6da,0x40,0);
  if (mode == 0xe000) {
    val = 0;
  }
  else {
    val = 0x10;
  }
  phy_reg_mod(pi,0x6a3,0x10,val);
  if (mode == 0xe000) {
    val = 0x2000;
  }
  else {
    val = 0;
  }
  phy_reg_mod(pi,0x637,0x2000,val);
  if (mode == 0xe000) {
    val = 0x1000;
  }
  else {
    val = 0;
  }
  phy_reg_mod(pi,0x637,0x1000,val);
  if (mode == 0xe000) {
    val = 4;
  }
  else {
    val = 0;
  }
  phy_reg_mod(pi,0x4d0,4,val);
  phy_reg_mod(pi,0x478,3,0);
  if ((uVar2 & 0xe000) == 0xe000) {
    FUN_000324ec(pi);
    FUN_0002d2d8(pi,0x46e,0);
  }
  if (mode == 0xe000) {
    FUN_00032e66(pi);
    phy_reg_mod(pi,0x4a4,0x1ff,*(short *)(iVar3 + 0x402) << 1);
    phy_reg_mod(pi,0x480,0x1ff,*(short *)(iVar3 + 0x404) << 1);
    phy_reg_mod(pi,0x4a5,0x1c00,*(short *)(iVar3 + 0x434) << 10);
    FUN_0002d2d8(pi,0x46e,1);
    uVar1 = FUN_000375d4();
    *(undefined2 *)(iVar3 + 0x436) = uVar1;
    FUN_0002f972(pi,0);
    *(undefined *)(iVar3 + 0x444) = 0xff;
    phy_reg_mod(pi,0x807,4,0);
  }
  else {
    FUN_0002f972(pi,1);
  }
  phy_reg_mod(pi,0x4a4,0xe000,(ushort)mode);
  return;
}


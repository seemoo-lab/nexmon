
void wlc_lcn40phy_write_table(int pi,uint **pti)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar1 = phy_reg_read(pi,0x9a5);
  phy_reg_mod(pi,0x9a5,2,2);
  if (pti[2] == (uint *)0x21) {
    iVar2 = phy_reg_read(pi,0x574);
    iVar3 = phy_reg_read(pi,0x575);
    phy_reg_mod(pi,0x574,0x4000,0x4000);
    phy_reg_mod(pi,0x575,0x400,0x400);
  }
  else {
    iVar3 = 0;
    iVar2 = 0;
  }
  FUN_0002321e(pi,pti,0xd,0xe,0xe,0x457,0x456);
  if (pti[2] == (uint *)0x21) {
    FUN_000229f0(pi,0x574,iVar2);
    FUN_000229f0(pi,0x575,iVar3);
  }
  FUN_000229f0(pi,0x9a5,iVar1);
  return;
}


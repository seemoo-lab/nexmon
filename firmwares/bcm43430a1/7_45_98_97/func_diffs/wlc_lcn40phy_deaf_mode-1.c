
void wlc_lcn40phy_deaf_mode(int pi,uint mode)

{
  ushort val;
  
  val = 1 - (short)mode;
  if (1 < mode) {
    val = 0;
  }
  phy_reg_mod(pi,0x440,1,val);
  return;
}


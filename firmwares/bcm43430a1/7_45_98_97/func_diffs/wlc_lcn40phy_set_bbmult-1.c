
void wlc_lcn40phy_set_bbmult(int pi,uint8_t m0)

{
  uint *local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  ushort local_a;
  
  local_20 = (uint *)&local_a;
  local_a = (ushort)m0;
  local_1c = 1;
  local_18 = 0;
  local_14 = 99;
  local_10 = 0x10;
  FUN_000257dc(pi,&local_20);
  local_14 = 0x73;
  FUN_000257dc(pi,&local_20);
  return;
}


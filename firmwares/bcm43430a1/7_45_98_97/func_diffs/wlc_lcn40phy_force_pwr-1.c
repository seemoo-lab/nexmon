
void wlc_lnc40phy_force_pwr_index(int pi,int indx)

{
  int iVar1;
  uint *local_4c;
  undefined4 local_48;
  undefined4 local_44;
  int local_40;
  undefined4 local_3c;
  ushort local_38;
  ushort local_36;
  ushort local_34;
  ushort local_32;
  uint local_30;
  uint local_2c;
  undefined2 local_28 [2];
  uint local_24 [2];
  
  iVar1 = *(int *)(pi + 0xe4);
  *(undefined *)(iVar1 + 0x418) = (char)indx;
  *(undefined *)(iVar1 + 0x419) = (char)indx;
  local_40 = indx + 0x240;
  local_4c = &local_30;
  local_44 = 7;
  local_3c = 0x20;
  local_48 = 1;
  FUN_000255ec(pi,&local_4c);
  local_40 = indx + 0xc0;
  local_4c = &local_2c;
  local_3c = 0x20;
  FUN_000255ec(pi,&local_4c);
  local_38 = (ushort)local_2c & 0x1f;
  local_34 = (ushort)(local_2c >> 0xd) & 0xff;
  local_36 = (ushort)(local_2c >> 5) & 0xff;
  local_32 = (ushort)(local_30 >> 0x11) & 0xf;
  FUN_0002534e(pi,&local_38);
  FUN_00024e34(pi,local_2c >> 0x15 & 0xff);
  FUN_00025904(pi,(ushort)(local_30 >> 9) & 0xff);
  phy_reg_mod(pi,0x807,4,4);
  phy_reg_mod(pi,0x807,0x1ff0,(ushort)((local_30 & 0x1ff) << 4));
  FUN_00024e0c(pi,1);
  local_40 = 0x140;
  local_4c = local_24;
  local_44 = 7;
  local_3c = 0x20;
  local_48 = 1;
  FUN_000255ec(pi,&local_4c);
  FUN_000259fa(pi);
  local_3c = 0x10;
  local_4c = (uint *)local_28;
  local_40 = indx + 0x1c0;
  FUN_000255ec(pi,&local_4c);
  FUN_00025a22(pi,local_28[0]);
  phy_reg_mod(pi,0x6a6,0x1fff,0);
  return;
}


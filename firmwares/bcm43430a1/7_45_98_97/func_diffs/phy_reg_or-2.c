
void phy_reg_or(int pi,uint16 addr,ushort val)

{
  ushort in_r2;
  int iVar1;
  
  iVar1 = *(int *)(pi + 0xe8);
  *(ushort *)(iVar1 + 0x3fc) = val;
  *(ushort *)(iVar1 + 0x3fe) = in_r2 | *(ushort *)(iVar1 + 0x3fe);
  *(undefined2 *)(pi + 300) = 0;
  return;
}


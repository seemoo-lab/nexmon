
int phy_reg_read(int pi,int add)

{
  int iVar1;
  
  iVar1 = *(int *)(pi + 0xe8);
  *(undefined2 *)(iVar1 + 0x3fc) = (short)add;
  *(undefined2 *)(pi + 300) = 0;
  return (uint)*(ushort *)(iVar1 + 0x3fe);
}


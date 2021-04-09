
void phy_reg_mod(int pi,ushort addr,ushort mask,ushort val)

{
  int iVar1;
  
  iVar1 = *(int *)(pi + 0xe8);
  *(ushort *)(iVar1 + 0x3fc) = addr;
  *(ushort *)(iVar1 + 0x3fe) = *(ushort *)(iVar1 + 0x3fe) & ~mask | val & mask;
  *(undefined2 *)(pi + 300) = 0;
  return;
}


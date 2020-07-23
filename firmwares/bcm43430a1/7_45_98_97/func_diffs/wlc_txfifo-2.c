
void wlc_txfifo(int wlc,int fifo,int p,undefined4 *txh,byte commit,char txpktpend)

{
  int iVar1;
  int local_28;
  short sVar2;
  int iVar3;
  uint uVar4;
  
  iVar3 = *(int *)(*(int *)(p + 0x28) + 0x10);
  uVar4 = (uint)*(ushort *)txh[7];
  local_28 = wlc;
  if ((*(char *)(iVar3 + 6) == '\0') && (-1 < (int)((uint)*(byte *)(p + 0x1f) << 0x1d))) {
    iVar1 = (int)(uVar4 & 0xc) >> 2;
    if (iVar1 == 1) {
      if ((*(ushort *)txh[7] >> 4 & 10) != 0) goto LAB_00014814;
    }
    else {
      if (((iVar1 == 2) && ((int)(uVar4 << 0x19) < 0)) &&
         ((**(char **)(iVar3 + 0x104) != '\x01' || (-1 < (int)(uVar4 << 0x1c))))) goto LAB_00014814;
    }
    local_28 = fifo;
    func_0x0081fb48(wlc,*(int *)(p + 0x28),p,1,fifo,fifo,p);
  }
LAB_00014814:
  if (*(int *)(p + 0x18) << 0x16 < 0) {
    *txh = *(undefined4 *)(p + 0x24);
    FUN_0000ead6(wlc,(int)txh);
    func_0x0082f244(wlc,p);
  }
  if (fifo == 4) {
    sVar2 = *(short *)(txh + 1);
  }
  else {
    sVar2 = -1;
  }
  if (commit != 0) {
    iVar3 = *(int *)(wlc + 0x1c) + (fifo + 0xc) * 2;
    *(short *)(iVar3 + 4) = *(short *)(iVar3 + 4) + (short)txpktpend;
  }
  if (sVar2 != -1) {
    func_0x008467c0(*(undefined4 *)(wlc + 0x10),0xa8);
  }
  local_28 = (**(code **)(DAT_00014894 + 0x28))
                       (*(undefined4 *)(*(int *)(wlc + 0x14) + fifo * 4),p,(uint)commit,
                        *(code **)(DAT_00014894 + 0x28),local_28);
  if ((local_28 < 0) && (commit != 0)) {
    local_28 = *(int *)(wlc + 0x1c) + (fifo + 0xc) * 2;
    *(short *)(local_28 + 4) = *(short *)(local_28 + 4) - (short)txpktpend;
  }
  return;
}


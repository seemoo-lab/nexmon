
void pkt_buf_get_skb(undefined4 osh,uint len)

{
  uint uVar1;
  char *pcVar2;
  undefined8 uVar3;
  
  uVar3 = func_0x00808744(osh,len,0);
  if (((((int)((ulonglong)uVar3 >> 0x20) == 0) && (pcVar2 = *DAT_00006380, pcVar2 != (char *)0x0))
      && (*pcVar2 != '\0')) && (len <= (uint)*(ushort *)(pcVar2 + 0xe))) {
    uVar1 = FUN_00004660((int)pcVar2,(int)uVar3,(uint)*(ushort *)(pcVar2 + 0xe),pcVar2);
    if (uVar1 != 0) {
      *(int *)PTR_DAT_00006384 = *(int *)PTR_DAT_00006384 + 1;
      return;
    }
    *(int *)PTR_DAT_00006388 = *(int *)PTR_DAT_00006388 + 1;
  }
  return;
}



void pkt_buf_free_skb(ushort **osh,ushort *p,int send,undefined4 param_4)

{
  ushort *extraout_r1;
  ushort *puVar1;
  int iVar2;
  ushort *puVar3;
  
  puVar1 = p;
  if ((send != 0) && (puVar3 = osh[2], puVar3 != (ushort *)0x0)) {
    (*(code *)puVar3)(osh[3],p,0,puVar3,param_4);
    puVar1 = extraout_r1;
  }
  iVar2 = *DAT_000063c4;
  puVar3 = p;
  while (puVar3 != (ushort *)0x0) {
    puVar1 = (ushort *)(uint)*(byte *)((int)puVar3 + 3);
    if (*(byte *)((int)puVar3 + 3) == 0) {
      puVar1 = (ushort *)((int)*osh + -1);
      *osh = puVar1;
    }
    puVar3 = *(ushort **)(iVar2 + (uint)puVar3[10] * 4);
  }
  FUN_00002d34(p,puVar1);
  return;
}


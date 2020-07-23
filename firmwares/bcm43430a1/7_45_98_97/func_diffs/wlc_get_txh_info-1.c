
void wlc_get_txh_info(int wlc,int p,undefined4 *txh)

{
  undefined2 uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined2 *puVar4;
  
  if (((p != 0) && (txh != (undefined4 *)0x0)) &&
     (puVar4 = *(undefined2 **)(p + 8), puVar4 != (undefined2 *)0x0)) {
    txh[8] = 0;
    txh[9] = 0;
    uVar2 = *(undefined4 *)(wlc + 4);
    *(undefined2 *)(txh + 1) = puVar4[0x26];
    *txh = *(undefined4 *)(puVar4 + 0x21);
    *(undefined2 *)((int)txh + 6) = *puVar4;
    uVar1 = puVar4[1];
    *(undefined2 **)(txh + 6) = puVar4;
    *(undefined2 *)(txh + 2) = uVar1;
    txh[5] = 0x70;
    *(undefined2 **)(txh + 7) = puVar4 + 0x3b;
    *(undefined2 **)(txh + 10) = puVar4 + 0x13;
    *(undefined2 **)(txh + 0xb) = puVar4 + 0x38;
    *(undefined2 *)((int)txh + 10) = puVar4[4];
    *(undefined2 *)(txh + 3) = puVar4[5];
    *(ushort *)(txh + 0xc) = (ushort)puVar4[0x46] >> 4;
    iVar3 = FUN_00004744(uVar2,p);
    *(short *)((int)txh + 0x12) = (short)iVar3 + -0x76;
    *(undefined2 *)((int)txh + 0xe) = 0;
    *(undefined2 *)(txh + 4) = puVar4[0x23];
  }
  return;
}


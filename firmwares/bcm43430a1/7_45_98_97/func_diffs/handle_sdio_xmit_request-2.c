
void handle_sdio_xmit_request(int sdio_hw,int p,undefined4 param_3)

{
  int iVar1;
  undefined2 uVar2;
  uint uVar3;
  code *pcVar4;
  undefined2 *puVar5;
  undefined2 *puVar6;
  int iVar7;
  undefined2 *puVar8;
  ushort *puVar9;
  uint uVar10;
  int iVar11;
  uint uVar12;
  
  puVar5 = (undefined2 *)0x0;
  uVar12 = 0;
  iVar7 = 0;
  puVar8 = (undefined2 *)0x0;
  iVar11 = sdio_hw;
  do {
    if ((ushort *)p == (ushort *)0x0) {
      return;
    }
    puVar9 = *(ushort **)(*DAT_00004864 + (uint)((ushort *)p)[0xb] * 4);
    uVar3 = *(uint *)((ushort *)p + 4);
    uVar10 = (uint)*(byte *)(uVar3 + 2) & 0xf;
    if (puVar9 != (ushort *)0x0) {
      uVar3 = (uint)*(byte *)(*(int *)(puVar9 + 4) + 2) & 0xf;
      uVar12 = uVar3;
    }
    iVar1 = FUN_00004684(*(int *)(sdio_hw + 0xc),(ushort *)p);
    if (iVar1 == 0) {
      if (puVar5 == (undefined2 *)0x0) {
        iVar7 = FUN_000046f8(sdio_hw,uVar10);
        if (iVar7 == 0) {
          FUN_0000b3e8(*(ushort ***)(sdio_hw + 4),(ushort *)p,0,uVar3);
          iVar1 = 1;
          goto LAB_00004812;
        }
        puVar6 = (undefined2 *)FUN_0000b460(*(int **)(sdio_hw + 4),p);
        puVar8 = puVar6;
      }
      else {
        puVar6 = (undefined2 *)FUN_0000b460(*(int **)(sdio_hw + 4),p);
        if (puVar6 == (undefined2 *)0x0) {
          uVar2 = 0;
        }
        else {
          uVar2 = *puVar6;
        }
        puVar5[0xb] = uVar2;
      }
      *(int *)(sdio_hw + 0x10) = *(int *)(sdio_hw + 0x10) + 1;
      puVar5 = puVar6;
    }
    else {
LAB_00004812:
      *(int *)(sdio_hw + 0x28) = *(int *)(sdio_hw + 0x28) + 1;
    }
    p = (int)puVar9;
    if ((puVar8 != (undefined2 *)0x0) &&
       (((puVar9 == (ushort *)0x0 || (iVar1 != 0)) || (uVar10 != uVar12)))) {
      puVar5[0xb] = 0;
      pcVar4 = *(code **)(*(int *)(iVar7 + 0x10) + 0xc);
      puVar5 = (undefined2 *)
               (*pcVar4)(*(undefined4 *)(sdio_hw + 0x38),iVar7,puVar8,pcVar4,iVar11,uVar12,param_3);
      puVar8 = puVar5;
      if (puVar5 != (undefined2 *)0x0) {
        puVar5 = (undefined2 *)0x0;
        *(int *)(sdio_hw + 0x28) = *(int *)(sdio_hw + 0x28) + 1;
        puVar8 = (undefined2 *)0x0;
      }
    }
  } while( true );
}


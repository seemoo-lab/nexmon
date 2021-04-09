
ushort * dngl_sendpkt(int sdio,ushort *p,uint chan)

{
  ushort uVar1;
  undefined4 *puVar2;
  uint uVar3;
  int iVar4;
  ushort *puVar5;
  uint uVar6;
  ushort *puVar7;
  ushort **ppuVar8;
  uint local_34;
  undefined4 local_30;
  ushort local_2a [3];
  
  uVar3 = (uint)*(byte *)(sdio + 0x178);
  puVar7 = (ushort *)0x0;
  ppuVar8 = *(ushort ***)(sdio + 8);
  local_30 = 0;
  local_34 = 0;
  if (*(byte *)(sdio + 0x178) == 0) {
    uVar6 = *(uint *)(sdio + 0x1d8) & *(uint *)(p + 4);
    if (((*(char *)(sdio + 0x22a) != '\0') && (*(int *)(sdio + 4) - 0x11U < 6)) && (p[6] < 0xf4)) {
      uVar6 = (uVar6 + 0xf4) - ((uint)p[6] & 0xfffffffc);
    }
    uVar3 = (*(uint *)(p + 4) & 0x1fffff) - (*(uint *)(p + 2) & 0x1fffff);
    puVar7 = p;
    if (uVar3 < uVar6 + 0xc) {
      uVar1 = p[6];
      puVar7 = (ushort *)FUN_0000b3a4(ppuVar8,*(int *)(sdio + 0x1e4) + (uint)uVar1 + uVar6);
      if (puVar7 == (ushort *)0x0) goto LAB_00003812;
      iVar4 = uVar6 + *(int *)(sdio + 0x1e4);
      puVar2 = (undefined4 *)(*(int *)(puVar7 + 4) + iVar4);
      *(undefined4 **)(puVar7 + 4) = puVar2;
      puVar7[6] = puVar7[6] - (short)iVar4;
      FUN_00002414(puVar2,*(undefined4 **)(p + 4),(uint)uVar1);
      puVar5 = *(ushort **)(*DAT_000038e8 + (uint)p[10] * 4);
      if (puVar5 != (ushort *)0x0) {
        puVar5 = (ushort *)(uint)*puVar5;
      }
      puVar7[10] = (ushort)puVar5;
      FUN_0000b3e8(ppuVar8,p,0,puVar5);
    }
    if (uVar6 != 0) {
      iVar4 = *(int *)(puVar7 + 4);
      *(uint *)(puVar7 + 4) = iVar4 - uVar6;
      puVar7[6] = (short)uVar6 + puVar7[6];
      func_0x00803b14(iVar4 - uVar6,0,uVar6);
    }
    puVar2 = (undefined4 *)(*(int *)(puVar7 + 4) - *(int *)(sdio + 0x1e4));
    puVar7[6] = (short)*(int *)(sdio + 0x1e4) + puVar7[6];
    *(undefined4 **)(puVar7 + 4) = puVar2;
    iVar4 = FUN_00009338(ppuVar8,(int)puVar7);
    local_2a[0] = (ushort)iVar4;
    FUN_00002414(puVar2,(undefined4 *)local_2a,2);
    local_2a[0] = ~local_2a[0];
    FUN_00002414((undefined4 *)((int)puVar2 + 2),(undefined4 *)local_2a,2);
    uVar3 = (chan & 0xf) << 8 | (uint)*(byte *)(sdio + 0x108) | (uVar6 + 0xc) * 0x1000000;
    local_34 = uVar3;
    FUN_00002414(puVar2 + 1,&local_34,8);
    puVar7 = (ushort *)FUN_000035f8(sdio,puVar7,chan,uVar3);
    if (puVar7 != (ushort *)0x0) {
      puVar7 = (ushort *)(Reset + 1);
      *(char *)(sdio + 0x108) = *(char *)(sdio + 0x108) + '\x01';
    }
  }
  else {
LAB_00003812:
    FUN_0000b3e8(ppuVar8,p,1,uVar3);
  }
  return puVar7;
}


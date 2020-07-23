
undefined2 * dma_rx(int di)

{
  ushort *puVar1;
  uint uVar2;
  ushort **ppuVar3;
  ushort *puVar4;
  int iVar5;
  ushort uVar6;
  ushort *puVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  
  uVar10 = 0;
  do {
    puVar1 = (ushort *)FUN_00009dc8(di,0);
    if (puVar1 == (ushort *)0x0) {
      return (undefined2 *)0x0;
    }
    puVar7 = *(ushort **)(puVar1 + 4);
    if ((*(uint *)(di + 8) & 0x80) == 0) {
      uVar8 = (uint)*puVar7;
    }
    else {
      uVar8 = (uint)puVar7[(*(uint *)(di + 0xa0) >> 1) + 2] + 4;
      *puVar7 = (ushort)uVar8;
    }
    iVar5 = *(int *)(di + 0xa0);
    uVar2 = uVar8 + iVar5;
    uVar9 = (uint)*(ushort *)(di + 0x94);
    if (uVar2 < (uint)*(ushort *)(di + 0x94)) {
      uVar9 = uVar2;
    }
    uVar6 = (ushort)uVar9;
    if (*(char *)(di + 0x32) == '\0') {
      puVar1[6] = uVar6;
    }
    else {
      uVar10 = (uint)puVar1[6];
      if (uVar10 < uVar9) {
        uVar6 = uVar6 - puVar1[6];
      }
      else {
        puVar1[6] = uVar6;
        uVar6 = 0;
      }
      puVar1[0x28] = uVar6;
    }
    uVar8 = (iVar5 - (uint)*(ushort *)(di + 0x94)) + uVar8;
    if ((int)uVar8 < 1) {
      return puVar1;
    }
    uVar9 = *(int *)(di + 8) << 0x19;
    puVar7 = puVar1;
    if ((int)uVar9 < 0) {
      ppuVar3 = *(ushort ***)(di + 0x28);
      uVar8 = 0;
    }
    else {
      do {
        puVar4 = (ushort *)FUN_00009dc8(di,0);
        if (puVar4 == (ushort *)0x0) break;
        puVar7[10] = *puVar4;
        uVar9 = (uint)*(ushort *)(di + 0x94);
        uVar2 = uVar8;
        if ((int)uVar9 <= (int)uVar8) {
          uVar2 = uVar9;
        }
        uVar6 = (ushort)uVar2;
        if ((*(char *)(di + 0x32) != '\0') && (uVar10 <= uVar2)) {
          uVar6 = (ushort)uVar10;
        }
        uVar8 = uVar8 - uVar9;
        puVar4[6] = uVar6;
        puVar7 = puVar4;
      } while (0 < (int)uVar8);
      uVar8 = *(uint *)(di + 8) & 4;
      if (uVar8 != 0) {
        return puVar1;
      }
      ppuVar3 = *(ushort ***)(di + 0x28);
    }
    FUN_0000b3e8(ppuVar3,puVar1,uVar8,uVar9);
    *(int *)(di + 0xc) = *(int *)(di + 0xc) + 1;
  } while( true );
}


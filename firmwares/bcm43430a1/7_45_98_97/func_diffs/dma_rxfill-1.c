
uint dma_rxfill(int di)

{
  ushort uVar1;
  byte bVar2;
  byte *pbVar3;
  uint uVar4;
  int iVar5;
  int extraout_r1;
  int extraout_r1_00;
  uint local_5c;
  char *pcVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  undefined4 *puVar11;
  uint uVar12;
  int iVar13;
  int iVar14;
  bool bVar15;
  int local_4c;
  uint local_2c [2];
  
  local_2c[0] = 0;
  bVar2 = *(byte *)(di + 0x32);
  uVar4 = (uint)bVar2;
  if ((*(uint *)(di + 8) & 0x10) == 0) {
    iVar14 = 1;
  }
  else {
    iVar14 = 0x10;
  }
  if (bVar2 == 0) {
    local_5c = 1;
  }
  else {
    local_5c = 2;
  }
  if (*(char *)(di + 0xde) != '\0') {
    pcVar6 = *(char **)(di + 0xbc);
    if ((pcVar6 == (char *)0x0) || (*pcVar6 == '\0')) {
      uVar1 = *(ushort *)(di + 0x94);
    }
    else {
      uVar1 = *(ushort *)(pcVar6 + 0xe);
    }
    local_5c = (uint)uVar1;
    if (bVar2 != 0) {
      local_5c = 0x800 - local_5c;
      uVar4 = 1;
    }
    local_5c = uVar4 + (local_5c + 0x1eb) / 0x1ec;
  }
  uVar8 = (uint)*(ushort *)(di + 0x70);
  uVar4 = (uint)*(ushort *)(di + 0x6c) / local_5c;
  if (*(uint *)(di + 0x9c) < uVar4) {
    uVar4 = *(uint *)(di + 0x9c);
  }
  iVar5 = ((1 - local_5c) -
          (uVar8 - *(ushort *)(di + 0x6e) & (uint)*(ushort *)(di + 0x6c) - 1 & 0xffff)) / local_5c +
          uVar4;
  if (*(ushort *)(di + 0x94) < 0xcd) {
    local_4c = 0;
  }
  else {
    local_4c = *(int *)(di + 0x98);
  }
  iVar13 = 0;
  iVar7 = iVar5;
  do {
    if (iVar13 == iVar5) {
      uVar4 = 0;
LAB_000053e2:
      *(undefined2 *)(di + 0x70) = (short)uVar8;
      *(int *)(*(int *)(di + 0x38) + 4) = *(int *)(di + 0x90) + uVar8 * 0x10;
      return uVar4;
    }
    pbVar3 = *(byte **)(di + 0xbc);
    if ((pbVar3 == (byte *)0x0) || (*pbVar3 == 0)) {
      uVar4 = FUN_00006348(*(undefined4 *)(di + 0x28),
                           ((uint)*(ushort *)(di + 0x94) - 1) + iVar14 + local_4c);
    }
    else {
      uVar4 = FUN_00004660((int)pbVar3,iVar7,iVar5,(uint)*pbVar3);
    }
    if (uVar4 == 0) {
      if ((iVar13 == 0) && (uVar4 = func_0x00807528(di), uVar4 != 0)) {
        uVar4 = 1;
      }
      *(int *)(di + 0x10) = *(int *)(di + 0x10) + 1;
      goto LAB_000053e2;
    }
    local_5c = *(uint *)(di + 8) & 0x10;
    if (local_5c != 0) {
      local_5c = iVar14 - *(int *)(uVar4 + 8) & iVar14 - 1U;
    }
    iVar7 = local_5c + local_4c;
    if (iVar7 != 0) {
      *(int *)(uVar4 + 8) = *(int *)(uVar4 + 8) + iVar7;
      *(short *)(uVar4 + 0xc) = *(short *)(uVar4 + 0xc) - (short)iVar7;
    }
    puVar11 = *(undefined4 **)(uVar4 + 8);
    iVar7 = *(int *)(di + 0x74);
    *puVar11 = 0;
    *(uint *)(iVar7 + uVar8 * 4) = uVar4;
    FUN_0000647c(*(int *)PTR_DAT_000053f8,uVar4,0);
    if (*(char *)(di + 0x32) == '\0') {
      uVar4 = (uint)*(ushort *)(di + 0x94);
      if (*(char *)(di + 0xde) != '\0') {
        uVar4 = 0x1ec;
      }
      local_2c[0] = 0x80000000;
      iVar9 = (uint)*(ushort *)(di + 0x94) - uVar4;
      while( true ) {
        local_5c = iVar9 + uVar4;
        if (uVar8 == (uint)*(ushort *)(di + 0x6c) - 1) {
          local_2c[0] = local_2c[0] | 0x10000000;
        }
        if ((int)uVar4 < (int)local_5c) {
          local_5c = uVar4;
        }
        FUN_00004a5c(di,*(int *)(di + 0x40),(uint)puVar11,uVar8,local_2c,local_5c);
        uVar8 = uVar8 + 1 & (uint)*(ushort *)(di + 0x6c) - 1 & 0xffff;
        puVar11 = (undefined4 *)((int)puVar11 + uVar4);
        iVar7 = extraout_r1;
        if (iVar9 < 1) break;
        *(undefined4 *)(*(int *)(di + 0x74) + uVar8 * 4) = 0x80000000;
        local_2c[0] = 0;
        iVar9 = iVar9 - uVar4;
      }
    }
    else {
      local_2c[0] = 0;
      if (uVar8 == (uint)*(ushort *)(di + 0x6c) - 1) {
        local_2c[0] = 0x10000000;
      }
      local_2c[0] = local_2c[0] | 0x80000000;
      FUN_00004a5c(di,*(int *)(di + 0x40),(uint)puVar11,uVar8,local_2c,
                   (uint)*(ushort *)(uVar4 + 0xc));
      uVar12 = *(uint *)(uVar4 + 0x40);
      iVar9 = *(int *)(uVar4 + 0x44);
      local_5c = (uint)*(ushort *)(uVar4 + 0x52);
      if (*(char *)(di + 0xde) != '\0') {
        local_5c = 0x1ec;
      }
      uVar8 = uVar8 + 1 & (uint)*(ushort *)(di + 0x6c) - 1 & 0xffff;
      iVar10 = (uint)*(ushort *)(uVar4 + 0x52) - local_5c;
      while( true ) {
        *(undefined4 *)(*(int *)(di + 0x74) + uVar8 * 4) = 0x80000000;
        local_2c[0] = 0;
        if (uVar8 == (uint)*(ushort *)(di + 0x6c) - 1) {
          local_2c[0] = 0x10000000;
        }
        func_0x00807444(di,*(undefined4 *)(di + 0x40),uVar12,iVar9,uVar8);
        uVar8 = uVar8 + 1 & (uint)*(ushort *)(di + 0x6c) - 1 & 0xffff;
        iVar7 = extraout_r1_00;
        if (iVar10 < 1) break;
        bVar15 = CARRY4(uVar12,local_5c);
        uVar12 = uVar12 + local_5c;
        iVar9 = iVar9 + (uint)bVar15;
        iVar10 = iVar10 - local_5c;
      }
    }
    iVar13 = iVar13 + 1;
  } while( true );
}



int dma_rxfill(int di)

{
  ushort uVar1;
  byte bVar2;
  uint uVar3;
  int iVar4;
  int extraout_r1;
  int extraout_r1_00;
  uint local_5c;
  char *pcVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  undefined4 *puVar10;
  uint uVar11;
  int iVar12;
  int iVar13;
  bool bVar14;
  undefined8 uVar15;
  int local_4c;
  uint local_2c [2];
  
  local_2c[0] = 0;
  bVar2 = *(byte *)(di + 0x32);
  uVar3 = (uint)bVar2;
  if ((*(uint *)(di + 8) & 0x10) == 0) {
    iVar13 = 1;
  }
  else {
    iVar13 = 0x10;
  }
  if (bVar2 == 0) {
    local_5c = 1;
  }
  else {
    local_5c = 2;
  }
  if (*(char *)(di + 0xde) != '\0') {
    pcVar5 = *(char **)(di + 0xbc);
    if ((pcVar5 == (char *)0x0) || (*pcVar5 == '\0')) {
      uVar1 = *(ushort *)(di + 0x94);
    }
    else {
      uVar1 = *(ushort *)(pcVar5 + 0xe);
    }
    local_5c = (uint)uVar1;
    if (bVar2 != 0) {
      local_5c = 0x800 - local_5c;
      uVar3 = 1;
    }
    local_5c = uVar3 + (local_5c + 0x1eb) / 0x1ec;
  }
  uVar7 = (uint)*(ushort *)(di + 0x70);
  uVar3 = (uint)*(ushort *)(di + 0x6c) / local_5c;
  if (*(uint *)(di + 0x9c) < uVar3) {
    uVar3 = *(uint *)(di + 0x9c);
  }
  iVar4 = ((1 - local_5c) -
          (uVar7 - *(ushort *)(di + 0x6e) & (uint)*(ushort *)(di + 0x6c) - 1 & 0xffff)) / local_5c +
          uVar3;
  if (*(ushort *)(di + 0x94) < 0xcd) {
    local_4c = 0;
  }
  else {
    local_4c = *(int *)(di + 0x98);
  }
  iVar12 = 0;
  iVar6 = iVar4;
  do {
    if (iVar12 == iVar4) {
      iVar13 = 0;
LAB_0000a2be:
      *(undefined2 *)(di + 0x70) = (short)uVar7;
      *(int *)(*(int *)(di + 0x38) + 4) = *(int *)(di + 0x90) + uVar7 * 0x10;
      return iVar13;
    }
    pcVar5 = *(char **)(di + 0xbc);
    if ((pcVar5 != (char *)0x0) && (*pcVar5 != '\0')) {
      uVar3 = (uint)*(ushort *)(pcVar5 + 8);
      iVar8 = iVar4;
      if (4 < *(ushort *)(pcVar5 + 8)) {
LAB_0000a0e6:
        uVar3 = FUN_00009254((int)pcVar5,iVar6,iVar8,uVar3);
        goto LAB_0000a0fe;
      }
      uVar15 = func_0x00803c2c(di + 0x20,PTR_s_wl0_dma0_0000a2d4);
      iVar6 = (int)uVar15;
      if ((int)((ulonglong)uVar15 >> 0x20) == 0) {
        pcVar5 = *(char **)(di + 0xbc);
        goto LAB_0000a0e6;
      }
LAB_0000a102:
      if (iVar12 == 0) {
        iVar13 = func_0x00807528(di);
        if (iVar13 != 0) {
          iVar13 = 1;
        }
      }
      else {
        iVar13 = 0;
      }
      *(int *)(di + 0x10) = *(int *)(di + 0x10) + 1;
      goto LAB_0000a2be;
    }
    uVar3 = FUN_0000b3a4(*(undefined4 *)(di + 0x28),
                         ((uint)*(ushort *)(di + 0x94) - 1) + iVar13 + local_4c);
LAB_0000a0fe:
    if (uVar3 == 0) goto LAB_0000a102;
    local_5c = *(uint *)(di + 8) & 0x10;
    if (local_5c != 0) {
      local_5c = iVar13 - *(int *)(uVar3 + 8) & iVar13 - 1U;
    }
    iVar6 = local_5c + local_4c;
    if (iVar6 != 0) {
      *(int *)(uVar3 + 8) = *(int *)(uVar3 + 8) + iVar6;
      *(short *)(uVar3 + 0xc) = *(short *)(uVar3 + 0xc) - (short)iVar6;
    }
    puVar10 = *(undefined4 **)(uVar3 + 8);
    iVar6 = *(int *)(di + 0x74);
    *puVar10 = 0;
    *(uint *)(iVar6 + uVar7 * 4) = uVar3;
    FUN_0000b4d8(*(int *)PTR_DAT_0000a2d8,uVar3,0);
    if (*(char *)(di + 0x32) == '\0') {
      uVar3 = (uint)*(ushort *)(di + 0x94);
      if (*(char *)(di + 0xde) != '\0') {
        uVar3 = 0x1ec;
      }
      local_2c[0] = 0x80000000;
      iVar8 = (uint)*(ushort *)(di + 0x94) - uVar3;
      while( true ) {
        local_5c = iVar8 + uVar3;
        if (uVar7 == (uint)*(ushort *)(di + 0x6c) - 1) {
          local_2c[0] = local_2c[0] | 0x10000000;
        }
        if ((int)uVar3 < (int)local_5c) {
          local_5c = uVar3;
        }
        FUN_00009952(di,*(int *)(di + 0x40),(uint)puVar10,uVar7,local_2c,local_5c);
        uVar7 = uVar7 + 1 & (uint)*(ushort *)(di + 0x6c) - 1 & 0xffff;
        puVar10 = (undefined4 *)((int)puVar10 + uVar3);
        iVar6 = extraout_r1;
        if (iVar8 < 1) break;
        *(undefined4 *)(*(int *)(di + 0x74) + uVar7 * 4) = 0x80000000;
        local_2c[0] = 0;
        iVar8 = iVar8 - uVar3;
      }
    }
    else {
      local_2c[0] = 0;
      if (uVar7 == (uint)*(ushort *)(di + 0x6c) - 1) {
        local_2c[0] = 0x10000000;
      }
      local_2c[0] = local_2c[0] | 0x80000000;
      FUN_00009952(di,*(int *)(di + 0x40),(uint)puVar10,uVar7,local_2c,
                   (uint)*(ushort *)(uVar3 + 0xc));
      uVar11 = *(uint *)(uVar3 + 0x40);
      iVar8 = *(int *)(uVar3 + 0x44);
      local_5c = (uint)*(ushort *)(uVar3 + 0x52);
      if (*(char *)(di + 0xde) != '\0') {
        local_5c = 0x1ec;
      }
      uVar7 = uVar7 + 1 & (uint)*(ushort *)(di + 0x6c) - 1 & 0xffff;
      iVar9 = (uint)*(ushort *)(uVar3 + 0x52) - local_5c;
      while( true ) {
        *(undefined4 *)(*(int *)(di + 0x74) + uVar7 * 4) = 0x80000000;
        local_2c[0] = 0;
        if (uVar7 == (uint)*(ushort *)(di + 0x6c) - 1) {
          local_2c[0] = 0x10000000;
        }
        func_0x00807444(di,*(undefined4 *)(di + 0x40),uVar11,iVar8,uVar7);
        uVar7 = uVar7 + 1 & (uint)*(ushort *)(di + 0x6c) - 1 & 0xffff;
        iVar6 = extraout_r1_00;
        if (iVar9 < 1) break;
        bVar14 = CARRY4(uVar11,local_5c);
        uVar11 = uVar11 + local_5c;
        iVar8 = iVar8 + (uint)bVar14;
        iVar9 = iVar9 - local_5c;
      }
    }
    iVar12 = iVar12 + 1;
  } while( true );
}


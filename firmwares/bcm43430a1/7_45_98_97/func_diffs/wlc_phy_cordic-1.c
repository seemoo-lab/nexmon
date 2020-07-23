
void wlc_phy_cordic(int theta,int *val)

{
  undefined *puVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  
  val[1] = 0x9b75;
  *val = 0;
  uVar4 = (uint)(-1 < theta);
  if (-1 >= theta) {
    uVar4 = 0xffffffff;
  }
  iVar5 = DAT_0002441c * uVar4 + (uVar4 * 0xb40000 + theta) % 0x1680000;
  if (iVar5 < 0) {
    iVar6 = -((-iVar5 >> 0xf) + 1 >> 1);
    if (iVar6 + 0x5a < 0 == SCARRY4(iVar6,0x5a)) goto LAB_000243c2;
    iVar5 = iVar5 + 0xb40000;
  }
  else {
    if ((iVar5 >> 0xf) + 1 < 0xb6) {
LAB_000243c2:
      iVar6 = 1;
      goto LAB_000243c4;
    }
    iVar5 = iVar5 + DAT_0002441c;
  }
  iVar6 = -1;
LAB_000243c4:
  iVar2 = 0;
  iVar3 = 0;
  iVar7 = 0;
  do {
    puVar1 = PTR_DAT_00024420;
    iVar8 = val[1];
    iVar10 = *val;
    if (iVar7 < iVar5) {
      val[1] = iVar8 - (iVar10 >> iVar3);
      iVar9 = *(int *)(puVar1 + iVar2);
      *val = (iVar8 >> iVar3) + iVar10;
    }
    else {
      val[1] = iVar8 + (iVar10 >> iVar3);
      iVar9 = *(int *)(puVar1 + iVar2);
      *val = iVar10 - (iVar8 >> iVar3);
      iVar9 = -iVar9;
    }
    iVar7 = iVar7 + iVar9;
    iVar3 = iVar3 + 1;
    iVar2 = iVar2 + 4;
  } while (iVar3 != 0x12);
  val[1] = iVar6 * val[1];
  *val = *val * iVar6;
  return;
}


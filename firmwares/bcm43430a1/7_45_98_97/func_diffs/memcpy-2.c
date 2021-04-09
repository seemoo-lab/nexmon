
int memcpy(undefined4 *dst,undefined4 *src,uint len)

{
  uint uVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  
  puVar3 = dst;
  if ((3 < len) && (uVar1 = ((uint)dst ^ (uint)src) & 3, uVar1 == 0)) {
    len = len - (-(int)src & 3U);
    while (uVar1 != (-(int)src & 3U)) {
      *(undefined *)((int)dst + uVar1) = *(undefined *)((int)src + uVar1);
      uVar1 = uVar1 + 1;
    }
    puVar3 = (undefined4 *)((int)dst + uVar1);
    src = (undefined4 *)((int)src + uVar1);
    if (0x1f < len) {
      puVar2 = (undefined4 *)((int)src + (len & 0xffffffe0));
      do {
        uVar4 = *src;
        uVar5 = src[1];
        uVar6 = src[2];
        uVar7 = src[3];
        uVar8 = src[4];
        uVar9 = src[5];
        uVar10 = src[6];
        uVar11 = src[7];
        src = src + 8;
        *puVar3 = uVar4;
        puVar3[1] = uVar5;
        puVar3[2] = uVar6;
        puVar3[3] = uVar7;
        puVar3[4] = uVar8;
        puVar3[5] = uVar9;
        puVar3[6] = uVar10;
        puVar3[7] = uVar11;
        puVar3 = puVar3 + 8;
      } while (src < puVar2);
      len = len & 0x1f;
    }
    switch(len >> 2) {
    case 1:
      *puVar3 = *src;
      src = src + 1;
      puVar3 = puVar3 + 1;
      break;
    case 2:
      uVar4 = *src;
      uVar5 = src[1];
      src = src + 2;
      *puVar3 = uVar4;
      puVar3[1] = uVar5;
      puVar3 = puVar3 + 2;
      break;
    case 3:
      uVar4 = *src;
      uVar5 = src[1];
      uVar6 = src[2];
      src = src + 3;
      *puVar3 = uVar4;
      puVar3[1] = uVar5;
      puVar3[2] = uVar6;
      puVar3 = puVar3 + 3;
      break;
    case 4:
      uVar4 = *src;
      uVar5 = src[1];
      uVar6 = src[2];
      uVar7 = src[3];
      src = src + 4;
      *puVar3 = uVar4;
      puVar3[1] = uVar5;
      puVar3[2] = uVar6;
      puVar3[3] = uVar7;
      puVar3 = puVar3 + 4;
      break;
    case 5:
      uVar4 = *src;
      uVar5 = src[1];
      uVar6 = src[2];
      uVar7 = src[3];
      uVar8 = src[4];
      src = src + 5;
      *puVar3 = uVar4;
      puVar3[1] = uVar5;
      puVar3[2] = uVar6;
      puVar3[3] = uVar7;
      puVar3[4] = uVar8;
      puVar3 = puVar3 + 5;
      break;
    case 6:
      uVar4 = *src;
      uVar5 = src[1];
      uVar6 = src[2];
      uVar7 = src[3];
      uVar8 = src[4];
      uVar9 = src[5];
      src = src + 6;
      *puVar3 = uVar4;
      puVar3[1] = uVar5;
      puVar3[2] = uVar6;
      puVar3[3] = uVar7;
      puVar3[4] = uVar8;
      puVar3[5] = uVar9;
      puVar3 = puVar3 + 6;
      break;
    case 7:
      uVar4 = *src;
      uVar5 = src[1];
      uVar6 = src[2];
      uVar7 = src[3];
      uVar8 = src[4];
      uVar9 = src[5];
      uVar10 = src[6];
      src = src + 7;
      *puVar3 = uVar4;
      puVar3[1] = uVar5;
      puVar3[2] = uVar6;
      puVar3[3] = uVar7;
      puVar3[4] = uVar8;
      puVar3[5] = uVar9;
      puVar3[6] = uVar10;
      puVar3 = puVar3 + 7;
    }
    len = len & 3;
  }
  uVar1 = 0;
  while (uVar1 != len) {
    *(undefined *)((int)puVar3 + uVar1) = *(undefined *)((int)src + uVar1);
    uVar1 = uVar1 + 1;
  }
  return (int)dst;
}


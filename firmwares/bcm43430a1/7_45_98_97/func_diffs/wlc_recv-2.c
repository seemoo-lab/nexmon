
ushort * wlc_recv(uint **wlc,ushort *pp)

{
  ushort uVar1;
  ushort uVar2;
  int iVar3;
  ushort *p;
  int **ppiVar4;
  ushort **osh;
  int iVar5;
  int iVar6;
  uint *puVar7;
  uint uVar8;
  uint uVar9;
  int *piVar10;
  uint **rxh;
  int *piVar11;
  int *piVar12;
  
  rxh = *(uint ***)(pp + 4);
  osh = (ushort **)wlc[1];
  wlc[0x198][0xc] = 0;
  puVar7 = wlc[0x172];
  iVar6 = (int)rxh + (int)puVar7;
  *(int *)(pp + 4) = iVar6;
  uVar8 = (uint)pp[6] - (int)puVar7 & 0xffff;
  uVar2 = (ushort)uVar8;
  pp[6] = uVar2;
  if ((int)((uint)*(ushort *)(rxh + 4) << 0x1d) < 0) {
    if (1 < uVar8) {
      *(int *)(pp + 4) = iVar6 + 2;
      pp[6] = uVar2 - 2;
      goto LAB_0001933a;
    }
    puVar7 = *wlc;
LAB_000193d2:
    uVar9 = puVar7[0x22];
    *(int *)(uVar9 + 100) = *(int *)(uVar9 + 100) + 1;
LAB_000194a2:
    pkt_buf_free_skb(osh,pp,0,uVar9);
    return (ushort *)osh;
  }
LAB_0001933a:
  uVar8 = (uint)pp[7] & 0x2000;
  if ((pp[7] & 0x2000) != 0) {
    uVar8 = (uint)pp[0x28];
  }
  iVar6 = *(int *)(pp + 4);
  uVar2 = pp[6];
  if (((*(ushort *)(wlc[8] + 2) < 0xd) && (0xa80000 << (int)*(short *)(wlc[8] + 2) < 0)) &&
     ((int)((uint)*(ushort *)(rxh + 1) << 0x17) < 0)) {
    *(ushort *)(rxh + 1) = *(ushort *)(rxh + 1) & 0xfeff;
  }
  uVar9 = (uint)*(ushort *)(rxh + 1);
  if ((*(ushort *)(rxh + 1) & 0x310) != 0) goto LAB_000194a2;
  if ((wlc[0x82] != (uint *)0x0) && (iVar3 = func_0x0085e81c(wlc[0x1b7]), iVar3 != 0)) {
    if ((*(ushort *)((int)rxh + 0x12) & 1) == 0) {
      p = (ushort *)FUN_0000fff4((int)wlc,(ushort *)rxh,(int)pp);
    }
    else {
      p = (ushort *)func_0x0081f634();
    }
    if ((wlc[0x82] != (uint *)0x0) && (*(char *)((int)*wlc + 0x3f) == '\0')) {
      return p;
    }
  }
  uVar9 = (uint)*(ushort *)(rxh + 4);
  if ((int)(uVar9 << 0x1f) < 0) goto LAB_000194a2;
  if (uVar8 + uVar2 < 8) {
    puVar7 = *wlc;
    goto LAB_000193d2;
  }
  uVar2 = *(ushort *)(iVar6 + 6);
  if ((uVar2 & 0x800) != 0) {
    *(int *)((*wlc)[0x22] + 0x194) = *(int *)((*wlc)[0x22] + 0x194) + 1;
  }
  uVar1 = *(ushort *)((int)rxh + 0x12);
  if ((uVar1 & 1) == 0) {
    iVar3 = (int)((uint)uVar2 & 0xc) >> 2;
    if ((iVar3 == 2) || (iVar3 == 0)) {
      iVar3 = FUN_0000948c(iVar6 + 0x10);
      if ((iVar3 != 0) || ((int)((uint)*(byte *)(iVar6 + 0x10) << 0x1f) < 0)) {
        uVar9 = (*wlc)[0x22];
        *(int *)(uVar9 + 0x74) = *(int *)(uVar9 + 0x74) + 1;
        goto LAB_000194a2;
      }
      *(int *)((*wlc)[0x22] + 0x1cc) = *(int *)((*wlc)[0x22] + 0x1cc) + 1;
    }
    FUN_0001f1d4((int)wlc[0x4c]);
  }
  uVar9 = *(uint *)(pp + 0xc) & 0x80;
  if (uVar9 != 0) {
LAB_00019470:
    wlc_recvdata((int *)wlc,osh,(int)rxh,pp);
    return (ushort *)wlc;
  }
  if ((uVar1 & 1) == 0) {
    uVar8 = (int)((uint)uVar2 & 0xc) >> 2;
    if (uVar8 == 2) goto LAB_00019470;
    if (uVar8 < 2) {
      p = (ushort *)FUN_00016c84(wlc,osh,rxh,pp);
      return p;
    }
    uVar9 = (*wlc)[0x22];
    *(int *)(uVar9 + 0x70) = *(int *)(uVar9 + 0x70) + 1;
    goto LAB_000194a2;
  }
  if (*(char *)((int)wlc + 0x212) == '\0') goto LAB_000194a2;
  ppiVar4 = (int **)wlc[0x4c];
  uVar2 = pp[7];
  osh = (ushort **)ppiVar4[1][5];
  if (((uVar2 & 0x2000) == 0) && ((int)(uint)pp[6] < *(int *)(*(int *)(**ppiVar4 + 0x1c) + 0x68))) {
    iVar6 = (*(uint *)(pp + 4) & 0x1fffff) - (*(uint *)(pp + 2) & 0x1fffff);
    p = (ushort *)pkt_buf_get_skb(osh,(uint)pp[6] + iVar6);
    if (p != (ushort *)0x0) {
      iVar5 = *(int *)(p + 4);
      *(int *)(p + 4) = iVar5 + iVar6;
      p[6] = p[6] - (short)iVar6;
      iVar3 = -iVar6;
      memcpy((undefined4 *)(iVar5 + iVar6 + iVar3),(undefined4 *)(*(int *)(pp + 4) + iVar3),
             (uint)pp[6] + iVar6);
      pkt_buf_free_skb(osh,pp,(uint)uVar2 & 0x2000,iVar3);
      pp = p;
    }
  }
  uVar8 = (uint)*(ushort *)((int)rxh + 2);
  piVar10 = ppiVar4[0x1b];
  piVar11 = ppiVar4[0x18];
  iVar6 = (int)((uint)*(ushort *)((int)rxh + 0x12) & 6) >> 1;
  piVar10[0xb] = piVar10[0xb] + 1;
  piVar12 = piVar11 + uVar8 * 3;
  iVar3 = piVar11[uVar8 * 3];
  if (iVar6 == 2) {
    if (iVar3 == 1) {
      piVar11[uVar8 * 3] = 3;
      if (pp[6] < 0x12) {
LAB_0001f2da:
        piVar10 = (int *)ppiVar4[1][0x22];
        piVar10[0x19] = piVar10[0x19] + 1;
        goto LAB_0001f360;
      }
      *(ushort *)(piVar12[2] + 0x14) = *pp;
LAB_0001f2f0:
      *(ushort **)(piVar12 + 2) = pp;
      goto LAB_0001f316;
    }
LAB_0001f2c8:
    piVar10[0xe] = piVar10[0xe] + 1;
  }
  else {
    if (iVar6 == 3) {
      if (iVar3 != 0) {
        piVar10[0xe] = piVar10[0xe] + 1;
        FUN_0001f0dc((int)ppiVar4,uVar8);
      }
      piVar10 = (int *)0x3;
    }
    else {
      if (iVar6 != 1) {
        if (iVar3 == 1) {
          if (pp[6] < 0xf) goto LAB_0001f2da;
          *(ushort *)(piVar12[2] + 0x14) = *pp;
          goto LAB_0001f2f0;
        }
        goto LAB_0001f2c8;
      }
      if (iVar3 != 0) {
        piVar10[0xe] = piVar10[0xe] + 1;
        FUN_0001f0dc((int)ppiVar4,uVar8);
      }
      piVar10 = (int *)(Reset + 1);
    }
    *(int **)(piVar11 + uVar8 * 3) = piVar10;
    iVar6 = FUN_0001f15c((int)ppiVar4,uVar8,(int)pp);
    if (iVar6 != 0) {
LAB_0001f316:
      if (piVar11[uVar8 * 3] != 3) {
        return (ushort *)0x0;
      }
      p = (ushort *)piVar12[1];
      piVar12[2] = 0;
      uVar9 = *(uint *)(p + 0xc);
      piVar12[1] = 0;
      piVar11[uVar8 * 3] = 0;
      *(uint *)(p + 0xc) = uVar9 | 0x80;
      piVar10 = ppiVar4[1];
      iVar6 = piVar10[0x22];
      *(int *)(iVar6 + 0x1cc) = *(int *)(iVar6 + 0x1cc) + 1;
      ppiVar4[0x1b][0xc] = ppiVar4[0x1b][0xc] + 1;
      wlc_recvdata(*ppiVar4,(ushort **)piVar10[5],(int)rxh,p);
      return (ushort *)0x0;
    }
  }
LAB_0001f360:
  FUN_0001f0dc((int)ppiVar4,uVar8);
  pkt_buf_free_skb(osh,pp,0,piVar10);
  return (ushort *)0x0;
}


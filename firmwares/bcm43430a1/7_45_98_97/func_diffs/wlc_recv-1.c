
/* WARNING: Control flow encountered bad instruction data */

ushort * wlc_recv(uint *wlc,ushort *pp)

{
  ushort uVar1;
  ushort uVar2;
  int iVar3;
  uint **ppuVar4;
  ushort **osh;
  ushort **ppuVar5;
  ushort *p;
  int iVar6;
  uint uVar7;
  uint uVar8;
  uint *puVar9;
  int rxh;
  uint *puVar10;
  uint *puVar11;
  
  rxh = *(int *)(pp + 4);
  osh = (ushort **)wlc[1];
  *(undefined4 *)(wlc[0x198] + 0x30) = 0;
  uVar7 = wlc[0x172];
  iVar6 = rxh + uVar7;
  *(int *)(pp + 4) = iVar6;
  uVar7 = pp[6] - uVar7 & 0xffff;
  uVar2 = (ushort)uVar7;
  pp[6] = uVar2;
  if ((int)((uint)*(ushort *)(rxh + 0x10) << 0x1d) < 0) {
    if (1 < uVar7) {
      *(int *)(pp + 4) = iVar6 + 2;
      pp[6] = uVar2 - 2;
      goto LAB_00012c5a;
    }
    uVar7 = *wlc;
LAB_00012ce2:
    uVar8 = *(uint *)(uVar7 + 0x88);
    *(int *)(uVar8 + 100) = *(int *)(uVar8 + 100) + 1;
LAB_00012db2:
    pkt_buf_free_skb(osh,pp,0,uVar8);
    return (ushort *)osh;
  }
LAB_00012c5a:
  uVar7 = (uint)pp[7] & 0x2000;
  if ((pp[7] & 0x2000) != 0) {
    uVar7 = (uint)pp[0x28];
  }
  iVar6 = *(int *)(pp + 4);
  uVar2 = pp[6];
  if (((*(ushort *)(wlc[8] + 8) < 0xd) && (0xa80000 << (int)*(short *)(wlc[8] + 8) < 0)) &&
     ((int)((uint)*(ushort *)(rxh + 4) << 0x17) < 0)) {
    *(ushort *)(rxh + 4) = *(ushort *)(rxh + 4) & 0xfeff;
  }
  uVar8 = (uint)*(ushort *)(rxh + 4);
  if ((*(ushort *)(rxh + 4) & 0x310) != 0) goto LAB_00012db2;
  if ((wlc[0x82] != 0) && (iVar3 = func_0x0085e81c(wlc[0x1b7]), iVar3 != 0)) {
    if ((*(ushort *)(rxh + 0x12) & 1) == 0) {
      func_0x0081f410(wlc,rxh,pp);
    }
    else {
      func_0x0081f634();
    }
  }
  uVar8 = (uint)*(ushort *)(rxh + 0x10);
  if ((int)(uVar8 << 0x1f) < 0) goto LAB_00012db2;
  if (uVar7 + uVar2 < 8) {
    uVar7 = *wlc;
    goto LAB_00012ce2;
  }
  uVar2 = *(ushort *)(iVar6 + 6);
  if ((uVar2 & 0x800) != 0) {
    *(int *)(*(int *)(*wlc + 0x88) + 0x194) = *(int *)(*(int *)(*wlc + 0x88) + 0x194) + 1;
  }
  uVar1 = *(ushort *)(rxh + 0x12);
  if ((uVar1 & 1) == 0) {
    iVar3 = (int)((uint)uVar2 & 0xc) >> 2;
    if ((iVar3 == 2) || (iVar3 == 0)) {
      iVar3 = FUN_000047d4(iVar6 + 0x10);
      if ((iVar3 != 0) || ((int)((uint)*(byte *)(iVar6 + 0x10) << 0x1f) < 0)) {
        uVar8 = *(uint *)(*wlc + 0x88);
        *(int *)(uVar8 + 0x74) = *(int *)(uVar8 + 0x74) + 1;
        goto LAB_00012db2;
      }
      *(int *)(*(int *)(*wlc + 0x88) + 0x1cc) = *(int *)(*(int *)(*wlc + 0x88) + 0x1cc) + 1;
    }
    FUN_00017c4a(wlc[0x4c]);
  }
  uVar8 = *(uint *)(pp + 0xc) & 0x80;
  if (uVar8 != 0) {
LAB_00012d80:
    wlc_recvdata(wlc,osh,rxh,pp);
    return (ushort *)wlc;
  }
  if ((uVar1 & 1) == 0) {
    uVar7 = (int)((uint)uVar2 & 0xc) >> 2;
    if (uVar7 == 2) goto LAB_00012d80;
    if (uVar7 < 2) {
                    /* WARNING: Bad instruction - Truncating control flow here */
      halt_baddata();
    }
    uVar8 = *(uint *)(*wlc + 0x88);
    *(int *)(uVar8 + 0x70) = *(int *)(uVar8 + 0x70) + 1;
    goto LAB_00012db2;
  }
  if (*(char *)((int)wlc + 0x212) == '\0') goto LAB_00012db2;
  ppuVar4 = (uint **)wlc[0x4c];
  uVar2 = pp[7];
  osh = (ushort **)ppuVar4[1][5];
  if (((uVar2 & 0x2000) == 0) && ((int)(uint)pp[6] < *(int *)(*(int *)(**ppuVar4 + 0x1c) + 0x68))) {
    iVar6 = (*(uint *)(pp + 4) & 0x1fffff) - (*(uint *)(pp + 2) & 0x1fffff);
    ppuVar5 = osh;
    pkt_buf_get_skb(osh,(uint)pp[6] + iVar6);
    if (ppuVar5 != (ushort **)0x0) {
      p = ppuVar5[2];
      ppuVar5[2] = (ushort *)((int)p + iVar6);
      *(ushort *)(ppuVar5 + 3) = *(ushort *)(ppuVar5 + 3) - (short)iVar6;
      iVar3 = -iVar6;
      memcpy((undefined4 *)((int)(ushort *)((int)p + iVar6) + iVar3),
             (undefined4 *)(*(int *)(pp + 4) + iVar3),(uint)pp[6] + iVar6);
      pkt_buf_free_skb(osh,pp,(uint)uVar2 & 0x2000,iVar3);
      pp = (ushort *)ppuVar5;
    }
  }
  uVar8 = (uint)*(ushort *)(rxh + 2);
  puVar9 = ppuVar4[0x1b];
  puVar10 = ppuVar4[0x18];
  iVar6 = (int)((uint)*(ushort *)(rxh + 0x12) & 6) >> 1;
  puVar9[0xb] = puVar9[0xb] + 1;
  puVar11 = puVar10 + uVar8 * 3;
  uVar7 = puVar10[uVar8 * 3];
  if (iVar6 == 2) {
    if (uVar7 == 1) {
      puVar10[uVar8 * 3] = 3;
      if (*(ushort *)((ushort **)pp + 3) < 0x12) {
LAB_00017d50:
        puVar9 = (uint *)ppuVar4[1][0x22];
        puVar9[0x19] = puVar9[0x19] + 1;
        goto LAB_00017dd6;
      }
      *(ushort *)(puVar11[2] + 0x14) = *pp;
LAB_00017d66:
      *(ushort **)(puVar11 + 2) = pp;
      goto LAB_00017d8c;
    }
LAB_00017d3e:
    puVar9[0xe] = puVar9[0xe] + 1;
  }
  else {
    if (iVar6 == 3) {
      if (uVar7 != 0) {
        puVar9[0xe] = puVar9[0xe] + 1;
        FUN_00017b52((int)ppuVar4,uVar8);
      }
      puVar9 = (uint *)0x3;
    }
    else {
      if (iVar6 != 1) {
        if (uVar7 == 1) {
          if (*(ushort *)((ushort **)pp + 3) < 0xf) goto LAB_00017d50;
          *(ushort *)(puVar11[2] + 0x14) = *pp;
          goto LAB_00017d66;
        }
        goto LAB_00017d3e;
      }
      if (uVar7 != 0) {
        puVar9[0xe] = puVar9[0xe] + 1;
        FUN_00017b52((int)ppuVar4,uVar8);
      }
      puVar9 = (uint *)(Reset + 1);
    }
    *(uint **)(puVar10 + uVar8 * 3) = puVar9;
    iVar6 = FUN_00017bd2((int)ppuVar4,uVar8,(int)pp);
    if (iVar6 != 0) {
LAB_00017d8c:
      if (puVar10[uVar8 * 3] != 3) {
        return (ushort *)0x0;
      }
      p = (ushort *)puVar11[1];
      puVar11[2] = 0;
      uVar7 = *(uint *)(p + 0xc);
      puVar11[1] = 0;
      puVar10[uVar8 * 3] = 0;
      *(uint *)(p + 0xc) = uVar7 | 0x80;
      puVar9 = ppuVar4[1];
      uVar7 = puVar9[0x22];
      *(int *)(uVar7 + 0x1cc) = *(int *)(uVar7 + 0x1cc) + 1;
      ppuVar4[0x1b][0xc] = ppuVar4[0x1b][0xc] + 1;
      wlc_recvdata(*ppuVar4,(ushort **)puVar9[5],rxh,p);
      return (ushort *)0x0;
    }
  }
LAB_00017dd6:
  FUN_00017b52((int)ppuVar4,uVar8);
  pkt_buf_free_skb(osh,pp,0,puVar9);
  return (ushort *)0x0;
}


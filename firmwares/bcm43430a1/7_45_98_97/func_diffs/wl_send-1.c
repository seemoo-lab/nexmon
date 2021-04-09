
int wl_send(int src,int dev,int pp)

{
  ushort *puVar1;
  int wlc;
  ushort *p;
  undefined *puVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  ushort *puVar7;
  ushort *puVar8;
  ushort *puVar9;
  int iVar10;
  undefined4 *puVar11;
  undefined4 *wlcif;
  int iVar12;
  
  iVar6 = *(int *)(dev + 0x18);
  if (dev == *(int *)(iVar6 + 0x10)) {
    puVar11 = (undefined4 *)0x0;
    wlcif = puVar11;
  }
  else {
    puVar11 = *(undefined4 **)(dev + 0x2c);
    wlcif = puVar11;
    if (puVar11 != (undefined4 *)0x0) {
      wlcif = (undefined4 *)*puVar11;
    }
  }
  wlc = *(int *)(iVar6 + 8);
  wlc_bsscfg_find_by_wlcif(wlc,(int)wlcif);
  puVar7 = (ushort *)0x0;
  iVar12 = 0;
  iVar10 = 0;
  puVar9 = (ushort *)0;
  do {
    while( true ) {
      do {
        if (pp == 0) {
          if (((*(char *)(*(int *)(iVar6 + 4) + 0x3e) != '\0') && (puVar9 != (ushort *)0x0)) &&
             (puVar2 = FUN_00010e18(*(int **)(iVar6 + 8),puVar9,(int)wlcif),
             puVar2 != (undefined *)0x0)) {
            iVar10 = 1;
          }
          return iVar10;
        }
        uVar3 = (uint)*(ushort *)(pp + 0x16);
        iVar4 = *(int *)(*DAT_0000810c + uVar3 * 4);
        *(undefined2 *)(pp + 0x16) = 0;
        iVar5 = *(int *)(iVar6 + 4);
        p = (ushort *)FUN_000063dc(*(int **)(iVar5 + 0x14),pp);
        iVar5 = FUN_00007ed4(iVar6,p,uVar3,iVar5);
        pp = iVar4;
      } while (iVar5 != 0);
      if (((*(char *)(*(int *)(iVar6 + 4) + 0x2e) == '\0') ||
          (iVar5 = FUN_00007fc8(iVar6,puVar11), iVar5 == 0)) ||
         (iVar5 = func_0x00812f38(iVar5,p), iVar5 != 2)) break;
      pkt_buf_free_skb(*(ushort ***)(*(int *)(iVar6 + 4) + 0x14),p,1,*(int *)(iVar6 + 4));
    }
    func_0x008195cc(iVar6,p);
    if (*(char *)(*(int *)(iVar6 + 4) + 0x3e) == '\0') goto LAB_000080ca;
    iVar12 = func_0x00819c84(iVar6,wlc,p,puVar9);
    puVar1 = puVar7;
    puVar8 = puVar9;
    if ((iVar12 != 0) && (p[7] = p[7] | 0x1000, puVar1 = p, puVar8 = p, puVar7 != (ushort *)0x0)) {
      puVar7[0xb] = *p;
      puVar8 = puVar9;
    }
    puVar7 = puVar1;
    puVar9 = puVar8;
    if (puVar8 == (ushort *)0x0) goto LAB_000080ca;
    if ((iVar4 == 0) || (iVar12 == 0)) {
      puVar7 = (ushort *)FUN_00010e18(*(int **)(iVar6 + 8),puVar8,(int)wlcif);
      puVar9 = puVar7;
      if (puVar7 != (ushort *)0x0) {
        puVar7 = (ushort *)0x0;
        iVar10 = 1;
        puVar9 = puVar7;
      }
LAB_000080ca:
      if ((iVar12 == 0) &&
         (puVar2 = FUN_00010e18(*(int **)(iVar6 + 8),p,(int)wlcif), puVar2 != (undefined *)0x0)) {
        iVar10 = 1;
      }
    }
  } while( true );
}


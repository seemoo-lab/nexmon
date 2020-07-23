
int wl_send(int src,int dev,int pp)

{
  byte bVar1;
  ushort *puVar2;
  int iVar3;
  ushort *p;
  int iVar4;
  int iVar5;
  int iVar6;
  int wlc;
  ushort *puVar7;
  ushort *puVar8;
  ushort *puVar9;
  undefined4 *puVar10;
  undefined4 *wlcif;
  uint uVar11;
  uint uStack52;
  
  iVar6 = *(int *)(dev + 0x18);
  if (dev == *(int *)(iVar6 + 0x10)) {
    puVar10 = (undefined4 *)0x0;
    wlcif = puVar10;
  }
  else {
    puVar10 = *(undefined4 **)(dev + 0x2c);
    wlcif = puVar10;
    if (puVar10 != (undefined4 *)0x0) {
      wlcif = (undefined4 *)*puVar10;
    }
  }
  wlc = *(int *)(iVar6 + 8);
  iVar3 = wlc;
  wlc_bsscfg_find_by_wlcif(wlc,(int)wlcif);
  if (*(char *)(*(int *)(iVar6 + 4) + 0x3e) == '\0') {
    uStack52 = 1;
  }
  else {
    if ((*(char *)(wlc + 0x221) == '\0') ||
       (uStack52 = (uint)*(byte *)(iVar3 + 6), *(byte *)(iVar3 + 6) != 0)) {
      if (*(char *)(wlc + 0x222) == '\0') {
        bVar1 = *(byte *)(*(int *)(iVar6 + 4) + 0x45);
        uStack52 = 1 - (uint)bVar1;
        if (1 < bVar1) {
          uStack52 = 0;
        }
      }
      else {
        uStack52 = 0;
      }
    }
  }
  puVar7 = (ushort *)0x0;
  uVar11 = 0;
  wlc = 0;
  puVar9 = (ushort *)0;
  do {
    while( true ) {
      if (pp == 0) {
        if (((*(char *)(*(int *)(iVar6 + 4) + 0x3e) != '\0') && (puVar9 != (ushort *)0x0)) &&
           (iVar6 = FUN_00017610(*(uint **)(iVar6 + 8),puVar9,(int)wlcif), iVar6 != 0)) {
          wlc = 1;
        }
        return wlc;
      }
      iVar5 = *(int *)(*DAT_0000c7c0 + (uint)*(ushort *)(pp + 0x16) * 4);
      *(undefined2 *)(pp + 0x16) = 0;
      p = (ushort *)FUN_0000b438(*(int **)(*(int *)(iVar6 + 4) + 0x14),pp);
      FUN_0000c568(iVar6,(int)p);
      pp = iVar5;
      if (((*(char *)(*(int *)(iVar6 + 4) + 0x2e) == '\0') ||
          (iVar4 = FUN_0000c648(iVar6,puVar10), iVar4 == 0)) ||
         (iVar4 = func_0x00812f38(iVar4,p), iVar4 != 2)) break;
      pkt_buf_free_skb(*(ushort ***)(*(int *)(iVar6 + 4) + 0x14),p,1,*(int *)(iVar6 + 4));
    }
    if (*(char *)(*(int *)(iVar6 + 4) + 0x3e) == '\0') goto LAB_0000c77e;
    puVar2 = puVar7;
    puVar8 = puVar9;
    uVar11 = uStack52;
    if (((uStack52 != 0) && (uVar11 = FUN_0000c578(iVar6,iVar3,(int)p,(int)puVar9), uVar11 != 0)) &&
       (p[7] = p[7] | 0x1000, puVar2 = p, puVar8 = p, puVar7 != (ushort *)0x0)) {
      puVar7[0xb] = *p;
      puVar8 = puVar9;
    }
    puVar7 = puVar2;
    puVar9 = puVar8;
    if (puVar8 == (ushort *)0x0) goto LAB_0000c77e;
    if ((iVar5 == 0) || (uVar11 == 0)) {
      puVar7 = (ushort *)FUN_00017610(*(uint **)(iVar6 + 8),puVar8,(int)wlcif);
      puVar9 = puVar7;
      if (puVar7 != (ushort *)0x0) {
        puVar7 = (ushort *)0x0;
        wlc = 1;
        puVar9 = puVar7;
      }
LAB_0000c77e:
      if ((uVar11 == 0) && (iVar5 = FUN_00017610(*(uint **)(iVar6 + 8),p,(int)wlcif), iVar5 != 0)) {
        wlc = 1;
      }
    }
  } while( true );
}


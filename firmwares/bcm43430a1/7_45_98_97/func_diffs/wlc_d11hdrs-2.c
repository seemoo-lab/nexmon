
/* WARNING: Could not reconcile some variable overlaps */

uint wlc_d11hdrs(int *wlc,int p,int scb,uint short_preamble,uint frag,uint nfrag,uint queue,
                int next_frag_len,uint key,uint rspec_override)

{
  char cVar1;
  char cVar2;
  byte bVar3;
  char cVar4;
  undefined2 uVar5;
  short sVar6;
  ushort uVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  code *pcVar12;
  undefined *puVar13;
  undefined4 *dst;
  int iVar14;
  code *pcVar15;
  char cVar16;
  undefined4 *src;
  uint uVar17;
  uint uVar18;
  uint uVar19;
  uint uVar20;
  byte bVar21;
  ushort uVar22;
  code *pcVar23;
  uint uVar24;
  uint uVar25;
  uint uVar26;
  int iVar27;
  ushort uVar28;
  uint uVar29;
  code *pcVar30;
  undefined4 uVar31;
  bool bVar32;
  bool bVar33;
  code *pcStack176;
  uint uStack172;
  code *pcStack168;
  code *pcStack160;
  code *pcStack156;
  code *pcStack152;
  code *pcStack148;
  int iStack144;
  code *pcStack136;
  code *pcStack132;
  uint uStack128;
  ushort uStack120;
  code *pcStack108;
  code *pcStack92;
  uint uStack76;
  uint uStack72;
  undefined uStack66;
  undefined4 auStack64 [2];
  undefined4 auStack56 [2];
  ushort uStack48;
  ushort uStack46;
  ushort uStack44;
  undefined2 auStack42 [3];
  
  pcVar23 = (code *)(short_preamble & 1);
  iVar9 = *(int *)(scb + 0x10);
  iVar14 = *(int *)(iVar9 + 0x344);
  cVar1 = **(char **)(iVar9 + *(int *)wlc[0x11b]);
  src = *(undefined4 **)(p + 8);
  cVar2 = **(char **)(iVar9 + *(int *)wlc[0x11c]);
  uVar7 = *(ushort *)src;
  uVar24 = (uint)uVar7;
  uVar25 = (uVar24 & 0xc) >> 2;
  if (uVar25 == 2) {
    bVar21 = (byte)uVar7 >> 7;
  }
  else {
    bVar21 = 0;
  }
  bVar32 = (uVar24 & 0x300) == 0x300;
  iStack144 = FUN_00009338(wlc[1],p);
  iStack144 = iStack144 + 4;
  if ((key != 0) &&
     ((*(char *)(key + 8) != '\v' || ((*(char *)(*wlc + 0xb1) != '\0' && (*(byte *)(key + 6) < 8))))
     )) {
    iStack144 = iStack144 + *(char *)(key + 0xf);
  }
  if (wlc[0x6d] < 0) {
    pcStack152 = (code *)key;
    if (key != 0) {
      if (*(char *)(key + 8) == '\x02') {
        if (*(byte *)(wlc + 0xa5) == 0) {
          uVar26 = *(uint *)(iVar9 + 0x50) & 8;
          pcStack152 = (code *)(uint)*(byte *)(wlc + 0xa5);
          if ((((uVar26 == 0) &&
               (bVar3 = *(byte *)(key + 6), pcStack152 = (code *)uVar26,
               (uint)bVar3 < *(uint *)(*wlc + 0xbc))) && (3 < bVar3)) &&
             ((bVar3 < 0xc && (-1 < *(int *)(p + 0x18) << 2)))) {
            if (nfrag != 1) goto LAB_0000ec5c;
            pcStack152 = (code *)0x1;
            iStack144 = iStack144 + 8;
          }
        }
        else {
          pcStack152 = Reset;
        }
      }
      else {
        pcStack152 = Reset;
      }
    }
  }
  else {
LAB_0000ec5c:
    pcStack152 = Reset;
  }
  iVar27 = *(int *)(p + 8);
  iVar10 = iVar27 + -0x76;
  *(short *)(p + 0xc) = *(short *)(p + 0xc) + 0x76;
  *(int *)(p + 8) = iVar10;
  func_0x00803b14(iVar10,0,0x70);
  if (((int)((uint)*(ushort *)(p + 0x1c) << 0x13) < 0) || (*(int *)(p + 0x18) << 0x15 < 0)) {
    uVar26 = (uint)*(ushort *)(p + 0x1c) & 0xfff;
LAB_0000ecd0:
    if (uVar25 != 1) {
      uStack172._0_2_ = 0;
      goto LAB_0000ece2;
    }
LAB_0000ecf4:
    uStack172._0_2_ = 0;
  }
  else {
    if ((((DAT_0000ef14 & *(uint *)(scb + 4)) != 0) && ((uVar24 & 0xfc) == 0x88)) &&
       (-1 < (int)((uint)*(byte *)(src + 1) << 0x1f))) {
      iVar11 = scb + (((uint)*(ushort *)(p + 0xe) & 7) + 0x60) * 2;
      uVar22 = *(ushort *)(iVar11 + 6);
      uVar26 = (uint)uVar22;
      if (frag == nfrag - 1) {
        *(short *)(iVar11 + 6) = uVar22 + 1;
      }
      goto LAB_0000ecd0;
    }
    if (uVar25 == 1) goto LAB_0000ecf4;
    uStack172._0_2_ = 0x10;
    uVar26 = 0;
LAB_0000ece2:
    *(ushort *)((int)src + 0x16) = (ushort)(uVar26 << 4) | (ushort)frag & 0xf;
  }
  iVar11 = wlc[2];
  *(ushort *)(p + 0x1c) = *(ushort *)(p + 0x1c) & 0xf000 | *(ushort *)((int)src + 0x16) >> 4;
  iVar11 = func_0x00819e08(iVar11);
  if (iVar11 << 0x1c < 0) {
    *(ushort *)(p + 0x1c) = *(ushort *)(p + 0x1c) | 0x2000;
  }
  if (queue == 4) {
    uStack48 = func_0x0081a064(wlc,iVar9,(uint)*(ushort *)(iVar27 + -0x2a));
  }
  else {
    uVar22 = *(ushort *)((int)wlc + 0x33e);
    if (frag == nfrag - 1) {
      *(short *)((int)wlc + 0x33e) = uVar22 + 1;
    }
    uStack48 = (ushort)((frag & 0xf | ((uint)uVar22 & 0xfff) << 4) << 5) & 0x7fe0 |
               (ushort)queue & 7;
  }
  if (((*(char *)(scb + 0xe7) != '\0') || ((uVar24 & 0xfc) == 0x80)) ||
     (*(char *)(iVar9 + 6) == '\0')) {
    uStack172._0_2_ = (ushort)uStack172 | 0x20;
  }
  uVar26 = DAT_0000ef18 & rspec_override;
  if (uVar26 == 0) {
    uVar20 = rspec_override;
    uStack128 = uVar26;
    if ((rspec_override & 0x3000000) != 0x1000000) {
      if (((uVar25 < 2) || (*(int *)(p + 0x18) << 0x1b < 0)) ||
         ((*(int *)(p + 0x18) < 0 || (*(char *)(p + 0x1f) < '\0')))) {
LAB_0000ee06:
        rspec_override = (uint)*(byte *)(scb + 0x50) & 0x7f;
      }
      else {
        rspec_override = *(uint *)(wlc[8] + 0x48);
        if ((((rspec_override & DAT_0000ef18) == 0) && ((rspec_override & 0x3000000) != 0x1000000))
           || (-1 < (int)((uint)*(byte *)(src + 1) << 0x1f))) {
          rspec_override = *(uint *)(wlc[8] + 0x44);
          if ((((rspec_override & DAT_0000ef18) == 0) && ((rspec_override & 0x3000000) != 0x1000000)
              ) || (uVar26 = (uint)*(byte *)(src + 1) & 1, uVar20 = rspec_override,
                   uStack128 = uVar26, (*(byte *)(src + 1) & 1) != 0)) {
            if (((int)((uint)*(byte *)(src + 1) << 0x1f) < 0) || (*(int *)(scb + 8) << 0x1c < 0))
            goto LAB_0000ee06;
            uStack66 = 2;
            FUN_0003d628(wlc[0x58],scb,&uStack48,&uStack76,&uStack44);
            if (((uVar24 & 0xfc) == 0x48) || ((uVar24 & 0xfc) == 200)) {
              uStack76 = func_0x0082d238(iVar9,uStack76,0);
              uStack72 = (uint)*(byte *)(scb + 0x50) & 0x7f;
              uVar26 = 0;
            }
            else {
              *(uint *)(p + 0x18) = *(uint *)(p + 0x18) | 0x8000000;
              uVar26 = 1 - frag;
              if (1 < frag) {
                uVar26 = 0;
              }
            }
            if ((int)((uint)uStack44 << 0x1f) < 0) {
              *(uint *)(p + 0x18) = *(uint *)(p + 0x18) | 0x2000;
            }
            rspec_override = uStack72;
            uVar20 = uStack76;
            uStack128 = (uint)*(byte *)(*wlc + 0xe5);
            if (*(byte *)(*wlc + 0xe5) != 0) {
              uStack128 = func_0x0086b424(wlc[0x1b9],scb,(uint)uStack48);
            }
          }
          goto LAB_0000eeb2;
        }
      }
      uVar26 = 0;
      uVar20 = rspec_override;
      uStack128 = uVar26;
    }
  }
  else {
    uStack128 = 0;
    uVar26 = 0;
    rspec_override = rspec_override;
    uVar20 = rspec_override;
  }
LAB_0000eeb2:
  iVar11 = wlc[0x11d];
  cVar4 = *(char *)(iVar11 + 0xc);
  pcVar30 = (code *)((uint)*(byte *)(*wlc + 0x4f) & 3);
  if ((*(byte *)(*wlc + 0x4f) & 3) == 0) {
    uVar29 = uVar20 & 0xfff8ffff | 0x10000;
    uVar17 = rspec_override & 0xfff8ffff | 0x10000;
    pcStack148 = pcVar30;
    pcStack136 = pcVar30;
    pcStack132 = pcVar30;
  }
  else {
    if (*(byte *)(iVar11 + 2) < 2) {
      bVar33 = false;
LAB_0000ef1c:
      cVar16 = '\0';
    }
    else {
      bVar33 = *(char *)(wlc[8] + 0x4d) == '\x01';
      if (*(char *)(wlc[8] + 0x4d) != -1) goto LAB_0000ef1c;
      cVar16 = '\0';
      if ((*(uint *)(scb + 4) & 0x40000000) != 0) {
        iVar11 = func_0x00804858(iVar11 + 0xe,2);
        cVar16 = (char)iVar11;
        if (iVar11 != 0) {
          cVar16 = '\x01';
        }
      }
    }
    uVar17 = uVar20 & 0xff;
    if ((uVar20 & 0x3000000) == 0) {
      if (*(char *)(DAT_0000f1bc + uVar17) < '\0') goto LAB_0000ef42;
    }
    else {
      if (0x1f < uVar17) {
        if (uVar17 == 0x20) goto LAB_0000ef42;
        uVar17 = uVar17 - 0x55;
      }
      if (uVar17 < 8) {
LAB_0000ef42:
        if (-1 < (int)uVar20) {
          uVar17 = uVar20 & 0xffeffcff;
          if (((uVar20 & 0x3000000) == 0) ||
             ((!bVar33 && (((uVar20 & 0x3000000) != 0x1000000 || (cVar16 == '\0')))))) {
            uVar20 = uVar17;
            if (cVar4 == '\x01') {
              uVar20 = uVar17 | 0x100;
            }
          }
          else {
            uVar20 = uVar17 | 0x100000;
          }
        }
      }
    }
    uVar17 = rspec_override & 0xff;
    if ((rspec_override & 0x3000000) == 0) {
      if (*(char *)(DAT_0000f1bc + uVar17) < '\0') goto LAB_0000ef94;
    }
    else {
      if (0x1f < uVar17) {
        if (uVar17 == 0x20) goto LAB_0000ef94;
        uVar17 = uVar17 - 0x55;
      }
      if (uVar17 < 8) {
LAB_0000ef94:
        if (-1 < (int)rspec_override) {
          uVar17 = rspec_override & 0xffeffcff;
          if (((rspec_override & 0x3000000) == 0) ||
             ((!bVar33 && (((rspec_override & 0x3000000) != 0x1000000 || (cVar16 == '\0')))))) {
            rspec_override = uVar17;
            if (cVar4 == '\x01') {
              rspec_override = uVar17 | 0x100;
            }
          }
          else {
            rspec_override = uVar17 | 0x100000;
          }
        }
      }
    }
    if ((*(ushort *)(wlc + 0x112) & 0x3800) == 0x1800) {
      uVar18 = uVar20 & 0x70000;
      if (uVar18 == 0) {
        if (((uVar20 & 0x3000000) == 0) || (-1 < *(int *)(scb + 4) << 0xc)) {
          uVar18 = 0x10000;
        }
        else {
          if ((*(char *)(*wlc + 0x3c) == '\0') || (*(char *)(iVar9 + 6) == '\0')) {
            uVar18 = 0x20000;
          }
          else {
            if ((*(ushort *)(*(int *)(iVar9 + 0xf4) + 0x32) & 0x3800) == 0x1800) {
              uVar18 = 0x20000;
            }
            else {
              uVar18 = 0x10000;
            }
          }
        }
      }
      if (((uVar20 & 0x3000000) != 0) && ((uVar20 & 0xff) == 0x20)) {
        uVar18 = 0x20000;
      }
    }
    else {
      if ((uVar20 & 0xff) == 0x20) {
        uVar20 = 0x1000000;
      }
      if ((rspec_override & 0xff) == 0x20) {
        rspec_override = 0x1000000;
      }
      uVar18 = 0x10000;
    }
    uVar17 = rspec_override & 0xfff8ffff;
    bVar33 = (rspec_override & 0x3000000) != 0;
    uVar29 = uVar20 & 0xfff8ffff | uVar18;
    if (bVar33) {
      uVar17 = uVar17 | uVar18;
    }
    if (!bVar33) {
      uVar17 = uVar17 | 0x10000;
    }
    if (((DAT_0000f1c0 & *(uint *)(wlc[8] + 0x44)) == 0) &&
       ((*(uint *)(wlc[8] + 0x44) & 0x3000000) != 0x1000000)) {
      if (((uVar20 & 0x3000000) == 0) || (*(char *)(wlc + 0x88) != '\x01')) {
        uVar19 = uVar29;
        if (*(char *)(wlc + 0x88) == '\0') {
          uVar19 = uVar20 & 0xff78ffff | uVar18;
        }
      }
      else {
        uVar19 = uVar29 | 0x800000;
      }
      if (((uVar17 & 0x3000000) == 0) || (*(char *)(wlc + 0x88) != '\x01')) {
        uVar20 = uVar17;
        if (*(char *)(wlc + 0x88) == '\0') {
          uVar20 = uVar17 & 0xff7fffff;
        }
      }
      else {
        uVar20 = uVar17 | 0x800000;
      }
      uVar29 = uVar19 & 0xffbfffff;
      uVar17 = uVar20 & 0xffbfffff;
      if ((*(char *)(wlc[0x11d] + 0x13) == '\x01') ||
         (((*(int *)(scb + 4) << 0xf < 0 && (*(int *)(scb + 8) << 0x18 < 0)) &&
          (*(char *)(wlc[0x11d] + 0x13) == -1)))) {
        if (((((uVar19 & 0x3000000) != 0) && (uVar19 = uVar19 & 0xff, 1 < uVar19 - 0x57)) &&
            (uVar19 != 99)) && (((uVar19 != 100 && (uVar19 != 0x65)) && (uVar19 != 0x66)))) {
          uVar29 = uVar29 | 0x400000;
        }
        if ((((((uVar20 & 0x3000000) != 0) && (uVar20 = uVar20 & 0xff, 1 < uVar20 - 0x57)) &&
             (uVar20 != 99)) && ((uVar20 != 100 && (uVar20 != 0x65)))) && (uVar20 != 0x66)) {
          uVar17 = uVar17 | 0x400000;
        }
      }
    }
    pcVar12 = (code *)func_0x0081fe30(wlc,scb);
    pcVar30 = (code *)(uVar29 & 0x3000000);
    pcStack148 = pcVar30;
    pcStack132 = pcVar30;
    if (pcVar30 != Reset) {
      if (cVar2 == '\x02') {
        pcVar30 = (code *)(uint)((uVar29 & 0x70000) == 0x20000);
      }
      else {
        pcVar30 = Reset;
      }
      uVar20 = uVar29 & 0xff;
      if (uVar20 < 0x20) {
        bVar33 = 7 < uVar20;
      }
      else {
        if (uVar20 == 0x20) {
          bVar33 = false;
        }
        else {
          bVar33 = 7 < uVar20 - 0x55;
        }
      }
      pcStack132 = (code *)(uint)bVar33;
      if (((bVar33 != false) &&
          (pcStack132 = (code *)(uint)*(byte *)(scb + 0x10e), *(byte *)(scb + 0x10e) != 0)) &&
         (pcStack132 = (code *)(uint)*(byte *)(scb + 0x10f), *(byte *)(scb + 0x10f) != 0)) {
        pcStack132 = Reset + 1;
      }
      pcStack148 = pcVar12;
      if ((uVar29 & 0x800000) != 0) {
        if (uVar20 < 0x20) {
          bVar33 = uVar20 < 8;
        }
        else {
          if (uVar20 == 0x20) {
            bVar33 = true;
          }
          else {
            bVar33 = uVar20 - 0x55 < 8;
          }
        }
        if (bVar33) {
          pcStack148 = UndefinedInstruction;
        }
      }
    }
    pcStack136 = (code *)(uVar17 & 0x3000000);
    if (((code *)(uVar17 & 0x3000000) != Reset) && (pcStack136 = pcVar12, (uVar17 & 0x800000) != 0))
    {
      uVar20 = uVar17 & 0xff;
      if (uVar20 < 0x20) {
        bVar33 = uVar20 < 8;
      }
      else {
        if (uVar20 == 0x20) {
          bVar33 = true;
        }
        else {
          bVar33 = uVar20 - 0x55 < 8;
        }
      }
      if (bVar33) {
        pcStack136 = UndefinedInstruction;
      }
    }
  }
  if (uVar26 != 0) {
    iVar11 = *(int *)(iVar9 + 0x11c);
    *(uint *)(iVar9 + (iVar11 + 0x24) * 8) = uVar29;
    *(uint *)(iVar9 + iVar11 * 8 + 0x124) = nfrag & 0xff;
    *(uint *)(iVar9 + 0x11c) = iVar11 + 1U & 0x3f;
    *(uint *)(scb + 0x230) = uVar17;
  }
  uVar26 = uVar29 & 0x3000000;
  if (uVar26 == 0) {
    pcStack92 = (code *)(uVar29 & 0xff);
  }
  else {
    pcStack92 = (code *)FUN_000389d8(uVar29);
  }
  if ((uVar25 == 2) || (uVar25 == 0)) {
    if (((int)(uint)*(ushort *)(wlc + 0x116) < iStack144) || (*(int *)(p + 0x18) << 5 < 0)) {
      pcStack108 = pcStack132;
      if ((*(byte *)(src + 1) & 1) == 0) {
        pcStack108 = Reset + 1;
      }
    }
    else {
      pcStack108 = pcStack132;
    }
  }
  else {
    pcStack108 = pcStack132;
  }
  if (((((*(char *)(wlc[8] + 0x15) != '\0') && (cVar1 != '\0')) && (uVar26 == 0)) &&
      (*(char *)(DAT_0000f4c8 + (uVar29 & 0xff)) < '\0')) ||
     ((((*(byte *)(*wlc + 0x4f) & 3) != 0 && (uVar26 != 0)) && (cVar2 != '\0')))) {
    if (nfrag < 2) {
      if ((*(char *)(wlc[8] + 0x15) != '\0') && (cVar1 != '\0')) {
        if (uVar26 == 0) {
          uVar26 = uVar29 & 0x7f;
          if ((((uVar26 != 2) && (uVar26 != 4)) && (uVar26 != 0xb)) && (uVar26 != 0x16)) {
            pcVar30 = Reset + 1;
          }
        }
        else {
          pcVar30 = Reset + 1;
        }
      }
    }
    else {
      if (cVar1 == '\0') {
        uVar29 = 0x30;
      }
      else {
        uVar29 = 0x16;
      }
      uVar29 = uVar29 | 0x10000;
      *(uint *)(p + 0x18) = *(uint *)(p + 0x18) & 0xf7ffffff;
      uVar17 = uVar29;
    }
  }
  pcVar12 = (code *)(uVar29 & 0x3000000);
  if (((pcVar12 == Reset) && ((uVar29 & 0x7f) < 0x17)) &&
     (((DAT_0000f4cc << (uVar29 & 0x7f) < 0 &&
       ((pcStack148 = pcVar23, pcVar23 != Reset && (pcStack148 = pcVar12, (uVar29 & 0xff) != 2))))
      && (pcStack148 = (code *)((int)*(char *)(*(int *)(scb + 0x10) + 0x118) + -1),
         pcStack148 != Reset)))) {
    pcStack148 = Reset + 1;
  }
  pcVar15 = (code *)(uVar17 & 0x3000000);
  if (((((pcVar15 == Reset) && ((uVar17 & 0x7f) < 0x17)) && (DAT_0000f4cc << (uVar17 & 0x7f) < 0))
      && ((pcStack136 = pcVar15, pcVar23 != Reset && ((uVar17 & 0xff) != 2)))) &&
     (pcStack136 = (code *)((int)*(char *)(*(int *)(scb + 0x10) + 0x118) + -1), pcStack136 != Reset)
     ) {
    pcStack136 = Reset + 1;
  }
  if (uVar25 == 2) {
    *(uint *)(scb + 0x168) = uVar29;
  }
  pcStack160 = (code *)(*(uint *)(scb + 4) & 0x10000);
  if (((pcStack160 != Reset) &&
      (pcStack160 = (code *)(uint)*(byte *)((int)wlc + 0x211), *(byte *)((int)wlc + 0x211) != 0)) &&
     (pcStack160 = (code *)(uint)*(byte *)((int)wlc + 0x215), *(byte *)((int)wlc + 0x215) != 0)) {
    if (cVar2 == '\x03') {
      pcStack160 = Reset;
    }
    else {
      if ((pcVar12 != Reset) ||
         ((((uVar26 = uVar29 & 0x7f, pcStack160 = pcVar12, uVar26 != 2 && (uVar26 != 4)) &&
           (uVar26 != 0xb)) && (uVar26 != 0x16)))) {
        pcStack160 = (code *)((uint)*(byte *)(src + 1) & 1);
        if ((*(byte *)(src + 1) & 1) == 0) {
          if (((uVar24 & 0xfc) == 0x88) && (queue < 4)) {
            *(uint *)(p + 0x18) = *(uint *)(p + 0x18) | 0x1000;
            if (bVar32) {
              iVar11 = 0x1e;
            }
            else {
              iVar11 = 0x18;
            }
            uStack172._0_2_ = (ushort)uStack172 | 0x5000;
            *(ushort *)((int)src + iVar11) = *(ushort *)((int)src + iVar11) & 0xff9f | 0x20;
            pcStack160 = Reset + 1;
          }
        }
        else {
          pcStack160 = Reset;
        }
      }
    }
  }
  func_0x00825654(wlc,uVar29,iStack144,uVar24,iVar27 + -6);
  func_0x00825654(wlc,uVar17,iStack144,uVar24,auStack64);
  memcpy((undefined4 *)(iVar27 + -0x40),auStack64,6);
  if (((pcVar15 == Reset) && ((uVar17 & 0x7f) < 0x17)) && (DAT_0000f4cc << (uVar17 & 0x7f) < 0)) {
    *(undefined *)(iVar27 + -0x3c) = (char)iStack144;
    *(undefined *)(iVar27 + -0x3b) = (char)((uint)iStack144 >> 8);
  }
  if ((int)(*(uint *)(p + 0x18) << 0x15) < 0) {
    if (pcVar12 != Reset) {
      if ((((key == 0) || (*(char *)(key + 8) == '\x04')) || (*(char *)(key + 8) == '\v')) &&
         (*(uint *)(p + 0x18) = *(uint *)(p + 0x18) | 0x800, *(char *)((int)wlc + 0x2be) != '\0')) {
        pcStack108 = Reset + 1;
      }
      goto LAB_0000f4d0;
    }
LAB_0000f4ae:
    if (-1 < *(char *)(DAT_0000f4c8 + (uVar29 & 0xff))) goto LAB_0000f4d0;
    uStack120 = (ushort)*(byte *)(iVar27 + -6) & 0xf;
  }
  else {
    if (pcVar12 == Reset) goto LAB_0000f4ae;
LAB_0000f4d0:
    uStack120 = (ushort)*(byte *)(iVar27 + -6);
  }
  if ((uVar7 == 0xa4) || ((int)((uint)*(byte *)(src + 1) << 0x1f) < 0)) {
    if (pcStack160 != Reset) goto LAB_0000f50a;
LAB_0000f520:
    if (uVar7 != 0xa4) goto LAB_0000f530;
    *(undefined2 *)(iVar27 + -0x3a) = *(undefined2 *)((int)src + 2);
  }
  else {
    if (pcStack160 != Reset) {
LAB_0000f50a:
      sVar6 = func_0x008251c8(wlc,uVar29,pcStack148,0x92a);
      *(short *)((int)src + 2) = sVar6 + 2;
      goto LAB_0000f520;
    }
    if (*(int *)(p + 0x18) << 0x15 < 0) {
      uVar5 = func_0x0081d368();
    }
    else {
      uVar5 = func_0x00825608(wlc,uVar29,pcStack148,next_frag_len);
    }
    *(undefined2 *)((int)src + 2) = uVar5;
LAB_0000f530:
    if (((int)((uint)*(byte *)(src + 1) << 0x1f) < 0) || (pcStack160 != Reset)) {
      *(undefined *)(iVar27 + -0x3a) = 0;
      *(undefined *)(iVar27 + -0x39) = 0;
    }
    else {
      if (*(int *)(p + 0x18) << 0x15 < 0) {
        uVar5 = func_0x0081d368();
      }
      else {
        uVar5 = func_0x00825608(wlc,uVar17,pcStack136,next_frag_len);
      }
      *(undefined2 *)(iVar27 + -0x3a) = uVar5;
    }
  }
  iVar11 = *(int *)(p + 0x18);
  if (iVar11 << 0x16 < 0) {
    *(undefined2 *)(iVar27 + -0x34) = *(undefined2 *)(p + 0x24);
    *(undefined2 *)(iVar27 + -0x32) = *(undefined2 *)(p + 0x26);
    uStack172._0_2_ = (ushort)uStack172 | 0x2000;
  }
  if (frag == 0) {
    uStack172._0_2_ = (ushort)uStack172 | 8;
  }
  if ((((-1 < (int)((uint)*(byte *)(src + 1) << 0x1f)) && (-1 < iVar11 << 0x13)) &&
      ((*(char *)(wlc + 0x85) == '\0' || (-1 < iVar11 << 0x19)))) &&
     (((iVar11 << 0x15 < 0 || (bVar21 == 0)) || (*(char *)(iVar14 + 0x28) == '\0')))) {
    uStack172._0_2_ = (ushort)uStack172 | 1;
  }
  uVar26 = (uint)*(byte *)(DAT_0000f804 + queue);
  if ((((uVar25 == 2) && (queue < 4)) &&
      ((*(char *)((int)wlc + 0x211) != '\0' &&
       ((UndefinedInstruction < pcStack92 && (uVar25 = FUN_0001b2cc(wlc[0x4d]), uVar25 == 0)))))) &&
     (((*(short *)(iVar14 + uVar26 * 2 + 0x1c) == 0 || (*(int *)(p + 0x18) << 0x15 < 0)) &&
      (pcStack132 == Reset)))) {
    uStack172._0_2_ = (ushort)uStack172 | 0x1000;
  }
  uVar25 = FUN_0002e060(*(int *)(wlc[8] + 0x10));
  if ((uVar25 & 0x3800) == 0x1800) {
    uStack172._0_2_ = (ushort)uStack172 | 0x100;
  }
  if (pcStack152 != Reset) {
    uStack172._0_2_ = (ushort)uStack172 | 0x8000;
  }
  *(ushort *)(iVar27 + -0x76) = (ushort)uStack172;
  if (key == 0) {
    uStack172 = key;
  }
  else {
    uStack172 = (uint)*(byte *)(wlc + 0xa5);
    if (*(byte *)(wlc + 0xa5) == 0) {
      uVar25 = *(uint *)(iVar9 + 0x50) & 8;
      if ((uVar25 == 0) &&
         ((*(char *)(key + 8) != '\v' ||
          ((uStack172 = (uint)*(byte *)(*wlc + 0xb1), *(byte *)(*wlc + 0xb1) != 0 &&
           (uStack172 = uVar25, *(byte *)(key + 6) < 8)))))) {
        if ((uint)*(byte *)(key + 6) < *(uint *)(*wlc + 0xbc)) {
          if (*(int *)(p + 0x18) << 2 < 0) {
            uStack172 = 0;
          }
          else {
            uStack172 = (uint)*(byte *)(key + 0xc) & 7 | (uint)*(byte *)(key + 6) << 4;
          }
        }
        else {
          uStack172 = 0;
        }
      }
    }
    else {
      uStack172 = 0;
    }
  }
  if (((uint)(pcStack136 + -1) & 0xff) < 2) {
    uStack172 = uStack172 & 0xffff | 0x2000;
  }
  memcpy((undefined4 *)(iVar27 + -0x72),src,2);
  *(undefined *)(iVar27 + -0x70) = 0;
  *(undefined *)(iVar27 + -0x6f) = 0;
  *(undefined *)(iVar27 + -0x4a) = 0;
  *(undefined *)(iVar27 + -0x49) = 0;
  if ((((key != 0) && (*(char *)(wlc + 0xa5) == '\0')) && (-1 < *(int *)(iVar9 + 0x50) << 0x1c)) &&
     ((((*(char *)(key + 8) != '\v' ||
        ((*(char *)(*wlc + 0xb1) != '\0' && (*(byte *)(key + 6) < 8)))) &&
       ((uint)*(byte *)(key + 6) < *(uint *)(*wlc + 0xbc))) && (-1 < *(int *)(p + 0x18) << 2)))) {
    if (bVar32) {
      dst = (undefined4 *)((int)src + 0x1e);
    }
    else {
      dst = src + 6;
    }
    if (bVar21 != 0) {
      dst = (undefined4 *)((int)dst + 2);
    }
    func_0x008303d8(wlc,iVar10,dst,key,0);
  }
  memcpy((undefined4 *)(iVar27 + -0x50),src + 1,6);
  *(ushort *)(iVar27 + -0x2a) = uStack48;
  sVar6 = func_0x0085d124(wlc[0x52],iVar9);
  *(ushort *)(iVar27 + -0x30) = *(ushort *)(iVar27 + -0x30) | sVar6 << 8;
  *(undefined *)(iVar27 + -0x28) = 0;
  *(undefined *)(iVar27 + -0x27) = 0;
  *(undefined *)(iVar27 + -0x26) = 0;
  *(undefined *)(iVar27 + -0x25) = 0;
  *(undefined *)(iVar27 + -0x24) = 0;
  *(undefined *)(iVar27 + -0x23) = 0;
  *(undefined *)(iVar27 + -0x22) = 0;
  *(undefined *)(iVar27 + -0x21) = 0;
  *(undefined *)(iVar27 + -0x20) = 0;
  *(undefined *)(iVar27 + -0x1f) = 0;
  if (pcStack108 == Reset) {
    if (pcVar30 != Reset) goto LAB_0000f78c;
    func_0x00803b14(iVar27 + -0x1e,0,6);
    func_0x00803b14(iVar27 + -0x18,0,0x10);
    func_0x00803b14(iVar27 + -0x48,0,6);
    *(undefined *)(iVar27 + -0x42) = 0;
    *(undefined *)(iVar27 + -0x41) = 0;
    pcStack176 = pcVar30;
    pcStack168 = pcVar30;
    pcStack156 = pcVar30;
    pcStack152 = pcVar30;
    pcStack132 = pcVar30;
  }
  else {
    pcVar30 = Reset;
LAB_0000f78c:
    pcStack176 = (code *)func_0x0082d238(iVar9,uVar29,0);
    pcStack168 = (code *)func_0x0082d238(iVar9,uVar17,0);
    pcVar23 = (code *)((uint)pcStack176 & 0x3000000);
    if (pcVar23 == Reset) {
      puVar13 = (undefined *)((uint)pcStack176 & 0xff);
      pcStack152 = pcVar23;
      if (-1 < (char)puVar13[DAT_0000f808]) goto LAB_0000f7c2;
    }
    else {
      puVar13 = FUN_000389d8((uint)pcStack176);
LAB_0000f7c2:
      pcStack152 = (code *)(puVar13 + -2);
      if (pcStack152 != Reset) {
        pcStack152 = Reset + 1;
      }
      if (pcStack152 != Reset) {
        if (*(char *)(*(int *)(scb + 0x10) + 0x118) == '\x01') {
          pcStack152 = Reset;
        }
        else {
          uStack172 = uStack172 & 0xffff | 0x4000;
          pcStack152 = Reset + 1;
        }
      }
    }
    pcStack132 = (code *)((uint)pcStack168 & 0x3000000);
    if (pcStack132 == Reset) {
      puVar13 = (undefined *)((uint)pcStack168 & 0xff);
      if (-1 < (char)puVar13[DAT_0000f808]) goto LAB_0000f812;
    }
    else {
      puVar13 = FUN_000389d8((uint)pcStack168);
LAB_0000f812:
      pcStack132 = (code *)(puVar13 + -2);
      if (pcStack132 != Reset) {
        pcStack132 = Reset + 1;
      }
      if (pcStack132 != Reset) {
        if (*(char *)(*(int *)(scb + 0x10) + 0x118) == '\x01') {
          pcStack132 = Reset;
        }
        else {
          uStack172._0_2_ = ~(~(ushort)((uStack172 << 0x11) >> 0x10) >> 1);
          pcStack132 = Reset + 1;
        }
      }
    }
    if (pcVar30 == Reset) {
      uVar7 = 6;
    }
    else {
      uVar7 = 0x800;
    }
    *(ushort *)(iVar27 + -0x76) = *(ushort *)(iVar27 + -0x76) | uVar7;
    if (pcVar30 == Reset) {
      uVar31 = 0x14;
    }
    else {
      uVar31 = 0xe;
    }
    func_0x00825654(wlc,pcStack176,uVar31,uVar24,iVar27 + -0x1e);
    func_0x00825654(wlc,pcStack168,uVar31,uVar24,auStack56);
    memcpy((undefined4 *)(iVar27 + -0x48),auStack56,6);
    pcStack156 = (code *)(iVar27 - 0x18);
    uVar5 = func_0x00825690(wlc,pcVar30,pcStack176,uVar29,pcStack152,pcStack148,iStack144,0);
    *(undefined2 *)(iVar27 + -0x16) = uVar5;
    uVar5 = func_0x00825690(wlc,pcVar30,pcStack168,uVar17,pcStack132,pcStack136,iStack144,0);
    *(undefined2 *)(iVar27 + -0x42) = uVar5;
    dst = (undefined4 *)(iVar27 + -0x14);
    if (pcVar30 == Reset) {
      *(undefined *)(iVar27 + -0x18) = 0xb4;
      *(undefined *)(iVar27 + -0x17) = 0;
      memcpy(dst,src + 1,6);
      dst = (undefined4 *)(iVar27 + -0xe);
    }
    else {
      *(undefined *)(iVar27 + -0x18) = 0xc4;
      *(undefined *)(iVar27 + -0x17) = 0;
    }
    memcpy(dst,(undefined4 *)((int)src + 10),6);
    if ((pcVar23 == Reset) && (*(char *)(DAT_0000fbac + ((uint)pcStack176 & 0xff)) < '\0')) {
      uVar24 = (uint)*(byte *)(iVar27 + -0x1e) & 0xf;
    }
    else {
      uVar24 = (uint)*(byte *)(iVar27 + -0x1e);
    }
    uStack120 = uStack120 | (ushort)(uVar24 << 8);
  }
  if ((*(int *)(p + 0x18) << 0x15 < 0) && (pcVar12 != Reset)) {
    auStack42[0] = 0;
    uVar24 = FUN_0001d98a((int *)wlc[0x4d],scb,uVar29,iStack144,auStack42);
    *(undefined *)(iVar27 + -0x43) = (char)uVar24;
  }
  *(ushort *)(iVar27 + -0x74) = (ushort)uStack172;
  *(ushort *)(iVar27 + -100) = uStack120;
  if (pcVar15 == (code *)0x1000000) {
    uVar7 = 2;
  }
  else {
    if (((pcVar15 == Reset) && ((uVar17 & 0x7f) < 0x17)) && (DAT_0000fbb0 << (uVar17 & 0x7f) < 0)) {
      uVar7 = 0;
    }
    else {
      uVar7 = 1;
    }
  }
  uVar24 = (uint)pcStack176 & 0x3000000;
  if (uVar24 == 0x1000000) {
    uVar22 = 8;
  }
  else {
    if (((uVar24 != 0) || (0x16 < ((uint)pcStack176 & 0x7f))) ||
       (-1 < DAT_0000fbb0 << ((uint)pcStack176 & 0x7f))) {
      uVar24 = 1;
    }
    uVar22 = (ushort)((uVar24 & 0x3fff) << 2);
  }
  uVar24 = (uint)pcStack168 & 0x3000000;
  if (uVar24 == 0x1000000) {
    uVar28 = 0x20;
  }
  else {
    if (((uVar24 != 0) || (0x16 < ((uint)pcStack168 & 0x7f))) ||
       (-1 < DAT_0000fbb0 << ((uint)pcStack168 & 0x7f))) {
      uVar24 = 1;
    }
    uVar28 = (ushort)((uVar24 & 0xfff) << 4);
  }
  uVar24 = FUN_0002e060(*(int *)(wlc[8] + 0x10));
  *(ushort *)(iVar27 + -0x62) = uVar28 | uVar7 | uVar22 | (ushort)((uVar24 & 0xff) << 8);
  if (pcVar12 == (code *)0x1000000) {
    uStack46 = 2;
  }
  else {
    if (((pcVar12 == Reset) && ((uVar29 & 0x7f) < 0x17)) && (DAT_0000fbb0 << (uVar29 & 0x7f) < 0)) {
      uStack46 = 0;
    }
    else {
      uStack46 = 1;
    }
  }
  if (((uint)(pcStack148 + -1) & 0xff) < 2) {
    uStack46 = uStack46 | 0x10;
    *(int *)(*(int *)(*wlc + 0x88) + 0x18) = *(int *)(*(int *)(*wlc + 0x88) + 0x18) + 1;
  }
  uVar25 = (uint)uStack46;
  uVar24 = FUN_0003f7ca((int)wlc);
  uVar25 = uVar25 | uVar24 & 0xffff;
  uStack46 = (ushort)uVar25;
  if ((*(int *)(p + 0x18) << 4 < 0) && (*(char *)(*wlc + 0xe5) != '\0')) {
    FUN_0002f2da(*(int *)(wlc[8] + 0x10),uVar25);
    FUN_0002f2ec(*(int *)(wlc[8] + 0x10),&uStack46,uStack128,wlc[8]);
  }
  *(ushort *)(iVar27 + -0x6e) = uStack46;
  uVar5 = func_0x0082b630(wlc,uVar29,(uint)*(ushort *)(wlc + 0x112));
  *(undefined2 *)(iVar27 + -0x6c) = uVar5;
  uVar5 = func_0x0082b630(wlc,uVar17,(uint)*(ushort *)(wlc + 0x112));
  *(undefined2 *)(iVar27 + -0x6a) = uVar5;
  if ((pcStack108 != Reset) || (pcVar30 != Reset)) {
    uVar5 = func_0x0082b630(wlc,pcStack176,(uint)*(ushort *)(wlc + 0x112));
    *(undefined2 *)(iVar27 + -0x68) = uVar5;
    uVar5 = func_0x0082b630(wlc,pcStack168,(uint)*(ushort *)(wlc + 0x112));
    *(undefined2 *)(iVar27 + -0x66) = uVar5;
  }
  if ((pcVar12 != Reset) && (pcStack148 == UndefinedInstruction)) {
    uVar5 = func_0x00825310(wlc,uVar29,iStack144);
    *(undefined2 *)(iVar27 + -0x38) = uVar5;
  }
  if ((pcVar15 != Reset) && (pcStack136 == UndefinedInstruction)) {
    uVar5 = func_0x00825310(wlc,uVar17,iStack144);
    *(undefined2 *)(iVar27 + -0x36) = uVar5;
  }
  if ((-1 < *(int *)(scb + 4) << 0x19) || (bVar21 == 0)) goto LAB_0000fcac;
  if (*(short *)(iVar14 + uVar26 * 2 + 0x1c) == 0) {
    if ((*(char *)(*wlc + 0x45) == '\0') || (3 < queue)) goto LAB_0000fcac;
    uVar26 = (uint)*(byte *)(DAT_0000fcd8 + queue);
    if (pcStack156 == Reset) {
      iVar9 = func_0x008251c8(wlc,uVar29,pcStack148,iStack144);
      iVar14 = func_0x00825608(wlc,uVar29,pcStack148,0);
      uVar24 = iVar14 + iVar9;
    }
    else {
      iVar9 = func_0x0081d044(wlc,pcStack176,pcStack152);
      uVar24 = iVar9 + (uint)*(ushort *)(pcStack156 + 2);
    }
    iVar9 = wlc[0x59];
  }
  else {
    if ((*(int *)(p + 0x18) << 0x15 < 0) || (frag != 0)) goto LAB_0000fcac;
    uVar25 = func_0x008251c8(wlc,uVar29,pcStack148,iStack144);
    if (pcStack156 == Reset) {
      if (pcStack160 == Reset) {
        iVar9 = func_0x00825608(wlc,uVar29,pcStack148,0);
        uVar24 = iVar9 + uVar25;
        sVar6 = func_0x008251c8(wlc,uVar17,pcStack136,iStack144);
        sVar8 = func_0x00825608(wlc,uVar17,pcStack136,0);
        goto LAB_0000fbe4;
      }
      sVar8 = 0;
      uVar24 = uVar25;
    }
    else {
      iVar9 = func_0x0081d044(wlc,pcStack176,pcStack152);
      sVar8 = func_0x0081d044(wlc,pcStack168,pcStack132);
      uVar24 = (uint)*(ushort *)(pcStack156 + 2) + iVar9;
      sVar6 = *(short *)(iVar27 + -0x42);
LAB_0000fbe4:
      sVar8 = sVar8 + sVar6;
    }
    *(undefined2 *)(iVar27 + -0x70) = (short)(uVar24 & 0xffff);
    *(short *)(iVar27 + -0x4a) = sVar8;
    if (-1 < (int)(((uVar25 + *(ushort *)(iVar14 + uVar26 * 2 + 0x1c)) - (uVar24 & 0xffff)) *
                  0x10000)) {
      uVar25 = func_0x0081d048(wlc,uVar29,pcStack148);
      if (uVar25 < 0x100) {
        uVar7 = 0x100;
      }
      else {
        uVar7 = *(ushort *)((int)wlc + 0x44a);
        if (uVar25 < *(ushort *)((int)wlc + 0x44a)) {
          uVar7 = (ushort)uVar25;
        }
      }
      if (*(ushort *)((int)wlc + queue * 2 + 0x44c) != uVar7) {
        func_0x0081e084(wlc);
      }
    }
    if ((*(char *)(*wlc + 0x45) == '\0') || (3 < queue)) goto LAB_0000fcac;
    iVar9 = wlc[0x59];
  }
  func_0x0084bb90(iVar9,uVar26,uVar24,scb);
LAB_0000fcac:
  *(uint *)(p + 0x18) = *(uint *)(p + 0x18) | 0x84;
  return (uint)uStack48;
}


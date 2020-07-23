
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
  short sVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  code *pcVar11;
  undefined *puVar12;
  undefined4 *dst;
  int iVar13;
  code *pcVar14;
  char cVar15;
  undefined4 *src;
  uint uVar16;
  uint uVar17;
  uint uVar18;
  uint uVar19;
  byte bVar20;
  ushort uVar21;
  code *pcVar22;
  uint uVar23;
  uint uVar24;
  uint uVar25;
  uint uVar26;
  int iVar27;
  ushort uVar28;
  ushort uVar29;
  uint uVar30;
  code *pcVar31;
  undefined4 uVar32;
  bool bVar33;
  bool bVar34;
  uint uStack180;
  code *pcStack176;
  code *pcStack172;
  code *pcStack164;
  code *pcStack160;
  code *pcStack156;
  code *pcStack152;
  int iStack148;
  uint uStack140;
  uint uStack136;
  code *pcStack132;
  code *pcStack128;
  ushort uStack120;
  code *pcStack108;
  code *pcStack88;
  uint uStack76;
  uint uStack72;
  undefined uStack66;
  undefined4 auStack64 [2];
  undefined4 auStack56 [2];
  ushort uStack48;
  ushort uStack46;
  ushort uStack44;
  undefined2 auStack42 [3];
  
  pcVar22 = (code *)(short_preamble & 1);
  iVar8 = *(int *)(scb + 0x10);
  iVar13 = *(int *)(iVar8 + 0x344);
  cVar1 = **(char **)(iVar8 + *(int *)wlc[0x11b]);
  src = *(undefined4 **)(p + 8);
  cVar2 = **(char **)(iVar8 + *(int *)wlc[0x11c]);
  uVar28 = *(ushort *)src;
  uVar23 = (uint)uVar28;
  uVar24 = (uVar23 & 0xc) >> 2;
  if (uVar24 == 2) {
    bVar20 = (byte)uVar28 >> 7;
  }
  else {
    bVar20 = 0;
  }
  uVar25 = (uint)bVar20;
  bVar33 = (uVar23 & 0x300) == 0x300;
  iStack148 = FUN_00004744(wlc[1],p);
  iStack148 = iStack148 + 4;
  if ((key != 0) &&
     ((*(char *)(key + 8) != '\v' || ((*(char *)(*wlc + 0xb1) != '\0' && (*(byte *)(key + 6) < 8))))
     )) {
    iStack148 = iStack148 + *(char *)(key + 0xf);
  }
  if (wlc[0x6d] < 0) {
    pcStack156 = (code *)key;
    if (key != 0) {
      if (*(char *)(key + 8) == '\x02') {
        if (*(byte *)(wlc + 0xa5) == 0) {
          uVar26 = *(uint *)(iVar8 + 0x50) & 8;
          pcStack156 = (code *)(uint)*(byte *)(wlc + 0xa5);
          if ((((uVar26 == 0) &&
               (bVar3 = *(byte *)(key + 6), pcStack156 = (code *)uVar26,
               (uint)bVar3 < *(uint *)(*wlc + 0xbc))) && (3 < bVar3)) &&
             ((bVar3 < 0xc && (-1 < *(int *)(p + 0x18) << 2)))) {
            if (nfrag != 1) goto LAB_0000a124;
            pcStack156 = (code *)0x1;
            iStack148 = iStack148 + 8;
          }
        }
        else {
          pcStack156 = Reset;
        }
      }
      else {
        pcStack156 = Reset;
      }
    }
  }
  else {
LAB_0000a124:
    pcStack156 = Reset;
  }
  iVar27 = *(int *)(p + 8);
  iVar9 = iVar27 + -0x76;
  *(short *)(p + 0xc) = *(short *)(p + 0xc) + 0x76;
  *(int *)(p + 8) = iVar9;
  func_0x00803b14(iVar9,0,0x70);
  if (((int)((uint)*(ushort *)(p + 0x1c) << 0x13) < 0) || (*(int *)(p + 0x18) << 0x15 < 0)) {
    uStack136 = (uint)*(ushort *)(p + 0x1c) & 0xfff;
LAB_0000a1a4:
    if (uVar24 == 1) {
      uStack180._0_2_ = 0;
    }
    else {
      uStack180._0_2_ = 0;
LAB_0000a1b8:
      uStack136 = frag & 0xf | (uStack136 & 0xfff) << 4;
      *(undefined2 *)((int)src + 0x16) = (short)uStack136;
    }
  }
  else {
    if (((((uint)PTR_DAT_0000a3b0 & *(uint *)(scb + 4)) != 0) && ((uVar23 & 0xfc) == 0x88)) &&
       (-1 < (int)((uint)*(byte *)(src + 1) << 0x1f))) {
      iVar10 = scb + (((uint)*(ushort *)(p + 0xe) & 7) + 0x60) * 2;
      uVar21 = *(ushort *)(iVar10 + 6);
      uStack136 = (uint)uVar21;
      if (frag == nfrag - 1) {
        *(short *)(iVar10 + 6) = uVar21 + 1;
      }
      goto LAB_0000a1a4;
    }
    if (uVar24 != 1) {
      uStack180._0_2_ = 0x10;
      uStack136 = 0;
      goto LAB_0000a1b8;
    }
    uStack180._0_2_ = 0;
    uStack136 = 0;
  }
  *(ushort *)(p + 0x1c) = *(ushort *)(p + 0x1c) & 0xf000 | *(ushort *)((int)src + 0x16) >> 4;
  iVar10 = func_0x00819e08(wlc[2]);
  if (iVar10 << 0x1c < 0) {
    *(ushort *)(p + 0x1c) = *(ushort *)(p + 0x1c) | 0x2000;
  }
  if (queue == 4) {
    uStack48 = func_0x0081a064(wlc,iVar8,(uint)*(ushort *)(iVar27 + -0x2a));
  }
  else {
    uStack136 = frag & 0xf | ((uint)*(ushort *)((int)wlc + 0x33e) & 0xfff) << 4;
    if (frag == nfrag - 1) {
      *(short *)((int)wlc + 0x33e) = *(ushort *)((int)wlc + 0x33e) + 1;
    }
    uStack48 = (ushort)(uStack136 << 5) & 0x7fe0 | (ushort)queue & 7;
  }
  if (((*(char *)(scb + 0xe7) != '\0') || ((uVar23 & 0xfc) == 0x80)) ||
     (*(char *)(iVar8 + 6) == '\0')) {
    uStack180._0_2_ = (ushort)uStack180 | 0x20;
  }
  uVar26 = DAT_0000a3b4 & rspec_override;
  if (uVar26 == 0) {
    uVar19 = rspec_override;
    uStack140 = uVar26;
    if ((rspec_override & 0x3000000) != 0x1000000) {
      if (((uVar24 < 2) || (*(int *)(p + 0x18) << 0x1b < 0)) ||
         ((*(int *)(p + 0x18) < 0 || (*(char *)(p + 0x1f) < '\0')))) {
        rspec_override = (uint)*(byte *)(scb + 0x50) & 0x7f;
        uStack140 = 0;
        uVar26 = 0;
        uVar19 = rspec_override;
      }
      else {
        rspec_override = *(uint *)(wlc[8] + 0x48);
        if ((((rspec_override & DAT_0000a3b4) == 0) && ((rspec_override & 0x3000000) != 0x1000000))
           || (-1 < (int)((uint)*(byte *)(src + 1) << 0x1f))) {
          rspec_override = *(uint *)(wlc[8] + 0x44);
          if ((((rspec_override & DAT_0000a3b4) == 0) && ((rspec_override & 0x3000000) != 0x1000000)
              ) || (uVar26 = (uint)*(byte *)(src + 1) & 1, uVar19 = rspec_override,
                   uStack140 = uVar26, (*(byte *)(src + 1) & 1) != 0)) {
            if (((int)((uint)*(byte *)(src + 1) << 0x1f) < 0) ||
               (uVar26 = *(uint *)(scb + 8) & 8, uVar26 != 0)) {
              rspec_override = (uint)*(byte *)(scb + 0x50) & 0x7f;
              goto LAB_0000a3a6;
            }
            if (*(int *)(p + 0x34) << 2 < 0) {
              rspec_override = (uint)*(byte *)(scb + 0x50) & 0x7f;
              uVar19 = rspec_override;
              uStack140 = uVar26;
            }
            else {
              uStack66 = 2;
              FUN_00030768(wlc[0x58],scb,&uStack48,&uStack76,&uStack44);
              if (((uVar23 & 0xfc) == 0x48) || ((uVar23 & 0xfc) == 200)) {
                uStack76 = func_0x0082d238(iVar8,uStack76,0);
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
              uVar19 = uStack76;
              uStack140 = (uint)*(byte *)(*wlc + 0xe5);
              if (*(byte *)(*wlc + 0xe5) != 0) {
                uStack140 = func_0x0086b424(wlc[0x1b9],scb,(uint)uStack48);
              }
            }
          }
        }
        else {
LAB_0000a3a6:
          uVar26 = 0;
          uVar19 = rspec_override;
          uStack140 = uVar26;
        }
      }
    }
  }
  else {
    uStack140 = 0;
    uVar26 = 0;
    rspec_override = rspec_override;
    uVar19 = rspec_override;
  }
  iVar10 = wlc[0x11d];
  cVar4 = *(char *)(iVar10 + 0xc);
  pcVar31 = (code *)((uint)*(byte *)(*wlc + 0x4f) & 3);
  if ((*(byte *)(*wlc + 0x4f) & 3) == 0) {
    uVar30 = uVar19 & 0xfff8ffff | 0x10000;
    uVar16 = rspec_override & 0xfff8ffff | 0x10000;
    pcStack152 = pcVar31;
    pcStack132 = pcVar31;
    pcStack128 = pcVar31;
  }
  else {
    if (*(byte *)(iVar10 + 2) < 2) {
      bVar34 = false;
LAB_0000a41c:
      cVar15 = '\0';
    }
    else {
      bVar34 = *(char *)(wlc[8] + 0x4d) == '\x01';
      if (*(char *)(wlc[8] + 0x4d) != -1) goto LAB_0000a41c;
      cVar15 = '\0';
      if ((*(uint *)(scb + 4) & 0x40000000) != 0) {
        iVar10 = func_0x00804858(iVar10 + 0xe,2);
        cVar15 = (char)iVar10;
        if (iVar10 != 0) {
          cVar15 = '\x01';
        }
      }
    }
    uVar16 = uVar19 & 0xff;
    if ((uVar19 & 0x3000000) == 0) {
      if (*(char *)(DAT_0000a6bc + uVar16) < '\0') goto LAB_0000a442;
    }
    else {
      if (0x1f < uVar16) {
        if (uVar16 == 0x20) goto LAB_0000a442;
        uVar16 = uVar16 - 0x55;
      }
      if (uVar16 < 8) {
LAB_0000a442:
        if (-1 < (int)uVar19) {
          uVar16 = uVar19 & 0xffeffcff;
          if (((uVar19 & 0x3000000) == 0) ||
             ((!bVar34 && (((uVar19 & 0x3000000) != 0x1000000 || (cVar15 == '\0')))))) {
            uVar19 = uVar16;
            if (cVar4 == '\x01') {
              uVar19 = uVar16 | 0x100;
            }
          }
          else {
            uVar19 = uVar16 | 0x100000;
          }
        }
      }
    }
    uVar16 = rspec_override & 0xff;
    if ((rspec_override & 0x3000000) == 0) {
      if (*(char *)(DAT_0000a6bc + uVar16) < '\0') goto LAB_0000a494;
    }
    else {
      if (0x1f < uVar16) {
        if (uVar16 == 0x20) goto LAB_0000a494;
        uVar16 = uVar16 - 0x55;
      }
      if (uVar16 < 8) {
LAB_0000a494:
        if (-1 < (int)rspec_override) {
          uVar16 = rspec_override & 0xffeffcff;
          if (((rspec_override & 0x3000000) == 0) ||
             ((!bVar34 && (((rspec_override & 0x3000000) != 0x1000000 || (cVar15 == '\0')))))) {
            rspec_override = uVar16;
            if (cVar4 == '\x01') {
              rspec_override = uVar16 | 0x100;
            }
          }
          else {
            rspec_override = uVar16 | 0x100000;
          }
        }
      }
    }
    if ((*(ushort *)(wlc + 0x112) & 0x3800) == 0x1800) {
      uVar17 = uVar19 & 0x70000;
      if (uVar17 == 0) {
        if (((uVar19 & 0x3000000) == 0) || (-1 < *(int *)(scb + 4) << 0xc)) {
          uVar17 = 0x10000;
        }
        else {
          if ((*(char *)(*wlc + 0x3c) == '\0') || (*(char *)(iVar8 + 6) == '\0')) {
            uVar17 = 0x20000;
          }
          else {
            if ((*(ushort *)(*(int *)(iVar8 + 0xf4) + 0x32) & 0x3800) == 0x1800) {
              uVar17 = 0x20000;
            }
            else {
              uVar17 = 0x10000;
            }
          }
        }
      }
      if (((uVar19 & 0x3000000) != 0) && ((uVar19 & 0xff) == 0x20)) {
        uVar17 = 0x20000;
      }
    }
    else {
      if ((uVar19 & 0xff) == 0x20) {
        uVar19 = 0x1000000;
      }
      if ((rspec_override & 0xff) == 0x20) {
        rspec_override = 0x1000000;
      }
      uVar17 = 0x10000;
    }
    uVar16 = rspec_override & 0xfff8ffff;
    bVar34 = (rspec_override & 0x3000000) != 0;
    uVar30 = uVar19 & 0xfff8ffff | uVar17;
    if (bVar34) {
      uVar16 = uVar16 | uVar17;
    }
    if (!bVar34) {
      uVar16 = uVar16 | 0x10000;
    }
    if (((DAT_0000a6c0 & *(uint *)(wlc[8] + 0x44)) == 0) &&
       ((*(uint *)(wlc[8] + 0x44) & 0x3000000) != 0x1000000)) {
      if (((uVar19 & 0x3000000) == 0) || (*(char *)(wlc + 0x88) != '\x01')) {
        uVar18 = uVar30;
        if (*(char *)(wlc + 0x88) == '\0') {
          uVar18 = uVar19 & 0xff78ffff | uVar17;
        }
      }
      else {
        uVar18 = uVar30 | 0x800000;
      }
      if (((uVar16 & 0x3000000) == 0) || (*(char *)(wlc + 0x88) != '\x01')) {
        uVar19 = uVar16;
        if (*(char *)(wlc + 0x88) == '\0') {
          uVar19 = uVar16 & 0xff7fffff;
        }
      }
      else {
        uVar19 = uVar16 | 0x800000;
      }
      uVar30 = uVar18 & 0xffbfffff;
      uVar16 = uVar19 & 0xffbfffff;
      if ((*(char *)(wlc[0x11d] + 0x13) == '\x01') ||
         (((*(int *)(scb + 4) << 0xf < 0 && (*(int *)(scb + 8) << 0x18 < 0)) &&
          (*(char *)(wlc[0x11d] + 0x13) == -1)))) {
        if (((((uVar18 & 0x3000000) != 0) && (uVar18 = uVar18 & 0xff, 1 < uVar18 - 0x57)) &&
            (uVar18 != 99)) && (((uVar18 != 100 && (uVar18 != 0x65)) && (uVar18 != 0x66)))) {
          uVar30 = uVar30 | 0x400000;
        }
        if ((((((uVar19 & 0x3000000) != 0) && (uVar19 = uVar19 & 0xff, 1 < uVar19 - 0x57)) &&
             (uVar19 != 99)) && ((uVar19 != 100 && (uVar19 != 0x65)))) && (uVar19 != 0x66)) {
          uVar16 = uVar16 | 0x400000;
        }
      }
    }
    pcVar11 = (code *)func_0x0081fe30(wlc,scb);
    pcVar31 = (code *)(uVar30 & 0x3000000);
    pcStack152 = pcVar31;
    pcStack128 = pcVar31;
    if (pcVar31 != Reset) {
      if (cVar2 == '\x02') {
        pcVar31 = (code *)(uint)((uVar30 & 0x70000) == 0x20000);
      }
      else {
        pcVar31 = Reset;
      }
      uVar19 = uVar30 & 0xff;
      if (uVar19 < 0x20) {
        bVar34 = 7 < uVar19;
      }
      else {
        if (uVar19 == 0x20) {
          bVar34 = false;
        }
        else {
          bVar34 = 7 < uVar19 - 0x55;
        }
      }
      pcStack128 = (code *)(uint)bVar34;
      if (((bVar34 != false) &&
          (pcStack128 = (code *)(uint)*(byte *)(scb + 0x10e), *(byte *)(scb + 0x10e) != 0)) &&
         (pcStack128 = (code *)(uint)*(byte *)(scb + 0x10f), *(byte *)(scb + 0x10f) != 0)) {
        pcStack128 = Reset + 1;
      }
      pcStack152 = pcVar11;
      if ((uVar30 & 0x800000) != 0) {
        if (uVar19 < 0x20) {
          bVar34 = uVar19 < 8;
        }
        else {
          if (uVar19 == 0x20) {
            bVar34 = true;
          }
          else {
            bVar34 = uVar19 - 0x55 < 8;
          }
        }
        if (bVar34) {
          pcStack152 = UndefinedInstruction;
        }
      }
    }
    pcStack132 = (code *)(uVar16 & 0x3000000);
    if (((code *)(uVar16 & 0x3000000) != Reset) && (pcStack132 = pcVar11, (uVar16 & 0x800000) != 0))
    {
      uVar19 = uVar16 & 0xff;
      if (uVar19 < 0x20) {
        bVar34 = uVar19 < 8;
      }
      else {
        if (uVar19 == 0x20) {
          bVar34 = true;
        }
        else {
          bVar34 = uVar19 - 0x55 < 8;
        }
      }
      if (bVar34) {
        pcStack132 = UndefinedInstruction;
      }
    }
  }
  if (uVar26 != 0) {
    iVar10 = *(int *)(iVar8 + 0x11c);
    *(uint *)(iVar8 + (iVar10 + 0x24) * 8) = uVar30;
    *(uint *)(iVar8 + iVar10 * 8 + 0x124) = nfrag & 0xff;
    *(uint *)(iVar8 + 0x11c) = iVar10 + 1U & 0x3f;
    *(uint *)(scb + 0x230) = uVar16;
  }
  uVar26 = uVar30 & 0x3000000;
  if (uVar26 == 0) {
    pcStack88 = (code *)(uVar30 & 0xff);
  }
  else {
    pcStack88 = (code *)FUN_0002d044(uVar30);
  }
  if ((uVar24 == 2) || (uVar24 == 0)) {
    if (((int)(uint)*(ushort *)(wlc + 0x116) < iStack148) || (*(int *)(p + 0x18) << 5 < 0)) {
      pcStack108 = pcStack128;
      if ((*(byte *)(src + 1) & 1) == 0) {
        pcStack108 = Reset + 1;
      }
    }
    else {
      pcStack108 = pcStack128;
    }
  }
  else {
    pcStack108 = pcStack128;
  }
  if (((((*(char *)(wlc[8] + 0x15) != '\0') && (cVar1 != '\0')) && (uVar26 == 0)) &&
      (*(char *)(DAT_0000a9c8 + (uVar30 & 0xff)) < '\0')) ||
     ((((*(byte *)(*wlc + 0x4f) & 3) != 0 && (uVar26 != 0)) && (cVar2 != '\0')))) {
    if (nfrag < 2) {
      if ((*(char *)(wlc[8] + 0x15) != '\0') && (cVar1 != '\0')) {
        if (uVar26 == 0) {
          uVar26 = uVar30 & 0x7f;
          if ((((uVar26 != 2) && (uVar26 != 4)) && (uVar26 != 0xb)) && (uVar26 != 0x16)) {
            pcVar31 = Reset + 1;
          }
        }
        else {
          pcVar31 = Reset + 1;
        }
      }
    }
    else {
      if (cVar1 == '\0') {
        uVar30 = 0x30;
      }
      else {
        uVar30 = 0x16;
      }
      uVar30 = uVar30 | 0x10000;
      *(uint *)(p + 0x18) = *(uint *)(p + 0x18) & 0xf7ffffff;
      uVar16 = uVar30;
    }
  }
  pcVar11 = (code *)(uVar30 & 0x3000000);
  if (((pcVar11 == Reset) && ((uVar30 & 0x7f) < 0x17)) &&
     (((DAT_0000a9cc << (uVar30 & 0x7f) < 0 &&
       ((pcStack152 = pcVar22, pcVar22 != Reset && (pcStack152 = pcVar11, (uVar30 & 0xff) != 2))))
      && (pcStack152 = (code *)((int)*(char *)(*(int *)(scb + 0x10) + 0x118) + -1),
         pcStack152 != Reset)))) {
    pcStack152 = Reset + 1;
  }
  pcVar14 = (code *)(uVar16 & 0x3000000);
  if (((((pcVar14 == Reset) && ((uVar16 & 0x7f) < 0x17)) && (DAT_0000a9cc << (uVar16 & 0x7f) < 0))
      && ((pcStack132 = pcVar14, pcVar22 != Reset && ((uVar16 & 0xff) != 2)))) &&
     (pcStack132 = (code *)((int)*(char *)(*(int *)(scb + 0x10) + 0x118) + -1), pcStack132 != Reset)
     ) {
    pcStack132 = Reset + 1;
  }
  if (uVar24 == 2) {
    *(uint *)(scb + 0x168) = uVar30;
  }
  pcStack164 = (code *)(*(uint *)(scb + 4) & 0x10000);
  if (((pcStack164 != Reset) &&
      (pcStack164 = (code *)(uint)*(byte *)((int)wlc + 0x211), *(byte *)((int)wlc + 0x211) != 0)) &&
     (pcStack164 = (code *)(uint)*(byte *)((int)wlc + 0x215), *(byte *)((int)wlc + 0x215) != 0)) {
    if (cVar2 == '\x03') {
      pcStack164 = Reset;
    }
    else {
      if ((pcVar11 != Reset) ||
         ((((uVar26 = uVar30 & 0x7f, pcStack164 = pcVar11, uVar26 != 2 && (uVar26 != 4)) &&
           (uVar26 != 0xb)) && (uVar26 != 0x16)))) {
        pcStack164 = (code *)((uint)*(byte *)(src + 1) & 1);
        if ((*(byte *)(src + 1) & 1) == 0) {
          if (((uVar23 & 0xfc) == 0x88) && (queue < 4)) {
            *(uint *)(p + 0x18) = *(uint *)(p + 0x18) | 0x1000;
            if (bVar33) {
              iVar10 = 0x1e;
            }
            else {
              iVar10 = 0x18;
            }
            uStack180._0_2_ = (ushort)uStack180 | 0x5000;
            *(ushort *)((int)src + iVar10) = *(ushort *)((int)src + iVar10) & 0xff9f | 0x20;
            pcStack164 = Reset + 1;
          }
        }
        else {
          pcStack164 = Reset;
        }
      }
    }
  }
  func_0x00825654(wlc,uVar30,iStack148,uVar23,iVar27 + -6);
  func_0x00825654(wlc,uVar16,iStack148,uVar23,auStack64);
  memcpy((undefined4 *)(iVar27 + -0x40),auStack64,6);
  if (((pcVar14 == Reset) && ((uVar16 & 0x7f) < 0x17)) && (DAT_0000a9cc << (uVar16 & 0x7f) < 0)) {
    *(undefined *)(iVar27 + -0x3c) = (char)iStack148;
    *(undefined *)(iVar27 + -0x3b) = (char)((uint)iStack148 >> 8);
  }
  if ((int)(*(uint *)(p + 0x18) << 0x15) < 0) {
    if (pcVar11 != Reset) {
      if ((((key == 0) || (*(char *)(key + 8) == '\x04')) || (*(char *)(key + 8) == '\v')) &&
         (*(uint *)(p + 0x18) = *(uint *)(p + 0x18) | 0x800, *(char *)((int)wlc + 0x2be) != '\0')) {
        pcStack108 = Reset + 1;
      }
      goto LAB_0000a9d0;
    }
LAB_0000a9ae:
    if (-1 < *(char *)(DAT_0000a9c8 + (uVar30 & 0xff))) goto LAB_0000a9d0;
    uStack120 = (ushort)*(byte *)(iVar27 + -6) & 0xf;
  }
  else {
    if (pcVar11 == Reset) goto LAB_0000a9ae;
LAB_0000a9d0:
    uStack120 = (ushort)*(byte *)(iVar27 + -6);
  }
  if ((uVar28 == 0xa4) || ((int)((uint)*(byte *)(src + 1) << 0x1f) < 0)) {
    if (pcStack164 != Reset) goto LAB_0000aa0a;
LAB_0000aa20:
    if (uVar28 != 0xa4) goto LAB_0000aa30;
    *(undefined2 *)(iVar27 + -0x3a) = *(undefined2 *)((int)src + 2);
  }
  else {
    if (pcStack164 != Reset) {
LAB_0000aa0a:
      sVar6 = func_0x008251c8(wlc,uVar30,pcStack152,0x92a);
      *(short *)((int)src + 2) = sVar6 + 2;
      goto LAB_0000aa20;
    }
    if (*(int *)(p + 0x18) << 0x15 < 0) {
      uVar5 = func_0x0081d368();
    }
    else {
      uVar5 = func_0x00825608(wlc,uVar30,pcStack152,next_frag_len);
    }
    *(undefined2 *)((int)src + 2) = uVar5;
LAB_0000aa30:
    if (((int)((uint)*(byte *)(src + 1) << 0x1f) < 0) || (pcStack164 != Reset)) {
      *(undefined *)(iVar27 + -0x3a) = 0;
      *(undefined *)(iVar27 + -0x39) = 0;
    }
    else {
      if (*(int *)(p + 0x18) << 0x15 < 0) {
        uVar5 = func_0x0081d368();
      }
      else {
        uVar5 = func_0x00825608(wlc,uVar16,pcStack132,next_frag_len);
      }
      *(undefined2 *)(iVar27 + -0x3a) = uVar5;
    }
  }
  iVar10 = *(int *)(p + 0x18);
  if (iVar10 << 0x16 < 0) {
    *(undefined2 *)(iVar27 + -0x34) = *(undefined2 *)(p + 0x24);
    *(undefined2 *)(iVar27 + -0x32) = *(undefined2 *)(p + 0x26);
    uStack180._0_2_ = (ushort)uStack180 | 0x2000;
  }
  if (frag == 0) {
    uStack180._0_2_ = (ushort)uStack180 | 8;
  }
  if ((((-1 < (int)((uint)*(byte *)(src + 1) << 0x1f)) && (-1 < iVar10 << 0x13)) &&
      ((*(char *)(wlc + 0x85) == '\0' || (-1 < iVar10 << 0x19)))) &&
     (((iVar10 << 0x15 < 0 || (bVar20 == 0)) || (*(char *)(iVar13 + 0x28) == '\0')))) {
    uStack180._0_2_ = (ushort)uStack180 | 1;
  }
  uVar26 = (uint)*(byte *)(DAT_0000ad04 + queue);
  if ((((uVar24 == 2) && (queue < 4)) &&
      ((*(char *)((int)wlc + 0x211) != '\0' &&
       ((UndefinedInstruction < pcStack88 && (uVar24 = FUN_000141d6(wlc[0x4d]), uVar24 == 0)))))) &&
     (((*(short *)(iVar13 + uVar26 * 2 + 0x1c) == 0 || (*(int *)(p + 0x18) << 0x15 < 0)) &&
      (pcStack128 == Reset)))) {
    uStack180._0_2_ = (ushort)uStack180 | 0x1000;
  }
  uVar24 = FUN_00023688(*(int *)(wlc[8] + 0x10));
  if ((uVar24 & 0x3800) == 0x1800) {
    uStack180._0_2_ = (ushort)uStack180 | 0x100;
  }
  if (pcStack156 != Reset) {
    uStack180._0_2_ = (ushort)uStack180 | 0x8000;
  }
  *(ushort *)(iVar27 + -0x76) = (ushort)uStack180;
  if (key == 0) {
    uStack180 = key;
  }
  else {
    uStack180 = (uint)*(byte *)(wlc + 0xa5);
    if (*(byte *)(wlc + 0xa5) == 0) {
      uVar24 = *(uint *)(iVar8 + 0x50) & 8;
      if ((uVar24 == 0) &&
         ((*(char *)(key + 8) != '\v' ||
          ((uStack180 = (uint)*(byte *)(*wlc + 0xb1), *(byte *)(*wlc + 0xb1) != 0 &&
           (uStack180 = uVar24, *(byte *)(key + 6) < 8)))))) {
        if ((uint)*(byte *)(key + 6) < *(uint *)(*wlc + 0xbc)) {
          if (*(int *)(p + 0x18) << 2 < 0) {
            uStack180 = 0;
          }
          else {
            uStack180 = (uint)*(byte *)(key + 0xc) & 7 | (uint)*(byte *)(key + 6) << 4;
          }
        }
        else {
          uStack180 = 0;
        }
      }
    }
    else {
      uStack180 = 0;
    }
  }
  if (((uint)(pcStack132 + -1) & 0xff) < 2) {
    uStack180 = uStack180 & 0xffff | 0x2000;
  }
  memcpy((undefined4 *)(iVar27 + -0x72),src,2);
  *(undefined *)(iVar27 + -0x70) = 0;
  *(undefined *)(iVar27 + -0x6f) = 0;
  *(undefined *)(iVar27 + -0x4a) = 0;
  *(undefined *)(iVar27 + -0x49) = 0;
  if ((((key != 0) && (*(char *)(wlc + 0xa5) == '\0')) && (-1 < *(int *)(iVar8 + 0x50) << 0x1c)) &&
     ((((*(char *)(key + 8) != '\v' ||
        ((*(char *)(*wlc + 0xb1) != '\0' && (*(byte *)(key + 6) < 8)))) &&
       ((uint)*(byte *)(key + 6) < *(uint *)(*wlc + 0xbc))) && (-1 < *(int *)(p + 0x18) << 2)))) {
    if (bVar33) {
      dst = (undefined4 *)((int)src + 0x1e);
    }
    else {
      dst = src + 6;
    }
    if (bVar20 != 0) {
      dst = (undefined4 *)((int)dst + 2);
    }
    func_0x008303d8(wlc,iVar9,dst,key,0);
  }
  memcpy((undefined4 *)(iVar27 + -0x50),src + 1,6);
  *(ushort *)(iVar27 + -0x2a) = uStack48;
  sVar6 = func_0x0085d124(wlc[0x52],iVar8);
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
    if (pcVar31 != Reset) goto LAB_0000ac8c;
    func_0x00803b14(iVar27 + -0x1e,0,6);
    func_0x00803b14(iVar27 + -0x18,0,0x10);
    func_0x00803b14(iVar27 + -0x48,0,6);
    *(undefined *)(iVar27 + -0x42) = 0;
    *(undefined *)(iVar27 + -0x41) = 0;
    pcStack176 = pcVar31;
    pcStack172 = pcVar31;
    pcStack160 = pcVar31;
    pcStack156 = pcVar31;
    pcStack128 = pcVar31;
  }
  else {
    pcVar31 = Reset;
LAB_0000ac8c:
    pcStack176 = (code *)func_0x0082d238(iVar8,uVar30,0);
    pcStack172 = (code *)func_0x0082d238(iVar8,uVar16,0);
    pcVar22 = (code *)((uint)pcStack176 & 0x3000000);
    if (pcVar22 == Reset) {
      puVar12 = (undefined *)((uint)pcStack176 & 0xff);
      pcStack156 = pcVar22;
      if (-1 < (char)puVar12[DAT_0000ad08]) goto LAB_0000acc2;
    }
    else {
      puVar12 = FUN_0002d044((uint)pcStack176);
LAB_0000acc2:
      pcStack156 = (code *)(puVar12 + -2);
      if (pcStack156 != Reset) {
        pcStack156 = Reset + 1;
      }
      if (pcStack156 != Reset) {
        if (*(char *)(*(int *)(scb + 0x10) + 0x118) == '\x01') {
          pcStack156 = Reset;
        }
        else {
          uStack180 = uStack180 & 0xffff | 0x4000;
          pcStack156 = Reset + 1;
        }
      }
    }
    pcStack128 = (code *)((uint)pcStack172 & 0x3000000);
    if (pcStack128 == Reset) {
      puVar12 = (undefined *)((uint)pcStack172 & 0xff);
      if (-1 < (char)puVar12[DAT_0000ad08]) goto LAB_0000ad12;
    }
    else {
      puVar12 = FUN_0002d044((uint)pcStack172);
LAB_0000ad12:
      pcStack128 = (code *)(puVar12 + -2);
      if (pcStack128 != Reset) {
        pcStack128 = Reset + 1;
      }
      if (pcStack128 != Reset) {
        if (*(char *)(*(int *)(scb + 0x10) + 0x118) == '\x01') {
          pcStack128 = Reset;
        }
        else {
          uStack180._0_2_ = ~(~(ushort)((uStack180 << 0x11) >> 0x10) >> 1);
          pcStack128 = Reset + 1;
        }
      }
    }
    if (pcVar31 == Reset) {
      uVar28 = 6;
    }
    else {
      uVar28 = 0x800;
    }
    *(ushort *)(iVar27 + -0x76) = *(ushort *)(iVar27 + -0x76) | uVar28;
    if (pcVar31 == Reset) {
      uVar32 = 0x14;
    }
    else {
      uVar32 = 0xe;
    }
    func_0x00825654(wlc,pcStack176,uVar32,uVar23,iVar27 + -0x1e);
    func_0x00825654(wlc,pcStack172,uVar32,uVar23,auStack56);
    memcpy((undefined4 *)(iVar27 + -0x48),auStack56,6);
    pcStack160 = (code *)(iVar27 - 0x18);
    uVar5 = func_0x00825690(wlc,pcVar31,pcStack176,uVar30,pcStack156,pcStack152,iStack148,0);
    *(undefined2 *)(iVar27 + -0x16) = uVar5;
    uVar5 = func_0x00825690(wlc,pcVar31,pcStack172,uVar16,pcStack128,pcStack132,iStack148,0);
    *(undefined2 *)(iVar27 + -0x42) = uVar5;
    dst = (undefined4 *)(iVar27 + -0x14);
    if (pcVar31 == Reset) {
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
    if ((pcVar22 == Reset) && (*(char *)(DAT_0000b0b0 + ((uint)pcStack176 & 0xff)) < '\0')) {
      uVar23 = (uint)*(byte *)(iVar27 + -0x1e) & 0xf;
    }
    else {
      uVar23 = (uint)*(byte *)(iVar27 + -0x1e);
    }
    uStack120 = uStack120 | (ushort)(uVar23 << 8);
  }
  if ((*(int *)(p + 0x18) << 0x15 < 0) && (pcVar11 != Reset)) {
    auStack42[0] = 0;
    uVar23 = FUN_0001656e((int *)wlc[0x4d],scb,uVar30,iStack148,auStack42);
    *(undefined *)(iVar27 + -0x43) = (char)uVar23;
  }
  *(ushort *)(iVar27 + -0x74) = (ushort)uStack180;
  *(ushort *)(iVar27 + -100) = uStack120;
  if (pcVar14 == (code *)0x1000000) {
    uVar28 = 2;
  }
  else {
    if (((pcVar14 == Reset) && ((uVar16 & 0x7f) < 0x17)) && (DAT_0000b0b4 << (uVar16 & 0x7f) < 0)) {
      uVar28 = 0;
    }
    else {
      uVar28 = 1;
    }
  }
  uVar23 = (uint)pcStack176 & 0x3000000;
  if (uVar23 == 0x1000000) {
    uVar21 = 8;
  }
  else {
    if (((uVar23 != 0) || (0x16 < ((uint)pcStack176 & 0x7f))) ||
       (-1 < DAT_0000b0b4 << ((uint)pcStack176 & 0x7f))) {
      uVar23 = 1;
    }
    uVar21 = (ushort)((uVar23 & 0x3fff) << 2);
  }
  uVar23 = (uint)pcStack172 & 0x3000000;
  if (uVar23 == 0x1000000) {
    uVar29 = 0x20;
  }
  else {
    if (((uVar23 != 0) || (0x16 < ((uint)pcStack172 & 0x7f))) ||
       (-1 < DAT_0000b0b4 << ((uint)pcStack172 & 0x7f))) {
      uVar23 = 1;
    }
    uVar29 = (ushort)((uVar23 & 0xfff) << 4);
  }
  uVar23 = FUN_00023688(*(int *)(wlc[8] + 0x10));
  *(ushort *)(iVar27 + -0x62) = uVar29 | uVar21 | uVar28 | (ushort)((uVar23 & 0xff) << 8);
  if (pcVar11 == (code *)0x1000000) {
    uStack46 = 2;
  }
  else {
    if (((pcVar11 == Reset) && ((uVar30 & 0x7f) < 0x17)) && (DAT_0000b0b4 << (uVar30 & 0x7f) < 0)) {
      uStack46 = 0;
    }
    else {
      uStack46 = 1;
    }
  }
  if (((uint)(pcStack152 + -1) & 0xff) < 2) {
    uStack46 = uStack46 | 0x10;
    *(int *)(*(int *)(*wlc + 0x88) + 0x18) = *(int *)(*(int *)(*wlc + 0x88) + 0x18) + 1;
  }
  uVar24 = (uint)uStack46;
  uVar23 = FUN_00032126((int)wlc);
  uVar24 = uVar24 | uVar23 & 0xffff;
  uStack46 = (ushort)uVar24;
  if ((*(int *)(p + 0x18) << 4 < 0) && (*(char *)(*wlc + 0xe5) != '\0')) {
    FUN_0002481e(*(int *)(wlc[8] + 0x10),uVar24);
    FUN_00024830(*(int *)(wlc[8] + 0x10),&uStack46,uStack140,wlc[8]);
  }
  *(ushort *)(iVar27 + -0x6e) = uStack46;
  uVar5 = func_0x0082b630(wlc,uVar30,(uint)*(ushort *)(wlc + 0x112));
  *(undefined2 *)(iVar27 + -0x6c) = uVar5;
  uVar5 = func_0x0082b630(wlc,uVar16,(uint)*(ushort *)(wlc + 0x112));
  *(undefined2 *)(iVar27 + -0x6a) = uVar5;
  if ((pcStack108 != Reset) || (pcVar31 != Reset)) {
    uVar5 = func_0x0082b630(wlc,pcStack176,(uint)*(ushort *)(wlc + 0x112));
    *(undefined2 *)(iVar27 + -0x68) = uVar5;
    uVar5 = func_0x0082b630(wlc,pcStack172,(uint)*(ushort *)(wlc + 0x112));
    *(undefined2 *)(iVar27 + -0x66) = uVar5;
  }
  if ((pcVar11 != Reset) && (pcStack152 == UndefinedInstruction)) {
    uVar5 = func_0x00825310(wlc,uVar30,iStack148);
    *(undefined2 *)(iVar27 + -0x38) = uVar5;
  }
  if ((pcVar14 != Reset) && (pcStack132 == UndefinedInstruction)) {
    uVar5 = func_0x00825310(wlc,uVar16,iStack148);
    *(undefined2 *)(iVar27 + -0x36) = uVar5;
  }
  if ((-1 < *(int *)(scb + 4) << 0x19) || (bVar20 == 0)) goto LAB_0000b1b0;
  if (*(short *)(iVar13 + uVar26 * 2 + 0x1c) == 0) {
    if ((*(char *)(*wlc + 0x45) == '\0') || (3 < queue)) goto LAB_0000b1b0;
    uVar26 = (uint)*(byte *)(DAT_0000b228 + queue);
    if (pcStack160 == Reset) {
      iVar8 = func_0x008251c8(wlc,uVar30,pcStack152,iStack148);
      iVar13 = func_0x00825608(wlc,uVar30,pcStack152,0);
      uVar23 = iVar13 + iVar8;
    }
    else {
      iVar8 = func_0x0081d044(wlc,pcStack176,pcStack156);
      uVar23 = iVar8 + (uint)*(ushort *)(pcStack160 + 2);
    }
    iVar8 = wlc[0x59];
  }
  else {
    if ((*(int *)(p + 0x18) << 0x15 < 0) || (frag != 0)) goto LAB_0000b1b0;
    uVar24 = func_0x008251c8(wlc,uVar30,pcStack152,iStack148);
    if (pcStack160 == Reset) {
      if (pcStack164 == Reset) {
        iVar8 = func_0x00825608(wlc,uVar30,pcStack152,0);
        uVar23 = iVar8 + uVar24;
        sVar6 = func_0x008251c8(wlc,uVar16,pcStack132,iStack148);
        sVar7 = func_0x00825608(wlc,uVar16,pcStack132,0);
        goto LAB_0000b0e8;
      }
      sVar7 = 0;
      uVar23 = uVar24;
    }
    else {
      iVar8 = func_0x0081d044(wlc,pcStack176,pcStack156);
      sVar7 = func_0x0081d044(wlc,pcStack172,pcStack128);
      uVar23 = (uint)*(ushort *)(pcStack160 + 2) + iVar8;
      sVar6 = *(short *)(iVar27 + -0x42);
LAB_0000b0e8:
      sVar7 = sVar7 + sVar6;
    }
    *(undefined2 *)(iVar27 + -0x70) = (short)(uVar23 & 0xffff);
    *(short *)(iVar27 + -0x4a) = sVar7;
    if (-1 < (int)(((uVar24 + *(ushort *)(iVar13 + uVar26 * 2 + 0x1c)) - (uVar23 & 0xffff)) *
                  0x10000)) {
      uVar24 = func_0x0081d048(wlc,uVar30,pcStack152);
      if (uVar24 < 0x100) {
        uVar28 = 0x100;
      }
      else {
        uVar28 = *(ushort *)((int)wlc + 0x44a);
        if (uVar24 < *(ushort *)((int)wlc + 0x44a)) {
          uVar28 = (ushort)uVar24;
        }
      }
      if (*(ushort *)((int)wlc + queue * 2 + 0x44c) != uVar28) {
        func_0x0081e084(wlc);
      }
    }
    if ((*(char *)(*wlc + 0x45) == '\0') || (3 < queue)) goto LAB_0000b1b0;
    iVar8 = wlc[0x59];
  }
  func_0x0084bb90(iVar8,uVar26,uVar23,scb);
LAB_0000b1b0:
  *(uint *)(p + 0x18) = *(uint *)(p + 0x18) | 0x84;
  if ((((*(char *)(*wlc + 0x39) != '\0') && (*(int *)(scb + 0x10) != 0)) &&
      (*(int *)(*(int *)(scb + 0x10) + 0xcc) << 8 < 0)) &&
     ((*(char *)(scb + 0xe7) != '\0' && (iVar8 = func_0x00874cbc(wlc[0x4f]), iVar8 != 0)))) {
    if (bVar20 != 0) {
      if (bVar33) {
        iVar8 = 0x1e;
      }
      else {
        iVar8 = 0x18;
      }
      uVar25 = (uint)*(byte *)((int)src + iVar8) & 0xf;
    }
    func_0x008754fc(wlc[0x4f],scb,uVar25,uStack136);
  }
  return (uint)uStack48;
}


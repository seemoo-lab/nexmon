
void wlc_recvdata(uint *wlc,ushort **osh,int rxh,ushort *p)

{
  int *piVar1;
  uint uVar2;
  int **ppiVar3;
  undefined4 uVar4;
  undefined **ppuVar5;
  int iVar6;
  ushort *puVar7;
  ushort *puVar8;
  ushort uVar9;
  ushort uVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  int *piVar14;
  int iVar15;
  int **ppiVar16;
  int iVar17;
  byte bVar18;
  int *piVar19;
  uint local_d8;
  ushort *local_c8;
  ushort *local_c4;
  int local_c0;
  int local_bc;
  int local_b8;
  ushort *local_b4;
  int local_b0;
  ushort local_ac;
  undefined2 local_aa;
  undefined2 local_a8;
  undefined local_a6;
  uint local_a4;
  char local_a0;
  bool local_9f;
  byte local_9e;
  undefined local_9d;
  byte local_9c;
  byte local_9b;
  bool local_99;
  undefined local_98;
  undefined4 local_90;
  undefined4 local_6c;
  undefined local_68;
  ushort local_60;
  ushort local_4a;
  int local_48;
  undefined4 uStack68;
  undefined local_3e;
  int **local_34;
  int **local_30;
  int local_2c [2];
  
  bVar18 = 0;
  local_34 = (int **)0x0;
  local_30 = (int **)0x0;
  uVar10 = *(ushort *)(rxh + 0x16);
  local_90 = 0;
  local_a6 = 0;
  local_a4 = 0;
  local_9d = 0;
  local_a0 = '\0';
  local_68 = 0;
  local_9c = 0;
  local_99 = false;
  local_6c = 0;
  local_b4 = p;
  local_b0 = rxh;
  local_48 = rxh;
  if (p[6] < 0x22) {
    uVar11 = *wlc;
  }
  else {
    puVar7 = *(ushort **)(p + 4);
    piVar1 = (int *)FUN_0002d270(rxh,puVar7);
    *(int **)(p + 0x16) = piVar1;
    local_c8 = puVar7 + 3;
    local_ac = puVar7[3];
    uVar12 = (uint)local_ac;
    uVar13 = uVar12 & 0xf0;
    uVar11 = (uVar12 & 0xc) >> 2;
    local_9f = (uVar12 & 0x300) == 0x300;
    local_aa = (undefined2)uVar11;
    if (uVar11 == 2) {
      bVar18 = (byte)(uVar13 >> 7);
    }
    uVar11 = (uint)bVar18;
    local_a8 = (undefined2)(uVar13 >> 4);
    if (((*(ushort *)(rxh + 4) & 3) == 2) && ((short)local_ac < 0)) {
      uVar13 = uVar13 >> 7;
    }
    else {
      uVar13 = 0;
    }
    local_9d = (undefined)uVar13;
    if (local_9f) {
      uVar9 = 0x28;
    }
    else {
      uVar9 = 0x22;
    }
    if (bVar18 != 0) {
      uVar9 = uVar9 + 2;
    }
    if (uVar13 != 0) {
      uVar9 = uVar9 + 4;
    }
    local_9e = bVar18;
    if (uVar9 <= local_b4[6]) {
      local_9c = *(byte *)(puVar7 + 5) & 1;
      local_c4 = puVar7 + 0xf;
      if (local_9f) {
        local_c4 = puVar7 + 0x12;
      }
      local_9b = 0;
      if (bVar18 != 0) {
        uVar11 = (uint)*local_c4;
        local_9b = *(byte *)local_c4 >> 7;
      }
      uVar12 = FUN_00004744(osh,(int)p);
      *(int *)(local_b4 + 4) = *(int *)(local_b4 + 4) + 6;
      local_b4[6] = local_b4[6] - 6;
      puVar8 = local_b4;
      if (local_9b != 0) {
        puVar8 = (ushort *)func_0x008049a8(osh);
      }
      func_0x008086ec(osh,puVar8);
      uVar13 = (uint)((uVar10 & 0xc000) != 0);
      local_98 = 0;
      local_30 = (int **)0x0;
      if (local_9f == false) {
        if ((local_ac & 0x300) == 0) {
          puVar8 = local_c8 + 8;
          local_34 = (int **)func_0x0086b3ac(wlc,local_c8 + 5,uVar13,&local_30);
        }
        else {
          if ((int)((uint)local_ac << 0x17) < 0) {
            puVar8 = local_c8 + 2;
          }
          else {
            puVar8 = local_c8 + 5;
          }
        }
        if (local_30 == (int **)0x0) {
          local_30 = (int **)FUN_0001d28e((int)wlc,puVar8);
        }
        local_98 = SUB41(local_30,0);
        if (local_30 != (int **)0x0) {
          local_98 = 1;
        }
      }
      ppiVar16 = local_30;
      if (local_30 != (int **)0x0) {
        ppiVar16 = (int **)(Reset + 1);
      }
      if (local_9c == 0) {
        uVar2 = FUN_0001d23a((int)wlc,local_c8 + 2);
        if (uVar2 != 0) {
          uVar2 = 1;
        }
      }
      else {
        uVar2 = 0;
      }
      local_d8 = uVar2;
      if ((wlc[0x82] == 0) && (*(char *)(*wlc + 0x2c) == '\0')) {
        if (local_9c == 0) {
          if (uVar2 == 0) goto LAB_00012bac;
        }
        else {
          if (uVar2 == 0) goto LAB_0001256a;
        }
LAB_00012556:
        local_d8 = (uint)local_9f;
        if (local_9f != false) {
          local_d8 = 0;
          goto LAB_0001256a;
        }
        if (ppiVar16 != (int **)0x0) {
          ppiVar16 = (int **)(Reset + 1);
          goto LAB_0001256a;
        }
      }
      else {
        if (*(char *)(*wlc + 0x3f) == '\0') {
          ppiVar16 = (int **)0x0;
        }
        if (uVar2 != 0) goto LAB_00012556;
        if ((((int)((uint)local_ac << 0x17) < 0) || (local_9c == 0)) || (ppiVar16 == (int **)0x0)) {
          if ((local_9f != false) && (local_9c != 0)) {
            local_d8 = 0;
            goto LAB_0001256a;
          }
          if (*(char *)(*wlc + 0x2c) != '\0') {
            if (ppiVar16 == (int **)0x0) {
              func_0x00858ff4(wlc,rxh);
            }
            else {
              if (*(code *)((int)local_30 + 6) == (code)0x0) {
                ppiVar16 = (int **)(Reset + 1);
                local_d8 = 1;
                goto LAB_0001256a;
              }
            }
          }
          goto LAB_00012bac;
        }
        ppiVar16 = (int **)(Reset + 1);
LAB_0001256a:
        if (local_9e != 0) {
          if (local_9b != 0) {
            if (((local_30 != (int **)0x0) && (*(code *)((int)local_30 + 0x325) != (code)0x0)) ||
               (*(char *)((int)wlc + 0x212) == '\0')) goto LAB_00012bac;
            *(uint *)(local_b4 + 0xc) = *(uint *)(local_b4 + 0xc) | 0x40;
          }
          local_a4 = (uint)*(byte *)(DAT_000127dc + (uint)*(byte *)(DAT_000127d8 + (uVar11 & 7)));
          local_a6 = (undefined)(uVar11 & 7);
          local_c4 = local_c4 + 1;
          local_a0 = (char)((int)(uVar11 & 0x10) >> 4);
          local_68 = local_a6;
        }
        uVar11 = (uint)local_b4[7] & 0x2000;
        if ((local_b4[7] & 0x2000) != 0) {
          uVar11 = (uint)local_b4[0x28];
        }
        local_bc = local_b4[6] + uVar11;
        puVar8 = (ushort *)((int)local_c4 - *(int *)(local_b4 + 4));
        local_c0 = local_bc - (int)puVar8;
        local_b8 = FUN_00004744(osh,(int)p);
        local_b8 = local_b8 - (int)puVar8;
        local_4a = local_c8[0xb];
        if (local_d8 != 0) {
          if ((local_ac & 0x300) == 0) {
            if (*(code *)(local_30 + 4) == (code)0x0) {
LAB_00012766:
              local_34 = thunk_FUN_00030630((int)wlc,(int)local_30,local_c8 + 5,uVar13);
              if (local_34 != (int **)0x0) goto LAB_0001277e;
              goto LAB_00012bac;
            }
          }
          else {
            if ((local_9f != false) || (*(code *)(local_30 + 4) != (code)0x0)) goto LAB_00012766;
          }
          goto LAB_000127c8;
        }
        ppiVar3 = FUN_00010048((int *)wlc,(int **)&local_30,local_c8,rxh,(int **)&local_34,local_bc)
        ;
        if (ppiVar3 == (int **)0x0) {
          if ((local_ac & 0x300) == 0) {
            if (local_30 != (int **)0x0) {
              local_34 = thunk_FUN_000304f4((int)wlc,(int)local_30,local_c8 + 5,uVar13);
              if ((local_34 == (int **)0x0) &&
                 (uVar11 = local_d8, *(code *)(local_30 + 4) != (code)0x0)) {
                do {
                  iVar6 = *(int *)(wlc[0x9a] + uVar11);
                  if ((iVar6 != 0) &&
                     ((((*(char *)(iVar6 + 6) == '\0' && (*(char *)(iVar6 + 8) != '\0')) &&
                       (-1 < *(int *)(iVar6 + 0xcc) << 8)) &&
                      ((iVar15 = FUN_0000236a((int)(local_c8 + 8),*(int *)(iVar6 + 0xf4),6),
                       iVar15 == 0 && (*(char *)(iVar6 + 0x10) != '\0')))))) {
                    if (*(char *)(*wlc + 0x39) != '\0') {
                      func_0x00803b14(&uStack68,0,0x10);
                      memcpy(&uStack68,(undefined4 *)(local_c8 + 5),6);
                      local_3e = 1;
                      FUN_00009350(wlc,DAT_000127e0,(undefined4 *)0x0,0,local_2c,4,'\0',
                                   *(undefined4 *)(iVar6 + 0xc));
                      if (local_2c[0] != 0) {
                        FUN_00009350(wlc,DAT_000127e4,(undefined4 *)0x0,0,&uStack68,0x10,'\x01',
                                     *(undefined4 *)(iVar6 + 0xc));
                      }
                    }
                    goto LAB_000126fe;
                  }
                  uVar11 = uVar11 + 4;
                } while (uVar11 != 0x20);
              }
              if (local_34 == (int **)0x0) {
                local_34 = thunk_FUN_00030630((int)wlc,(int)local_30,local_c8 + 5,uVar13);
                if (local_34 == (int **)0x0) {
LAB_000126fe:
                  *(int *)(*(int *)(*wlc + 0x88) + 0x6c) =
                       *(int *)(*(int *)(*wlc + 0x88) + 0x6c) + 1;
                  goto LAB_00012bac;
                }
                func_0x0086b180(wlc,local_34);
              }
              local_99 = ((uint)local_30[0x33] & 0x800000) != 0;
              if ((!local_99) ||
                 ((ppiVar16 != (int **)0x0 &&
                  (iVar6 = func_0x00875318(wlc[0x4f],local_34), iVar6 == 0)))) goto LAB_00012c08;
            }
          }
          else {
LAB_00012c08:
            if (local_30 == (int **)0x0) {
              local_30 = (int **)local_34[4];
            }
LAB_0001277e:
            if (uVar2 != 0) {
              local_34[10] = *(int **)(*wlc + 0x28);
            }
            piVar19 = local_30[0x3f];
            if ((local_d8 != 0) || (local_9f != false)) goto LAB_000127e8;
            if ((local_9c == 0) || (-1 < (int)((uint)local_ac << 0x17))) {
              if (*(code *)((int)local_30 + 6) == (code)0x0) {
                if (*(code *)(local_30 + 4) == (code)0x0) {
                  uVar11 = 0;
                }
                else {
                  uVar11 = 0x200;
                }
                if (((uint)local_ac & 0x300) == uVar11) goto LAB_000127e8;
              }
              else {
                if ((int)((uint)local_ac << 0x17) < 0) {
LAB_000127e8:
                  iVar6 = func_0x00858ff4(wlc,rxh);
                  *(undefined *)((int)p + 0x23) = (char)iVar6;
                  *(undefined *)(p + 0x11) = 0;
                  if (((*(code *)((int)local_30 + 6) == (code)0x0) && (local_99 == false)) &&
                     (iVar6 != 0)) {
                    func_0x008590b4(local_30,iVar6,(int)*(char *)(rxh + 0x1d),uVar2);
                    uVar4 = func_0x00858f3c(wlc,rxh,wlc[0x1ab]);
                    *(char *)(p + 0x11) = (char)uVar4;
                    func_0x00859150(local_30,uVar4,uVar2);
                  }
                  if ((((*(char *)((int)local_34 + 0x1a2) != '\0') || (local_99 != false)) ||
                      (local_34[0x41] != (int *)0x0)) && (iVar6 != 0)) {
                    local_34[0x3f][(int)local_34[0x40]] = iVar6;
                    uVar11 = wlc[0x11d];
                    bVar18 = *(byte *)(uVar11 + 5);
                    piVar14 = (int *)((uint)bVar18 & 1);
                    if ((bVar18 & 1) != 0) {
                      piVar14 = (int *)(int)*(char *)(rxh + 0x1f);
                    }
                    local_34[(int)local_34[0x40] + 0x6b] = piVar14;
                    piVar14 = (int *)((int)(uint)*(byte *)(uVar11 + 5) >> 1 & 1);
                    if (piVar14 != (int *)0x0) {
                      piVar14 = (int *)(int)*(char *)(rxh + 0x20);
                    }
                    local_34[(int)local_34[0x40] + 0x73] = piVar14;
                    local_34[0x40] = (int *)((int)local_34[0x40] + 1U & 7);
                  }
                  if (((*(char *)(*wlc + 0x39) != '\0') && (local_30 != (int **)0x0)) &&
                     (((int)local_30[0x33] << 8 < 0 &&
                      ((local_30[0xd2] != (int *)0x0 &&
                       ((int)((uint)*(byte *)((int)local_30[0xd2] + 0xe) << 0x1f) < 0)))))) {
                    FUN_000199d0((int *)wlc,local_c8 + 5,((uint)local_ac << 0x13) >> 0x1f);
                  }
                  if (local_9f == false) {
                    if ((*(code *)((int)local_30 + 6) == (code)0x0) &&
                       ((local_30[0xd2] == (int *)0x0 ||
                        (-1 < (int)((uint)*(byte *)((int)local_30[0xd2] + 0xe) << 0x1f))))) {
                      local_60 = *(ushort *)(local_34 + 0x11);
                      if ((*(code *)(local_30 + 4) == (code)0x0) &&
                         (uVar10 = *(ushort *)((int)local_30 + 0x5a),
                         (int)((uint)uVar10 << 0x1f) < 0)) goto LAB_00012980;
                    }
                    else {
                      local_60 = *(ushort *)(local_34 + 0x11);
                      if ((((int)local_34[1] << 0x19 < 0) &&
                          ((((local_9e != 0 && ((local_4a & 0xf) == 0)) &&
                            (*(char *)((int)local_34 + 0xe7) != '\0')) &&
                           ((-1 < (int)((uint)*(byte *)(local_34 + 5) << 0x1f) &&
                            ((int)((uint)local_ac << 0x13) < 0)))))) &&
                         ((((int)local_34[1] << 0xe < 0 &&
                           (((int)(uint)*(byte *)((int)local_34 + 0xd9) >> local_a4) << 0x1f < 0))
                          && ((((local_30[0xd2] == (int *)0x0 ||
                                (-1 < (int)((uint)*(byte *)((int)local_30[0xd2] + 0xe) << 0x1f))) ||
                               (local_a0 == '\0')) ||
                              (uVar11 = FUN_00019d98((int)wlc,(int)local_34), uVar11 != 0)))))) {
                        func_0x0083addc(wlc,local_34,local_a4);
                      }
                      if ((local_ac & 0x1000) == 0) {
                        piVar14 = (int *)((uint)local_34[1] & 0xfffdffff);
                      }
                      else {
                        piVar14 = (int *)((uint)local_34[1] | 0x20000);
                      }
                      local_34[1] = piVar14;
                    }
                  }
                  else {
                    uVar10 = *(ushort *)((int)local_30 + 0x5a);
LAB_00012980:
                    local_60 = uVar10;
                  }
                  if ((((*(code *)((int)local_30 + 6) == (code)0x0) &&
                       (*(code *)(local_30 + 4) != (code)0x0)) && (local_9c == 0)) &&
                     (local_d8 == 0)) {
                    *(undefined *)((int)piVar19 + 6) = 0;
                  }
                  if ((local_9c == 0) ||
                     (((*(code *)((int)local_30 + 6) == (code)0x0 &&
                       ((*(code *)(local_30 + 4) == (code)0x0 ||
                        (iVar6 = FUN_0000236a((int)(local_c8 + 8),(int)((int)local_30 + 0xc2),6),
                        iVar6 != 0)))) &&
                      ((iVar6 = FUN_000047bc((int)(local_c8 + 2)), iVar6 != 0 ||
                       ((*(code *)((int)local_30 + 0x45) != (code)0x0 ||
                        (iVar6 = func_0x00847bd8(local_30,local_c8 + 2), iVar6 == 0)))))))) {
                    *(int **)(p + 0x16) = piVar1;
                    p[0xe] = local_4a;
                    if ((*(byte *)(local_c8 + 8) & 1) == 0) {
                      local_34[0x58] = (int *)((int)local_34[0x58] + 1);
                      ppiVar16 = local_34 + 100;
                      piVar19 = *ppiVar16;
                      piVar14 = local_34[0x65];
                    }
                    else {
                      local_34[0x59] = (int *)((int)local_34[0x59] + 1);
                      ppiVar16 = local_34 + 0x66;
                      piVar19 = *ppiVar16;
                      piVar14 = local_34[0x67];
                    }
                    *ppiVar16 = (int *)((int)piVar19 + uVar12);
                    ppiVar16[1] = (int *)((int)piVar14 + (uint)CARRY4((uint)piVar19,uVar12));
                    if ((local_9c == 0) &&
                       ((byte)(*(byte *)(puVar7 + 1) | *(byte *)((int)puVar7 + 1) | *(byte *)puVar7)
                        != 0)) {
                      FUN_000307a8(wlc[0x58],*(uint *)(p + 0x16),(uint)*(ushort *)(rxh + 0x12));
                      local_34[0x5b] = piVar1;
                    }
                    if (*(char *)((int)puVar7 + 3) < '\0') {
                      *(int *)(*(int *)(*wlc + 0x88) + 0x2a8) =
                           *(int *)(*(int *)(*wlc + 0x88) + 0x2a8) + 1;
                    }
                    if ((*(byte *)((int)puVar7 + 3) & 0x30) != 0) {
                      puVar7 = (ushort *)(*(int *)(*(int *)(*wlc + 0x88) + 0x2b0) + 1);
                      *(ushort **)(*(int *)(*wlc + 0x88) + 0x2b0) = puVar7;
                    }
                    if (local_9c == 0) {
                      uVar11 = *(uint *)(p + 0x16);
                      iVar6 = *(int *)(*wlc + 0x88);
                      if ((uVar11 & 0x3000000) == 0) {
                        ppuVar5 = (undefined **)(uVar11 & 0xff);
                      }
                      else {
                        ppuVar5 = (undefined **)FUN_0002d044(uVar11);
                      }
                      if (ppuVar5 == (undefined **)&DAT_00000016) {
                        *(int *)(iVar6 + 0x24c) = *(int *)(iVar6 + 0x24c) + 1;
                      }
                      else {
                        if (&DAT_00000016 < ppuVar5) {
                          if (ppuVar5 == (undefined **)0x30) {
                            *(int *)(iVar6 + 600) = *(int *)(iVar6 + 600) + 1;
                          }
                          else {
                            if (ppuVar5 < (undefined **)0x31) {
                              if (ppuVar5 == &IRQ) {
                                *(int *)(iVar6 + 0x250) = *(int *)(iVar6 + 0x250) + 1;
                              }
                              else {
                                if (ppuVar5 == (undefined **)0x24) {
                                  *(int *)(iVar6 + 0x254) = *(int *)(iVar6 + 0x254) + 1;
                                }
                              }
                            }
                            else {
                              if (ppuVar5 == (undefined **)0x60) {
                                *(int *)(iVar6 + 0x260) = *(int *)(iVar6 + 0x260) + 1;
                              }
                              else {
                                if (ppuVar5 == (undefined **)0x6c) {
                                  *(int *)(iVar6 + 0x264) = *(int *)(iVar6 + 0x264) + 1;
                                }
                                else {
                                  if (ppuVar5 == (undefined **)0x48) {
                                    *(int *)(iVar6 + 0x25c) = *(int *)(iVar6 + 0x25c) + 1;
                                  }
                                }
                              }
                            }
                          }
                        }
                        else {
                          if (ppuVar5 == (undefined **)((int)&SupervisorCall + 3)) {
                            *(int *)(iVar6 + 0x240) = *(int *)(iVar6 + 0x240) + 1;
                          }
                          else {
                            if ((undefined **)((int)&SupervisorCall + 3) < ppuVar5) {
                              if (ppuVar5 == (undefined **)&PrefetchAbort) {
                                *(int *)(iVar6 + 0x244) = *(int *)(iVar6 + 0x244) + 1;
                              }
                              else {
                                if (ppuVar5 == (undefined **)((int)&DataAbort + 2)) {
                                  *(int *)(iVar6 + 0x248) = *(int *)(iVar6 + 0x248) + 1;
                                }
                              }
                            }
                            else {
                              if (ppuVar5 == (undefined **)0x2) {
                                *(int *)(iVar6 + 0x238) = *(int *)(iVar6 + 0x238) + 1;
                              }
                              else {
                                if (ppuVar5 == (undefined **)UndefinedInstruction) {
                                  *(int *)(iVar6 + 0x23c) = *(int *)(iVar6 + 0x23c) + 1;
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                    if (local_9c != 0) {
                      FUN_00009190(local_30,(int)&local_c8,puVar7,(uint)local_9c);
                    }
                    if ((((int)local_34[1] << 0xd < 0) && (local_9c == 0)) && (local_d8 == 0)) {
                      FUN_000174f8((int **)wlc[0x4e],local_34,(int *)&local_c8);
                      return;
                    }
                    FUN_00011bd0((int *)wlc,(int)local_34,(int *)&local_c8);
                    return;
                  }
                  goto LAB_00012bac;
                }
              }
            }
LAB_000127c8:
            *(int *)(*(int *)(*wlc + 0x88) + 0x58) = *(int *)(*(int *)(*wlc + 0x88) + 0x58) + 1;
          }
        }
      }
LAB_00012bac:
      uVar11 = *wlc;
      if ((*(int *)(uVar11 + 0x34) != 0) && (local_9c == 0)) {
        iVar15 = *(int *)(uVar11 + 0x8c) +
                 ((uint)*(byte *)(DAT_00012c1c +
                                 (uint)*(byte *)(DAT_00012c18 + ((uint)local_b4[7] & 7))) + 0xc) * 8
        ;
        iVar17 = *(int *)(iVar15 + 8);
        uVar11 = *(int *)(iVar15 + 4) + 1;
        *(uint *)(iVar15 + 4) = uVar11;
        iVar6 = FUN_00004744(osh,(int)local_b4);
        *(int *)(iVar15 + 8) = iVar6 + iVar17;
      }
      goto LAB_00012be4;
    }
    uVar11 = *wlc;
  }
  uVar11 = *(uint *)(uVar11 + 0x88);
  *(int *)(uVar11 + 100) = *(int *)(uVar11 + 100) + 1;
LAB_00012be4:
  pkt_buf_free_skb(osh,local_b4,0,uVar11);
  return;
}


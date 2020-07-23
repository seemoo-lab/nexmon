
void wlc_recvdata(int *wlc,ushort **osh,int rxh,ushort *p)

{
  int *piVar1;
  undefined4 uVar2;
  code *pcVar3;
  undefined *puVar4;
  int iVar5;
  ushort *puVar6;
  uint uVar7;
  uint uVar8;
  int *piVar9;
  int *piVar10;
  ushort uVar11;
  ushort uVar12;
  int iVar13;
  uint uVar14;
  uint uVar15;
  uint uVar16;
  int *piVar17;
  int iVar18;
  int iVar19;
  byte bVar20;
  int **ppiVar21;
  ushort *puVar22;
  bool bVar23;
  uint local_c4;
  ushort *local_b4;
  ushort *local_b0;
  int local_ac;
  int local_a8;
  int local_a4;
  ushort *local_a0;
  int local_9c;
  ushort local_98;
  undefined2 local_96;
  undefined2 local_94;
  undefined local_92;
  uint local_90;
  char local_8c;
  bool local_8b;
  byte local_8a;
  undefined local_89;
  byte local_88;
  byte local_87;
  byte local_85;
  undefined local_84;
  undefined4 local_7c;
  undefined4 local_58;
  undefined local_54;
  ushort local_4c;
  ushort local_36;
  int local_34;
  int **local_30;
  int **local_2c [2];
  
  bVar20 = 0;
  local_30 = (int **)0x0;
  local_2c[0] = (int **)0x0;
  uVar12 = *(ushort *)(rxh + 0x16);
  local_7c = 0;
  local_92 = 0;
  local_90 = 0;
  local_89 = 0;
  local_8c = '\0';
  local_54 = 0;
  local_88 = 0;
  local_85 = 0;
  local_58 = 0;
  local_a0 = p;
  local_9c = rxh;
  local_34 = rxh;
  if (p[6] < 0x22) {
    iVar13 = *wlc;
  }
  else {
    puVar22 = *(ushort **)(p + 4);
    piVar1 = (int *)FUN_00038c04(rxh,puVar22);
    local_b4 = puVar22 + 3;
    *(int **)(p + 0x16) = piVar1;
    local_98 = puVar22[3];
    uVar14 = (uint)local_98;
    uVar15 = uVar14 & 0xf0;
    uVar7 = (uVar14 & 0xc) >> 2;
    local_8b = (uVar14 & 0x300) == 0x300;
    if (uVar7 == 2) {
      bVar20 = (byte)(uVar15 >> 7);
    }
    local_96 = (undefined2)uVar7;
    local_c4 = (uint)bVar20;
    local_94 = (undefined2)(uVar15 >> 4);
    if (((*(ushort *)(rxh + 4) & 3) == 2) && ((short)local_98 < 0)) {
      uVar15 = uVar15 >> 7;
    }
    else {
      uVar15 = 0;
    }
    local_89 = (undefined)uVar15;
    if (local_8b) {
      uVar11 = 0x28;
    }
    else {
      uVar11 = 0x22;
    }
    if (bVar20 != 0) {
      uVar11 = uVar11 + 2;
    }
    if (uVar15 != 0) {
      uVar11 = uVar11 + 4;
    }
    local_8a = bVar20;
    if (uVar11 <= local_a0[6]) {
      local_88 = *(byte *)(puVar22 + 5) & 1;
      local_b0 = puVar22 + 0xf;
      if (local_8b) {
        local_b0 = puVar22 + 0x12;
      }
      local_87 = 0;
      if (bVar20 != 0) {
        local_c4 = (uint)*local_b0;
        local_87 = *(byte *)local_b0 >> 7;
      }
      uVar7 = FUN_00009338(osh,(int)p);
      *(int *)(local_a0 + 4) = *(int *)(local_a0 + 4) + 6;
      local_a0[6] = local_a0[6] - 6;
      puVar6 = local_a0;
      if (local_87 != 0) {
        puVar6 = (ushort *)func_0x008049a8(osh);
      }
      func_0x008086ec(osh,puVar6,4);
      uVar14 = (uint)((uVar12 & 0xc000) != 0);
      local_84 = 0;
      local_2c[0] = (int **)0x0;
      if (local_8b == false) {
        if ((local_98 & 0x300) == 0) {
          puVar6 = local_b4 + 8;
          local_30 = (int **)func_0x0086b3ac(wlc,local_b4 + 5,uVar14,local_2c);
        }
        else {
          if ((int)((uint)local_98 << 0x17) < 0) {
            puVar6 = local_b4 + 2;
          }
          else {
            puVar6 = local_b4 + 5;
          }
        }
        if (local_2c[0] == (int **)0x0) {
          local_2c[0] = (int **)FUN_00026794((int)wlc,puVar6);
        }
        local_84 = SUB41(local_2c[0],0);
        if (local_2c[0] != (int **)0x0) {
          local_84 = 1;
        }
      }
      ppiVar21 = local_2c[0];
      if (local_2c[0] != (int **)0x0) {
        ppiVar21 = (int **)(Reset + 1);
      }
      if (local_88 == 0) {
        uVar15 = FUN_00026740((int)wlc,local_b4 + 2);
        if (uVar15 != 0) {
          uVar15 = 1;
        }
      }
      else {
        uVar15 = 0;
      }
      uVar16 = uVar15;
      if ((wlc[0x82] == 0) && (*(char *)(*wlc + 0x2c) == '\0')) {
        if (local_88 != 0) {
          if (uVar15 != 0) goto LAB_00018c94;
          goto LAB_00018caa;
        }
        if (uVar15 == 0) goto LAB_00019292;
LAB_00018c94:
        uVar16 = (uint)local_8b;
        if (local_8b != false) goto LAB_00018ca6;
        if (ppiVar21 != (int **)0x0) goto LAB_00018caa;
      }
      else {
        if (*(char *)(*wlc + 0x3f) == '\0') {
          ppiVar21 = (int **)0x0;
        }
        if (uVar15 != 0) goto LAB_00018c94;
        if (((-1 < (int)((uint)local_98 << 0x17)) && (local_88 != 0)) && (ppiVar21 != (int **)0x0))
        goto LAB_00018caa;
        if ((local_8b == false) || (local_88 == 0)) {
          if (*(char *)(*wlc + 0x2c) != '\0') {
            if (ppiVar21 == (int **)0x0) {
              func_0x00858ff4(wlc,rxh);
            }
            else {
              if (*(code *)((int)local_2c[0] + 6) == (code)0x0) {
                uVar16 = 1;
                goto LAB_00018caa;
              }
            }
          }
          goto LAB_00019292;
        }
LAB_00018ca6:
        uVar16 = 0;
LAB_00018caa:
        if (local_8a != 0) {
          if (local_87 != 0) {
            if (((local_2c[0] != (int **)0x0) && (*(code *)((int)local_2c[0] + 0x325) != (code)0x0))
               || (*(char *)((int)wlc + 0x212) == '\0')) goto LAB_00019292;
            *(uint *)(local_a0 + 0xc) = *(uint *)(local_a0 + 0xc) | 0x40;
          }
          local_92 = (undefined)(local_c4 & 7);
          local_90 = (uint)*(byte *)(DAT_00018e98 + (uint)*(byte *)(DAT_00018e9c + (local_c4 & 7)));
          local_8c = (char)((int)(local_c4 & 0x10) >> 4);
          local_b0 = local_b0 + 1;
          local_54 = local_92;
        }
        uVar8 = (uint)local_a0[7] & 0x2000;
        if ((local_a0[7] & 0x2000) != 0) {
          uVar8 = (uint)local_a0[0x28];
        }
        local_a8 = local_a0[6] + uVar8;
        puVar6 = (ushort *)((int)local_b0 - *(int *)(local_a0 + 4));
        local_ac = local_a8 - (int)puVar6;
        local_a4 = FUN_00009338(osh,(int)p);
        local_a4 = local_a4 - (int)puVar6;
        local_36 = local_b4[0xb];
        if (uVar16 != 0) {
          if ((local_98 & 0x300) == 0) {
            if (*(code *)(local_2c[0] + 4) == (code)0x0) {
LAB_00018e22:
              local_30 = thunk_FUN_0003d4f2(wlc,(int)local_2c[0],(undefined4 *)(local_b4 + 5),uVar14
                                           );
              if (local_30 != (int **)0x0) goto LAB_00018e3a;
              goto LAB_00019292;
            }
          }
          else {
            if ((local_8b != false) || (*(code *)(local_2c[0] + 4) != (code)0x0)) goto LAB_00018e22;
          }
          goto LAB_00018e88;
        }
        ppiVar21 = FUN_00015e84(wlc,(int **)local_2c,local_b4,rxh,(int **)&local_30,local_a8);
        if (ppiVar21 == (int **)0x0) {
          if ((local_98 & 0x300) == 0) {
            if (local_2c[0] == (int **)0x0) goto LAB_00019292;
            local_30 = thunk_FUN_0003d2e8((int)wlc,(int)local_2c[0],local_b4 + 5,uVar14);
            if ((local_30 == (int **)0x0) &&
               (uVar8 = uVar16, *(code *)(local_2c[0] + 4) != (code)0x0)) {
              do {
                iVar13 = *(int *)(wlc[0x9a] + uVar8);
                if ((iVar13 != 0) &&
                   ((((*(char *)(iVar13 + 6) == '\0' && (*(char *)(iVar13 + 8) != '\0')) &&
                     (-1 < *(int *)(iVar13 + 0xcc) << 8)) &&
                    ((iVar5 = FUN_000023ee((int)(local_b4 + 8),*(int *)(iVar13 + 0xf4),6),
                     iVar5 == 0 && (*(char *)(iVar13 + 0x10) != '\0')))))) goto LAB_00018ddc;
                uVar8 = uVar8 + 4;
              } while (uVar8 != 0x20);
            }
            if (local_30 == (int **)0x0) {
              local_30 = thunk_FUN_0003d4f2(wlc,(int)local_2c[0],(undefined4 *)(local_b4 + 5),uVar14
                                           );
              if (local_30 == (int **)0x0) {
LAB_00018ddc:
                *(int *)(*(int *)(*wlc + 0x88) + 0x6c) = *(int *)(*(int *)(*wlc + 0x88) + 0x6c) + 1;
                goto LAB_00019292;
              }
              func_0x0086b180(wlc,local_30);
            }
            local_85 = (byte)((uint)((int)local_2c[0][0x33] << 8) >> 0x1f);
          }
          if (local_2c[0] == (int **)0x0) {
            local_2c[0] = (int **)local_30[4];
          }
LAB_00018e3a:
          if (uVar15 != 0) {
            local_30[10] = *(int **)(*wlc + 0x28);
          }
          piVar9 = local_2c[0][0x3f];
          if ((uVar16 != 0) || (local_8b != false)) goto LAB_00018ea0;
          if ((local_88 == 0) || (-1 < (int)((uint)local_98 << 0x17))) {
            if (*(code *)((int)local_2c[0] + 6) == (code)0x0) {
              if (*(code *)(local_2c[0] + 4) == (code)0x0) {
                uVar14 = 0;
              }
              else {
                uVar14 = 0x200;
              }
              if (((uint)local_98 & 0x300) == uVar14) goto LAB_00018ea0;
            }
            else {
              if ((int)((uint)local_98 << 0x17) < 0) {
LAB_00018ea0:
                iVar13 = func_0x00858ff4(wlc,rxh);
                *(undefined *)((int)p + 0x23) = (char)iVar13;
                *(undefined *)(p + 0x11) = 0;
                if (((*(code *)((int)local_2c[0] + 6) == (code)0x0) && (local_85 == 0)) &&
                   (iVar13 != 0)) {
                  func_0x008590b4(local_2c[0],iVar13,(int)*(char *)(rxh + 0x1d),uVar15);
                  uVar2 = func_0x00858f3c(wlc,rxh,wlc[0x1ab]);
                  *(char *)(p + 0x11) = (char)uVar2;
                  func_0x00859150(local_2c[0],uVar2,uVar15);
                }
                if ((((*(char *)((int)local_30 + 0x1a2) != '\0') || (local_85 != 0)) ||
                    (local_30[0x41] != (int *)0x0)) && (iVar13 != 0)) {
                  local_30[0x3f][(int)local_30[0x40]] = iVar13;
                  iVar13 = wlc[0x11d];
                  bVar20 = *(byte *)(iVar13 + 5);
                  piVar17 = (int *)((uint)bVar20 & 1);
                  if ((bVar20 & 1) != 0) {
                    piVar17 = (int *)(int)*(char *)(rxh + 0x1f);
                  }
                  local_30[(int)local_30[0x40] + 0x6b] = piVar17;
                  piVar17 = (int *)((int)(uint)*(byte *)(iVar13 + 5) >> 1 & 1);
                  if (piVar17 != (int *)0x0) {
                    piVar17 = (int *)(int)*(char *)(rxh + 0x20);
                  }
                  local_30[(int)local_30[0x40] + 0x73] = piVar17;
                  local_30[0x40] = (int *)((int)local_30[0x40] + 1U & 7);
                }
                if (local_8b == false) {
                  if ((*(code *)((int)local_2c[0] + 6) == (code)0x0) &&
                     ((local_2c[0][0xd2] == (int *)0x0 ||
                      (-1 < (int)((uint)*(byte *)((int)local_2c[0][0xd2] + 0xe) << 0x1f))))) {
                    local_4c = *(ushort *)(local_30 + 0x11);
                    if ((*(code *)(local_2c[0] + 4) == (code)0x0) &&
                       (uVar12 = *(ushort *)((int)local_2c[0] + 0x5a),
                       (int)((uint)uVar12 << 0x1f) < 0)) goto LAB_0001900e;
                  }
                  else {
                    local_4c = *(ushort *)(local_30 + 0x11);
                    if (((((int)local_30[1] << 0x19 < 0) &&
                         ((((local_8a != 0 && ((local_36 & 0xf) == 0)) &&
                           (*(char *)((int)local_30 + 0xe7) != '\0')) &&
                          ((-1 < (int)((uint)*(byte *)(local_30 + 5) << 0x1f) &&
                           ((int)((uint)local_98 << 0x13) < 0)))))) &&
                        (((int)local_30[1] << 0xe < 0 &&
                         (((int)(uint)*(byte *)((int)local_30 + 0xd9) >> local_90) << 0x1f < 0))))
                       && ((((local_2c[0][0xd2] == (int *)0x0 ||
                             (-1 < (int)((uint)*(byte *)((int)local_2c[0][0xd2] + 0xe) << 0x1f))) ||
                            (local_8c == '\0')) ||
                           (uVar14 = FUN_0002127c((int)wlc,(int)local_30), uVar14 != 0)))) {
                      func_0x0083addc(wlc,local_30,local_90);
                    }
                    if ((local_98 & 0x1000) == 0) {
                      piVar17 = (int *)((uint)local_30[1] & 0xfffdffff);
                    }
                    else {
                      piVar17 = (int *)((uint)local_30[1] | 0x20000);
                    }
                    local_30[1] = piVar17;
                  }
                }
                else {
                  uVar12 = *(ushort *)((int)local_2c[0] + 0x5a);
LAB_0001900e:
                  local_4c = uVar12;
                }
                if ((((*(code *)((int)local_2c[0] + 6) == (code)0x0) &&
                     (*(code *)(local_2c[0] + 4) != (code)0x0)) && (local_88 == 0)) && (uVar16 == 0)
                   ) {
                  *(undefined *)((int)piVar9 + 6) = 0;
                }
                if ((local_88 == 0) ||
                   (((*(code *)((int)local_2c[0] + 6) == (code)0x0 &&
                     ((*(code *)(local_2c[0] + 4) == (code)0x0 ||
                      (iVar13 = FUN_000023ee((int)(local_b4 + 8),(int)((int)local_2c[0] + 0xc2),6),
                      iVar13 != 0)))) &&
                    ((iVar13 = FUN_00009474((int)(local_b4 + 2)), iVar13 != 0 ||
                     ((*(code *)((int)local_2c[0] + 0x45) != (code)0x0 ||
                      (iVar13 = func_0x00847bd8(local_2c[0],local_b4 + 2), iVar13 == 0)))))))) {
                  *(int **)(p + 0x16) = piVar1;
                  p[0xe] = local_36;
                  if ((*(byte *)(local_b4 + 8) & 1) == 0) {
                    local_30[0x58] = (int *)((int)local_30[0x58] + 1);
                    ppiVar21 = local_30 + 100;
                    piVar9 = *ppiVar21;
                    piVar17 = local_30[0x65];
                  }
                  else {
                    local_30[0x59] = (int *)((int)local_30[0x59] + 1);
                    ppiVar21 = local_30 + 0x66;
                    piVar9 = *ppiVar21;
                    piVar17 = local_30[0x67];
                  }
                  piVar10 = (int *)((int)piVar9 + uVar7);
                  *ppiVar21 = piVar10;
                  ppiVar21[1] = (int *)((int)piVar17 + (uint)CARRY4((uint)piVar9,uVar7));
                  if ((local_88 == 0) &&
                     (bVar20 = *(byte *)((int)puVar22 + 1) | *(byte *)puVar22,
                     piVar10 = (int *)(uint)bVar20, (*(byte *)(puVar22 + 1) | bVar20) != 0)) {
                    piVar10 = (int *)(uint)*(ushort *)(rxh + 0x12);
                    FUN_0003d668(wlc[0x58],*(uint *)(p + 0x16),(uint)piVar10);
                    local_30[0x5b] = piVar1;
                  }
                  if (*(char *)((int)puVar22 + 3) < '\0') {
                    piVar10 = (int *)(*(int *)(*(int *)(*wlc + 0x88) + 0x2a8) + 1);
                    *(int **)(*(int *)(*wlc + 0x88) + 0x2a8) = piVar10;
                  }
                  if ((*(byte *)((int)puVar22 + 3) & 0x30) != 0) {
                    piVar10 = (int *)(*(int *)(*(int *)(*wlc + 0x88) + 0x2b0) + 1);
                    *(int **)(*(int *)(*wlc + 0x88) + 0x2b0) = piVar10;
                  }
                  if (local_88 == 0) {
                    uVar7 = *(uint *)(p + 0x16);
                    iVar13 = *(int *)(*wlc + 0x88);
                    if ((uVar7 & 0x3000000) == 0) {
                      pcVar3 = (code *)(uVar7 & 0xff);
                    }
                    else {
                      pcVar3 = (code *)FUN_000389d8(uVar7);
                    }
                    if (pcVar3 == NotUsed + 2) {
                      *(int *)(iVar13 + 0x24c) = *(int *)(iVar13 + 0x24c) + 1;
                    }
                    else {
                      if (NotUsed + 2 < pcVar3) {
                        if (pcVar3 == (code *)0x30) {
                          *(int *)(iVar13 + 600) = *(int *)(iVar13 + 600) + 1;
                        }
                        else {
                          if (pcVar3 < (code *)0x31) {
                            if (pcVar3 == IRQ) {
                              *(int *)(iVar13 + 0x250) = *(int *)(iVar13 + 0x250) + 1;
                            }
                            else {
                              if (pcVar3 == (code *)0x24) {
                                *(int *)(iVar13 + 0x254) = *(int *)(iVar13 + 0x254) + 1;
                              }
                            }
                          }
                          else {
                            if (pcVar3 == (code *)0x60) {
                              *(int *)(iVar13 + 0x260) = *(int *)(iVar13 + 0x260) + 1;
                            }
                            else {
                              if (pcVar3 == (code *)0x6c) {
                                *(int *)(iVar13 + 0x264) = *(int *)(iVar13 + 0x264) + 1;
                              }
                              else {
                                if (pcVar3 == (code *)0x48) {
                                  *(int *)(iVar13 + 0x25c) = *(int *)(iVar13 + 0x25c) + 1;
                                }
                              }
                            }
                          }
                        }
                      }
                      else {
                        if (pcVar3 == SupervisorCall + 3) {
                          *(int *)(iVar13 + 0x240) = *(int *)(iVar13 + 0x240) + 1;
                        }
                        else {
                          if (SupervisorCall + 3 < pcVar3) {
                            if (pcVar3 == PrefetchAbort) {
                              *(int *)(iVar13 + 0x244) = *(int *)(iVar13 + 0x244) + 1;
                            }
                            else {
                              if (pcVar3 == DataAbort + 2) {
                                *(int *)(iVar13 + 0x248) = *(int *)(iVar13 + 0x248) + 1;
                              }
                            }
                          }
                          else {
                            if (pcVar3 == Reset) {
                              *(int *)(iVar13 + 0x238) = *(int *)(iVar13 + 0x238) + 1;
                            }
                            else {
                              if (pcVar3 == UndefinedInstruction) {
                                *(int *)(iVar13 + 0x23c) = *(int *)(iVar13 + 0x23c) + 1;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                  if (local_88 != 0) {
                    FUN_0000dcf6(local_2c[0],(int)&local_b4,piVar10,(uint)local_88);
                  }
                  if (local_88 == 0) {
                    puVar4 = *(undefined **)(p + 0x16);
                    if (((uint)puVar4 & 0x3000000) != 0) {
                      puVar4 = FUN_000389d8((uint)puVar4);
                    }
                    uVar7 = (uint)puVar4 & 0x7f;
                    if (((uVar7 == 2) || (uVar7 == 4)) || (uVar7 == 0xb)) {
                      bVar23 = true;
                    }
                    else {
                      bVar23 = uVar7 == 0x16;
                    }
                    FUN_00040d50((int)wlc,(uint)(*(ushort *)(rxh + 6) >> 8),0,
                                 (int)*(char *)(rxh + 0x1c),'\0',bVar23,
                                 (byte)((ushort)*(undefined2 *)(rxh + 4) >> 0xe),'\0',
                                 (int)local_2c[0]);
                  }
                  if ((((int)local_30[1] << 0xd < 0) && (local_88 == 0)) && (uVar16 == 0)) {
                    FUN_0001e9b0((int **)wlc[0x4e],local_30,(int *)&local_b4);
                    return;
                  }
                  FUN_00018274(wlc,(int)local_30,(int *)&local_b4);
                  return;
                }
                goto LAB_00019292;
              }
            }
          }
LAB_00018e88:
          *(int *)(*(int *)(*wlc + 0x88) + 0x58) = *(int *)(*(int *)(*wlc + 0x88) + 0x58) + 1;
        }
      }
LAB_00019292:
      iVar13 = *wlc;
      if ((*(int *)(iVar13 + 0x34) != 0) && (local_88 == 0)) {
        iVar18 = *(int *)(iVar13 + 0x8c) +
                 ((uint)*(byte *)(DAT_000192fc +
                                 (uint)*(byte *)(DAT_000192f8 + ((uint)local_a0[7] & 7))) + 0xc) * 8
        ;
        iVar19 = *(int *)(iVar18 + 8);
        iVar13 = *(int *)(iVar18 + 4) + 1;
        *(int *)(iVar18 + 4) = iVar13;
        iVar5 = FUN_00009338(osh,(int)local_a0);
        *(int *)(iVar18 + 8) = iVar5 + iVar19;
      }
      goto LAB_000192ca;
    }
    iVar13 = *wlc;
  }
  iVar13 = *(int *)(iVar13 + 0x88);
  *(int *)(iVar13 + 100) = *(int *)(iVar13 + 100) + 1;
LAB_000192ca:
  pkt_buf_free_skb(osh,local_a0,0,iVar13);
  return;
}


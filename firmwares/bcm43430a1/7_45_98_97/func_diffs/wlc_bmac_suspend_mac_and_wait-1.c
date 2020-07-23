
/* WARNING: Control flow encountered bad instruction data */

void wlc_bmac_suspend_mac_and_wait(int *wlc_hw,undefined4 param_2,uint param_3)

{
  int extraout_r1;
  int extraout_r1_00;
  int extraout_r1_01;
  int iVar1;
  int iVar2;
  undefined *puVar3;
  
  iVar1 = wlc_hw[0x3c];
  iVar2 = wlc_hw[0x22];
  wlc_hw[0x3c] = iVar1 + 1U;
  if (1 < iVar1 + 1U) {
    return;
  }
  if (*(char *)(*wlc_hw + 0x82a) != '\0') {
    func_0x008457f4(wlc_hw,2,0);
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  }
  func_0x00846ad4(wlc_hw,4);
  iVar1 = extraout_r1;
  if (*(int *)(iVar2 + 0x120) != -1) {
    param_3 = *(uint *)(iVar2 + 0x128);
    iVar1 = param_3 + 1;
    if (iVar1 != 0) {
      param_3 = param_3 & 1;
      if (param_3 == 0) {
        func_0x008457f4(wlc_hw,1);
        puVar3 = &DAT_0000206d;
        iVar1 = extraout_r1_00;
        while ((-1 < *(int *)(iVar2 + 0x128) << 0x1f &&
               (puVar3 = puVar3 + -1, puVar3 != (undefined *)0x0))) {
          func_0x008081f0(10);
          iVar1 = extraout_r1_01;
        }
        if (*(int *)(iVar2 + 0x128) << 0x1f < 0) {
          if (*(int *)(iVar2 + 0x120) != -1) {
            return;
          }
          goto LAB_0001c4a2;
        }
        iVar1 = 5;
      }
      else {
        iVar1 = 4;
      }
      wlc_hw[0x51] = iVar1;
      return;
    }
  }
LAB_0001c4a2:
  FUN_00008930(*(int *)(*wlc_hw + 8),iVar1,param_3);
  return;
}


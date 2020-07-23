
/* WARNING: Control flow encountered bad instruction data */

void wlc_bmac_enable_mac(int *wlc_hw,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  
  iVar3 = wlc_hw[0x22];
  if ((wlc_hw[0x51] == 0) && (iVar2 = wlc_hw[0x3c], wlc_hw[0x3c] = iVar2 + -1, iVar2 + -1 == 0)) {
    if (*(byte *)(*wlc_hw + 0x82a) == 0) {
      uVar1 = 1;
    }
    else {
      uVar1 = 3;
    }
    func_0x008457f4(wlc_hw,uVar1,uVar1,(uint)*(byte *)(*wlc_hw + 0x82a),param_4);
    *(undefined4 *)(iVar3 + 0x128) = 1;
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  }
  return;
}



int ** wlc_scb_lookup(int *wlc,int bsscfg,undefined4 *ea,int param_4)

{
  int iVar1;
  int **ppiVar2;
  
  if ((int)((uint)*(byte *)ea << 0x1f) < 0) {
    return (int **)0x0;
  }
  iVar1 = func_0x00859368(wlc[0x19a]);
  if ((iVar1 == 1) || (iVar1 == 4)) {
    ppiVar2 = (int **)0x0;
  }
  else {
    ppiVar2 = FUN_0003d2e8((int)wlc,bsscfg,(ushort *)ea,param_4);
    if (ppiVar2 == (int **)0x0) {
      ppiVar2 = (int **)FUN_0003d440(wlc,bsscfg,ea,wlc[param_4 + 10]);
      return ppiVar2;
    }
  }
  return ppiVar2;
}


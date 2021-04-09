
/* WARNING: Control flow encountered bad instruction data */

int ** wlc_scb_lookup(int wlc,int bsscfg,ushort *ea,int param_4)

{
  int iVar1;
  int **ppiVar2;
  
  if ((int)((uint)*(byte *)ea << 0x1f) < 0) {
    return (int **)0x0;
  }
  iVar1 = func_0x00859368(*(undefined4 *)(wlc + 0x668));
  if ((iVar1 == 1) || (iVar1 == 4)) {
    ppiVar2 = (int **)0x0;
  }
  else {
    ppiVar2 = FUN_000304f4(wlc,bsscfg,ea,param_4);
    if (ppiVar2 == (int **)0x0) {
                    /* WARNING: Bad instruction - Truncating control flow here */
      halt_baddata();
    }
  }
  return ppiVar2;
}


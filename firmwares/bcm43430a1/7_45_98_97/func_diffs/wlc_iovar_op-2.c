
int wlc_iovar_op(uint *wlc,int varname,undefined4 *params,uint p_len,undefined4 *arg,uint len,
                char set,undefined4 wlcif)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  char *pcVar6;
  int iVar7;
  int iVar8;
  code *local_3c;
  int local_30;
  uint local_2c [2];
  
  pcVar6 = (char *)wlc[0x68];
  iVar2 = 0;
  local_30 = 0;
  iVar8 = 0;
  iVar7 = 0;
  local_3c = Reset;
  while (iVar7 < (int)(uint)*(byte *)(*wlc + 0xb8)) {
    iVar3 = *(int *)(wlc[0x1c1] + iVar7 * 8);
    if (iVar3 == 0) {
      iVar3 = *(int *)(wlc[0xaa] + iVar8 + 0x20);
      if ((iVar3 != 0) && (iVar2 = func_0x0081ed54(iVar3,varname), iVar2 != 0)) {
        local_3c = *(code **)(wlc[0xaa] + iVar8 + 0x28);
        goto LAB_0000e084;
      }
    }
    else {
      iVar2 = func_0x0081ed54(iVar3,varname);
      if (iVar2 != 0) {
        if (*(short *)(iVar2 + 6) != -1) {
          local_3c = *(code **)(wlc[0x1c1] + iVar7 * 8 + 4);
          goto LAB_0000e084;
        }
        local_3c = Reset + 1;
      }
    }
    iVar7 = iVar7 + 1;
    iVar8 = iVar8 + 0x38;
  }
  if (local_3c != Reset) {
LAB_0000e0e0:
    local_30 = -0x17;
    goto LAB_0000e204;
  }
LAB_0000e084:
  if ((int)(uint)*(byte *)(*wlc + 0xb8) <= iVar7) {
    iVar7 = 0;
    iVar8 = 0;
    while (iVar7 < (int)(uint)*(byte *)(*wlc + 0xb8)) {
      iVar3 = *(int *)(wlc[0xaa] + iVar8 + 0x20);
      if (((iVar3 != 0) && (*(int *)(wlc[0x1c1] + iVar7 * 8) != 0)) &&
         (iVar2 = func_0x0081ed54(iVar3,varname), iVar2 != 0)) {
        local_3c = *(code **)(wlc[0xaa] + iVar8 + 0x28);
        break;
      }
      iVar7 = iVar7 + 1;
      iVar8 = iVar8 + 0x38;
    }
    if ((int)(uint)*(byte *)(*wlc + 0xb8) <= iVar7) goto LAB_0000e0e0;
  }
  if (params == (undefined4 *)0x0) {
    params = arg;
    p_len = len;
  }
  uVar1 = *(ushort *)(iVar2 + 8);
  uVar5 = (uint)uVar1;
  if ((uVar1 != 0) && (uVar5 = len, uVar1 != 8)) {
    uVar5 = 4;
  }
  uVar4 = (uint)*(ushort *)(iVar2 + 4) * 2;
  if (set != '\0') {
    uVar4 = uVar4 + 1;
  }
  iVar8 = func_0x00803c8c(DAT_0000e21c,varname,7);
  if (iVar8 == 0) {
    if ((int)p_len < 4) {
      local_30 = -0xe;
      goto LAB_0000e204;
    }
    memcpy(local_2c,params,4);
    iVar8 = FUN_000266f6((int)wlc,local_2c[0],&local_30);
    if ((local_30 == -0x1e) && ((int)(uVar4 << 0x1f) < 0)) {
      iVar8 = func_0x008476b8(wlc,local_2c[0],0,0,1);
      if (iVar8 == 0) {
        local_30 = -0x1b;
      }
      else {
        local_30 = FUN_00026aa8((int *)wlc,iVar8);
        if (local_30 != 0) {
          func_0x00847abc(wlc,iVar8);
        }
      }
    }
    if (local_30 != 0) goto LAB_0000e204;
    varname = varname + 7;
    params = params + 1;
    p_len = p_len - 4;
    if ((int)(uVar4 << 0x1f) < 0) {
      len = len - 4;
      arg = arg + 1;
    }
    wlcif = *(undefined4 *)(iVar8 + 0xc);
  }
  local_30 = func_0x0082aac8(wlc,iVar2,arg,len,uVar4 & 1,wlcif);
  if (((local_30 == 0) &&
      (local_30 = (*local_3c)(*(undefined4 *)(wlc[0xab] + iVar7 * 4),iVar2,uVar4,varname,params,
                              p_len,arg,len,uVar5,wlcif), (uVar4 & 1) != 0)) && (*pcVar6 != '\0')) {
    func_0x00876118(pcVar6);
  }
LAB_0000e204:
  if (local_30 != 0) {
    uVar4 = local_30 + 0x34;
    uVar5 = uVar4;
    if (uVar4 < 0x35) {
      uVar5 = *wlc;
    }
    if (uVar4 < 0x35) {
      *(int *)(uVar5 + 0x68) = local_30;
    }
  }
  return local_30;
}


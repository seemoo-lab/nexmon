
int wlc_sendctl(int *wlc,ushort *p,int qi,int scb,uint fifo,uint rate_override,char enq_only)

{
  byte bVar1;
  ushort uVar2;
  uint uVar3;
  int iVar4;
  
  iVar4 = *(int *)(scb + 0x10);
  if (iVar4 == 0) {
LAB_0000de5e:
    pkt_buf_free_skb((ushort **)wlc[1],p,1,scb);
    iVar4 = 0;
    *(int *)(*(int *)(*wlc + 0x88) + 0x20) = *(int *)(*(int *)(*wlc + 0x88) + 0x20) + 1;
  }
  else {
    uVar2 = p[7];
    if ((((uint)PTR_DAT_0000de80 & *(uint *)(scb + 4)) != 0) && (*(int *)(*wlc + 0x34) != 0)) {
      fifo = (uint)*(byte *)(DAT_0000de84 + ((uint)uVar2 & 7));
    }
    *(int *)(p + 0x14) = scb;
    *(undefined *)((int)p + 0x21) = *(undefined *)(iVar4 + 0x44);
    func_0x0081f7dc(wlc,p,0,0,rate_override,fifo);
    if ((((*(char *)(*(int *)(scb + 0x10) + 6) == '\0') &&
         (-1 < *(int *)(*(int *)(scb + 0x10) + 0x394) << 0x1d)) || (*(char *)(scb + 0xe7) == '\0'))
       || (*(int *)(p + 0xc) << 0x1e < 0)) {
      bVar1 = *(byte *)(DAT_0000de88 + ((uint)uVar2 & 7));
      if (bVar1 < 0xe) {
        scb = (uint)bVar1 + 1;
      }
      else {
        scb = 0xf;
      }
      iVar4 = FUN_000095a8();
      if (iVar4 == 0) goto LAB_0000de5e;
      if (enq_only == '\0') {
        FUN_0000dbe2(wlc,qi);
      }
    }
    else {
      scb = DAT_0000de88;
      uVar3 = FUN_000195f0(wlc,(uint)p);
      if (uVar3 == 0) goto LAB_0000de5e;
    }
    iVar4 = 1;
  }
  return iVar4;
}


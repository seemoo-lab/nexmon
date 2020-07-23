
void wlc_lcn40phy_run_samples
               (int pi,short num_samps,int num_loops,ushort wait,char iqcalmode,byte tx_pu_param)

{
  uint uVar1;
  ushort val;
  ushort mask;
  undefined4 unaff_r4;
  undefined4 unaff_r5;
  int iVar2;
  undefined4 unaff_r6;
  uint uVar3;
  
  val = (ushort)num_loops;
  phy_reg_or(pi,(uint16)CONCAT412(unaff_r6,CONCAT48(unaff_r5,CONCAT44(unaff_r4,(uint)wait))),0x6da);
  phy_reg_mod(pi,0x642,0xff,num_samps - 1);
  if (num_loops != 0xffff) {
    val = val - 1;
  }
  phy_reg_mod(pi,0x640,0xffff,val);
  phy_reg_mod(pi,0x641,0xffff,wait);
  if (iqcalmode == '\0') {
    uVar3 = 1;
  }
  else {
    uVar3 = 2;
  }
  iVar2 = 10;
  do {
    if (iqcalmode == '\0') {
      val = 0x63f;
      mask = 1;
    }
    else {
      val = 0x453;
      mask = 0x8000;
    }
    phy_reg_mod(pi,val,mask,mask);
    func_0x008081f0(1);
    uVar1 = phy_reg_read(pi,0x644);
    iVar2 = iVar2 + -1;
  } while ((iVar2 != 0) && ((uVar1 & uVar3) == 0));
  if (iqcalmode != '\0') {
    return;
  }
  FUN_0002feb4(pi,1,(uint)tx_pu_param);
  return;
}


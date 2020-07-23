
void wlc_bmac_read_tsf(int wlc_hw,uint tsf_l_ptr,uint tsf_h_ptr)

{
  int iVar1;
  
  iVar1 = *(int *)(wlc_hw + 0x88);
  if (tsf_l_ptr != 0) {
    *(undefined4 *)tsf_l_ptr = *(undefined4 *)(iVar1 + 0x180);
  }
  if (tsf_h_ptr != 0) {
    *(undefined4 *)tsf_h_ptr = *(undefined4 *)(iVar1 + 0x184);
  }
  return;
}



void wlc_bsscfg_find_by_wlcif(int wlc,int wlcif)

{
  if (wlcif == 0) {
    FUN_0002671a(wlc);
    return;
  }
  if (*(char *)(wlcif + 4) != '\x01') {
    if (*(char *)(wlcif + 4) == '\x02') {
      return;
    }
    return;
  }
  return;
}


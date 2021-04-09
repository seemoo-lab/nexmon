
int hndrte_del_timer(int t)

{
  if (*(int *)(t + 0x28) != 0) {
    *(undefined4 *)(t + 0x28) = 0;
    *(undefined *)(t + 0x20) = 0;
    func_0x00807ba0(t + 0x10);
  }
  return 1;
}


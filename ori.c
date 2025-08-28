#define TB_IMPL

#include "termbox2.h"
#include <stdio.h>
int
main(void)
{
  if(tb_init() != 0)
  {
    fprintf(stderr, "Termbox init fail\n");
    return -1;
  }

  tb_clear();
}

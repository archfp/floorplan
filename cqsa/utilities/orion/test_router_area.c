#include <stdio.h>
#include "SIM_port.h"
#include "SIM_power.h"
#include "SIM_power_array.h"
#include "SIM_power_router.h"


int main(int argc, char **argv)
{
  u_int n_port, n_row, flit_width;
  double bitline_len, wordline_len, xb_in_len, xb_out_len;
  double Atotal = 0;

  n_port = atoi(argv[1]);
  n_row = PARM(in_buf_set);
  flit_width = PARM(flit_width);

  /* 1 read port and 1 write port */
  bitline_len = n_row * (RegCellHeight + 2 * WordlineSpacing);
  wordline_len = flit_width * (RegCellWidth + 2 * 2 * BitlineSpacing);

  /* input buffer area */
  Atotal += n_port * (bitline_len * wordline_len);

  /* only have a crossbar if more than one port */
  if (n_port > 1) {
    xb_in_len = n_port * flit_width * CrsbarCellWidth;
    xb_out_len = n_port * flit_width * CrsbarCellHeight;

    /* crossbar area */
    Atotal += xb_in_len * xb_out_len;
  }

  printf("%g", Atotal);

  exit(0);
}

#include <stdio.h>
#include "SIM_power.h"
#include "SIM_power_misc.h"
#include "SIM_power_misc_internal.h"


static int SIM_fpfp_avg_read_stat(SIM_power_ff_t *ff, u_int width);
static int SIM_fpfp_avg_write_stat(SIM_power_ff_t *ff, u_int width);


int main(int argc, char **argv)
{
  SIM_power_ff_t ff;
  double avg_read, avg_write, Estatic;
  u_int width = 4;

  SIM_fpfp_init(&ff, NEG_DFF, 0);

  SIM_fpfp_clear_stat(&ff);
  SIM_fpfp_avg_read_stat(&ff, width);
  avg_read = SIM_fpfp_report(&ff);

  SIM_fpfp_clear_stat(&ff);
  SIM_fpfp_avg_write_stat(&ff, width);
  avg_write = SIM_fpfp_report(&ff);

  Estatic = ff.I_static * Vdd * Period * SCALE_S;

  printf("average read energy: %g\naverage write energy: %g\nstatic energy: %g\n", avg_read, avg_write, Estatic * width);

  return 0;
}


static int SIM_fpfp_avg_read_stat(SIM_power_ff_t *ff, u_int width)
{
  ff->n_clock += width;

  return 0;
}


static int SIM_fpfp_avg_write_stat(SIM_power_ff_t *ff, u_int width)
{
  ff->n_clock += width;
  ff->n_switch += width * 0.5;
  ff->n_keep_0 += width * 0.25;
  ff->n_keep_1 += width * 0.25;

  return 0;
}

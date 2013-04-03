/* 
 * A dummy simulator template file for HotSpot block model to 
 * illustrate its use. The grid model is also similar in
 * its use and the actual details of its use can be found
 * in the README file and in the comments provided in hotspot.c.
 * This file contains the following sample routines:
 * 	a) Model initialization	(sim_init)
 *	b) Model use in a cycle-by-cycle power model (sim_main)
 *	c) Model uninitialization (sim_exit)
 *  Please note that all of these routines are just instructional
 *  templates and not full-fledged code. Hence they are not used
 *  anywhere else in the distribution.
 */
#include <string.h>

#include "temperature.h"
#include "flp.h"
#include "util.h"

/* input and output files	*/
static char *flp_file;		/* has the floorplan configuration	*/
static char *init_file;		/* initial temperatures	from file	*/
static char *steady_file;	/* steady state temperatures to file	*/

/* floorplan	*/
static flp_t *flp;
/* hotspot temperature model	*/
static RC_model_t *model;
/* instantaneous temperature and power values	*/
static double *temp, *power;
/* steady state temperature and power values	*/
static double *overall_power, *steady_temp;

/* sample model initialization	*/
void sim_init()
{
	/* initialize flp, get adjacency matrix */
	flp = read_flp(flp_file, FALSE);

	/* 
	 * configure thermal model parameters. default_thermal_config 
	 * returns a set of default parameters. only those configuration
	 * parameters (config.*) that need to be changed are set explicitly. 
	 */
	thermal_config_t config = default_thermal_config();
	strcpy(config.init_file, init_file);
	strcpy(config.steady_file, steady_file);

	/* allocate and initialize the RC model	*/
	model = alloc_RC_model(&config, flp);
	populate_R_model(model, flp);
	populate_C_model(model, flp);

	/* allocate the temp and power arrays	*/
	/* using hotspot_vector to internally allocate any extra nodes needed	*/
	temp = hotspot_vector(model);
	power = hotspot_vector(model);
	steady_temp = hotspot_vector(model);
	overall_power = hotspot_vector(model);
	
	/* set up initial instantaneous temperatures */
	if (strcmp(model->config->init_file, NULLFILE)) {
		if (!model->config->dtm_used)	/* initial T = steady T for no DTM	*/
			read_temp(model, temp, model->config->init_file, FALSE);
		else	/* initial T = clipped steady T with DTM	*/
			read_temp(model, temp, model->config->init_file, TRUE);
	}
	else	/* no input file - use init_temp as the common temperature	*/
		set_temp(model, temp, model->config->init_temp);
}

/* 
 * sample routine to illustrate the possible use of hotspot in a 
 * cycle-by-cycle power model. note that this is just a stub 
 * function and is not called anywhere in this file	
 */
void sim_main()
{
	static double cur_time, prev_time;
	int i;

	/* the main simulator loop */
	while (1) {
		
		/* set the per cycle power values as returned by Wattch/power simulator	*/
		power[get_blk_index(flp, "Icache")] +=  0;	/* set the power numbers instead of '0'	*/
		power[get_blk_index(flp, "Dcache")] +=  0;	
		power[get_blk_index(flp, "Bpred")] +=  0;	
		/* ... more functional units ...	*/


		/* call compute_temp at regular intervals */
		if ((cur_time - prev_time) >= model->config->sampling_intvl) {
			double elapsed_time = (cur_time - prev_time);
			prev_time = cur_time;

			/* find the average power dissipated in the elapsed time */
			for (i = 0; i < flp->n_units; i++) {
				overall_power[i] += power[i];
				/* 
				 * 'power' array is an aggregate of per cycle numbers over 
				 * the sampling_intvl. so, compute the average power 
				 */
				power[i] /= (elapsed_time * model->config->base_proc_freq);
			}

			/* calculate the current temp given the previous temp,
			 * time elapsed since then, and the average power dissipated during 
			 * that interval */
			compute_temp(model, power, temp, elapsed_time);

			/* reset the power array */
			for (i = 0; i < flp->n_units; i++)
				power[i] = 0;
		}
	}
}

/* 
 * sample uninitialization routine to illustrate the possible use of hotspot in a 
 * cycle-by-cycle power model. note that this is just a stub 
 * function and is not called anywhere in this file	
 */
void sim_exit()
{
	double total_elapsed_cycles = 0; 	/* set this to be the correct time elapsed  (in cycles) */
	int i;

	/* find the average power dissipated in the elapsed time */
	for (i = 0; i < flp->n_units; i++) {
		overall_power[i] /= total_elapsed_cycles;
	}

	/* get steady state temperatures */
	steady_state_temp(model, overall_power, steady_temp);

	/* dump temperatures if needed	*/
	if (strcmp(model->config->steady_file, NULLFILE))
		dump_temp(model, steady_temp, model->config->steady_file);

	/* cleanup..*/
	delete_RC_model(model);
	free_flp(flp, 0);
	free_dvector(temp);
	free_dvector(power);
	free_dvector(steady_temp);
	free_dvector(overall_power);
}

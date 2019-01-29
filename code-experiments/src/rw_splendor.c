/**
 * @file rw_gan_mario.c
 *
 * @brief Implementation of the real-world problems of unsupervised learning of a Generative
 * Adversarial Network (GAN) that understands the structure of Super Mario Bros. levels.
 */

#include <stdio.h>
#include <assert.h>

#include "coco.h"
#include "coco_problem.c"
#include "rw_problem.c"

/**
 * @brief Creates a single- or bi-objective rw_gan_mario problem.
 */
static coco_problem_t *rw_splendor_problem_allocate(const char *suite_name,
                                                     const size_t objectives,
                                                     const size_t function,
                                                     const size_t dimension,
                                                     const size_t instance) {

  coco_problem_t *problem = NULL;
  size_t i;

  if ((objectives != 1) && (objectives != 2))
    coco_error("rw_splendor_problem_allocate(): %lu objectives are not supported (only 1 or 2)",
        (unsigned long)objectives);

  /*1: sequenceLength, integer, 1-(200/number of players)
  2: evals, integer, 0-budgetPerCall (1000)
  3: flipatlestonevalue -> binary
  4: useShiftBuffer -> binary*/

  /*TODO: figure out number of players based on function*/

  problem = coco_problem_allocate(dimension, objectives, 0);
  problem->smallest_values_of_interest[0] = 1;
  problem->largest_values_of_interest[0] = 50;
  problem->smallest_values_of_interest[1] = 0;
  problem->largest_values_of_interest[1] = 1000;
  if(dimension>2){
	  problem->smallest_values_of_interest[2] = 0;
	  problem->largest_values_of_interest[2] = 1;
    if(dimension>3){
	    problem->smallest_values_of_interest[3] = 0;
	    problem->largst_values_of_interest[3] = 1;
    }
  }

  problem->number_of_integer_variables = 4;
  problem->evaluate_function = rw_problem_evaluate;
  problem->problem_free_function = rw_problem_data_free;

  coco_problem_set_id(problem, "%s_f%03lu_i%02lu_d%02lu", suite_name, (unsigned long) function,
      (unsigned long) instance, (unsigned long) dimension);

  /*coco_error("test objectives %lu problem f%lu instance %lu in %luD",
        objectives, function, instance, dimension);*/


  if (objectives == 1) {
    coco_problem_set_name(problem, "real-world Splendor single-objective problem f%lu instance %lu in %luD",
        function, instance, dimension);
	problem->best_value[0] = 0;

    coco_problem_set_type(problem, "single-objective");
  }
  else if (objectives == 2) {
    coco_problem_set_name(problem, "real-world Splendor bi-objective problem f%lu instance %lu in %luD",
        function, instance, dimension);
    coco_problem_set_type(problem, "bi-objective");
    /* TODO Add realistic values */
    problem->best_value[0] = -1000;
    problem->best_value[1] = -1000;
    problem->nadir_value[0] = 1000;
    problem->nadir_value[1] = 1000;
  }

  if (problem->best_parameter != NULL) {
    coco_free_memory(problem->best_parameter);
    problem->best_parameter = NULL;
  }

  problem->data = get_rw_problem_data("splendor", objectives, function, dimension, instance);

  return problem;
}

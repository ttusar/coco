/**
 * @file rw_mario_gan.c
 *
 * @brief Implementation of Mario GAN functions
 */
#include "coco.h"
#include "coco_platform.h"
#include "socket_communication.c"


/**
 * @brief Creates the mario-gan problem.
 */
static coco_problem_t *rw_mario_gan_problem_allocate(const size_t number_of_objectives,
                                                     const size_t function,
                                                     const size_t dimension,
                                                     const size_t instance,
                                                     const char *problem_id_template,
                                                     const char *problem_name_template) {

  coco_problem_t *problem = NULL;
  size_t i;

  if ((number_of_objectives != 1) && (number_of_objectives != 2))
    coco_error("rw_mario_gan_problem_allocate(): %lu objectives are not supported (only 1 or 2)",
        (unsigned long)number_of_objectives);

  /* Provide the region of interest */
  problem = coco_problem_allocate(dimension, number_of_objectives, 0);
  for (i = 0; i < dimension; ++i) {
    problem->smallest_values_of_interest[i] = -1;
    problem->largest_values_of_interest[i] = 1;
  }
  problem->number_of_integer_variables = 0;
  problem->evaluate_function = socket_evaluate;

  coco_problem_set_id(problem, problem_id_template, function, instance, dimension);
  coco_problem_set_name(problem, problem_name_template, function, instance, dimension);

  if (((number_of_objectives == 1) && (function <= 10)) ||
      ((number_of_objectives == 2) && (function <= 2))) {
    coco_problem_set_type(problem, "rw-mario-gan-computed");
  } else {
    coco_problem_set_type(problem, "rw-mario-gan-simulated");
  }

  if (number_of_objectives == 1) {
    /* Unknown best_parameter and best_value */
    if (problem->best_parameter != NULL) {
      coco_free_memory(problem->best_parameter);
      problem->best_parameter = NULL;
    }
    if (problem->best_value != NULL) {
      coco_free_memory(problem->best_value);
      problem->best_value = NULL;
    }
  }

  /* Need to provide estimation of the ideal and nadir points for all bi-objective problem instances */
  else if (number_of_objectives == 2) { /* TODO Vanessa */
    problem->best_value[0] = -1000;
    problem->best_value[1] = -1000;
    problem->nadir_value[0] = 1000;
    problem->nadir_value[1] = 1000;
  }
  return problem;
}

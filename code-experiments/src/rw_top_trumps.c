#include "rw_top_trumps.h" /* File copied here during building of the top trumps problem */
#include "coco.h"
#include "coco_platform.h"
#include "socket_communication.c"


static void rw_top_trumps_set_bounds(coco_problem_t *problem,
                                     const size_t instance) {

  /* Low and high bounds are given for each of the 15 instances (rows) and repeated every
   * 4 dimensions (columns) */
  double low_bound[15][4] = {
      {51, 55, 26, 21},
      {32, 52, 14,  5},
      {7,  48,  2, 11},
      {76, 44, 85,  6},
      {45, 41, 67,  1},
      {14,  2, 50, 72},
      {40, 11, 32, 14},
      {21, 19, 14, 56},
      {3,  26, 29, 80},
      {84, 22, 17, 40},
      {59, 19,  5, 70},
      {28, 15, 44, 23},
      {29, 11, 26, 60},
      {11,  8,  9,  7},
      {35,  4, 57, 49}
  };
  double high_bound[15][4] = {
      {69, 59, 38, 63},
      {38, 68, 20, 16},
      {14, 76, 3, 46},
      {95, 85, 90, 88},
      {77, 94, 77, 30},
      {58, 37, 65, 96},
      {83, 33, 53, 91},
      {52, 30, 41, 85},
      {21, 28, 97, 98},
      {90, 37, 79, 75},
      {66, 45, 61, 82},
      {48, 54, 93, 65},
      {97, 62, 81, 65},
      {66, 71, 69, 55},
      {92, 80, 91, 50},
  };
  size_t i;

  if (instance - 1 > 15) {
    coco_error("rw_top_trumps_set_bounds(): instance number %lu not supported", instance);
  }

  for (i = 0; i < problem->number_of_variables; i++) {
    problem->smallest_values_of_interest[i] = low_bound[instance - 1][i % 4];
    problem->largest_values_of_interest[i] = high_bound[instance - 1][i % 4];
  }
}

/**
 * @brief Creates a single- or bi-objective rw-top-trumps problem.
 */
static coco_problem_t *rw_top_trumps_problem_allocate(const size_t number_of_objectives,
                                                      const size_t function,
                                                      const size_t dimension,
                                                      const size_t instance,
                                                      const char *problem_id_template,
                                                      const char *problem_name_template) {

  coco_problem_t *problem = NULL;

  if ((number_of_objectives != 1) && (number_of_objectives != 2))
      coco_error("rw_top_trumps_problem_allocate(): %lu objectives are not supported (only 1 or 2)",
        (unsigned long)number_of_objectives);

  problem = coco_problem_allocate(dimension, number_of_objectives, 0);
  rw_top_trumps_set_bounds(problem, instance);

  problem->number_of_integer_variables = dimension;
  problem->evaluate_function = socket_evaluate;

  coco_problem_set_id(problem, problem_id_template, function, instance, dimension);
  coco_problem_set_name(problem, problem_name_template, function, instance, dimension);
  coco_problem_set_type(problem, "rw-top-trumps");

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
  /* Need to provide estimation for the ideal and nadir points in the bi-objective case */
  else if (number_of_objectives == 2) {
    problem->best_value[0] = -1;
    problem->best_value[1] = -1;
    problem->nadir_value[0] = 0;
    problem->nadir_value[1] = 0;
  }

  return problem;
}


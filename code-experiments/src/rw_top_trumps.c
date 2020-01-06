#include "rw_top_trumps.h" /* File copied here during building of the top trumps problem */
#include "coco.h"
#include "coco_platform.h"
#include "socket_communication.c"

#define EVALUATE_RW_TOP_TRUMPS 0              /* Value can be modified through do.py */

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
#if EVALUATE_RW_TOP_TRUMPS > 0
  rw_top_trumps_bounds(instance, dimension,
      problem->smallest_values_of_interest,
      problem->largest_values_of_interest);
#endif

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


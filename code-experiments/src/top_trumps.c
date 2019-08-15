/**
 * @file top_trumps.c
 *
 * @brief Calling suite
 */

#include "coco.h"
#include "coco_platform.h"
#include "socket_communication.c"

/**
 * @brief Creates a single- or bi-objective top-trumps problem.
 */
static coco_problem_t *top_trumps_problem_allocate(const size_t number_of_objectives,
                                                   const size_t function,
                                                   const size_t dimension,
                                                   const size_t instance,
                                                   const char *problem_id_template,
                                                   const char *problem_name_template) {

  coco_problem_t *problem = NULL;
  size_t i;

  if ((number_of_objectives != 1) && (number_of_objectives != 2))
      coco_error("top-trumps_problem_allocate(): %lu objectives are not supported (only 1 or 2)",
        (unsigned long)number_of_objectives);

  problem = coco_problem_allocate(dimension, number_of_objectives, 0);
  for (i = 0; i < dimension; ++i) {
    problem->smallest_values_of_interest[i] = -5;
    problem->largest_values_of_interest[i] = 5;
  }
  problem->number_of_integer_variables = 0;
  problem->evaluate_function = socket_evaluate;

  coco_problem_set_id(problem, problem_id_template, function, instance, dimension);
  coco_problem_set_name(problem, problem_name_template, function, instance, dimension);

  if (number_of_objectives == 1) {
    problem->best_value[0] = -1;
  }
  /* Need to provide estimation for the ideal and nadir points in the bi-objective case */
  else if (number_of_objectives == 2) {
    problem->best_value[0] = -1;
    problem->best_value[1] = -1;
    problem->nadir_value[0] = 0;
    problem->nadir_value[1] = 0;
  }
    
  for (i = 0; i < dimension; ++i)
    problem->best_parameter[i] = 0;

  return problem;
}


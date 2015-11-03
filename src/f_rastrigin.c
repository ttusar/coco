#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "coco.h"
#include "coco_problem.c"

static void private_f_rastrigin_evaluate(coco_problem_t *self, const double *x, double *y) {
  size_t i;
  double sum1 = 0.0, sum2 = 0.0;
  assert(self->number_of_objectives == 1);

  for (i = 0; i < self->number_of_variables; ++i) {
    sum1 += cos(coco_two_pi * x[i]);
    sum2 += x[i] * x[i];
  }
  y[0] = 10.0 * ((double) (long) self->number_of_variables - sum1) + sum2;
}

static coco_problem_t *f_rastrigin(const size_t number_of_variables) {
  size_t i, problem_id_length;
  coco_problem_t *problem = coco_problem_allocate(number_of_variables, 1, 0);
  problem->problem_name = coco_strdup("rastrigin function");
  problem_id_length = (size_t) snprintf(NULL, 0, "%s_%02lu", "rastrigin", (long) number_of_variables);
  problem->problem_id = (char *) coco_allocate_memory(problem_id_length + 1);
  snprintf(problem->problem_id, problem_id_length + 1, "%s_%02lu", "rastrigin", (long) number_of_variables);
  problem->number_of_variables = number_of_variables;
  problem->number_of_objectives = 1;
  problem->number_of_constraints = 0;
  problem->evaluate_function = private_f_rastrigin_evaluate;
  for (i = 0; i < number_of_variables; ++i) {
    problem->smallest_values_of_interest[i] = -5.0;
    problem->largest_values_of_interest[i] = 5.0;
    problem->best_parameter[i] = 0.0;
  }
  /* Calculate best parameter value */
  private_f_rastrigin_evaluate(problem, problem->best_parameter, problem->best_value);

  return problem;
}

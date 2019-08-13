/**
 * @file logger_bbob.c
 * @brief Implementation of the bbob logger.
 *
 * Logs the performance of a single-objective optimizer on noisy or noiseless problems.
 * It produces four kinds of files:
 * - The "info" files ...
 * - The "dat" files ...
 * - The "tdat" files ...
 * - The "rdat" files ...
 */

/* TODOs:
 * - Can we get rid of dimensions_in_current_info_file
 * - Document this file in doxygen style!
 */

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <errno.h>

#include "coco.h"

#include "coco_utilities.c"
#include "coco_problem.c"
#include "coco_string.c"
#include "observer_bbob_new.c"

/** @brief Factor used in logged indicator (f-f*)^+ + sum_i g_i^+ in front of the sum */
#define LOGGER_BBOB_WEIGHT_CONSTRAINTS 1e0

/**
 * @brief The bbob logger data type.
 */
typedef struct {
  coco_observer_t *observer;
  int is_initialized;
  /*char *path;// relative path to the data folder. //Wassim: now fetched from the observer */
  /*const char *alg_name; the alg name, for now, temporarily the same as the path. Wassim: Now in the observer */
  FILE *info_file;                           /**< @brief Index file */
  FILE *dat_file;                            /**< @brief File with function value aligned data */
  FILE *tdat_file;                           /**< @brief File with number of function evaluations aligned data */
  FILE *rdat_file;                           /**< @brief File with restart information */
  size_t number_of_evaluations;
  size_t number_of_evaluations_constraints;
  double best_fvalue;
  double last_fvalue;
  short written_last_eval; /* allows writing the data of the final fun eval in the .tdat file if not already written by the t_trigger*/
  double *best_solution;
  /* The following are to only pass data as a parameter in the free function. The
   * interface should probably be the same for all free functions so passing the
   * problem as a second parameter is not an option even though we need info
   * form it.*/
  size_t function;                           /**< @brief Suite-dependent function number */
  size_t instance;                           /**< @brief Suite-dependent instance number */
  size_t number_of_variables;                /**< @brief Number of all variables */
  size_t number_of_integer_variables;        /**< @brief Number of integer variables */
  int log_discrete_as_int;                   /**< @brief Whether to output discrete variables in int or double format. */
  double optimal_fvalue;
  char *suite_name;

  size_t info_file_first_instance;           /**< @brief First instance in the info file */
  char *info_file_first_instance_string;     /**< @brief String representation of the first instance in the info file */
  /* a possible solution: have a list of dims that are already in the file, if the ones we're about to log
   * is != bbob_current_dim and the funId is currend_funId, create a new .info file with as suffix the
   * number of the first instance */
  size_t number_of_dimensions;               /**< @brief Number of dimensions in the suite */
  size_t *dimensions_in_current_info_file;

  coco_observer_targets_t *targets;          /**< @brief Triggers based on target values. */
  coco_observer_evaluations_t *evaluations;  /**< @brief Triggers based on the number of evaluations. */

} logger_bbob_new_data_t;

/**
 * @brief Discretized constraint value, ~8 + log10(c), in a single digit.
 *
 * -\infty..0 -> 0
 *    0..1e-7 -> 1
 * 1e-7..1e-6 -> 2
 *    ...
 * 1e-1..1    -> 8
 *   >1       -> 9
 */
static int single_digit_constraint_value_new(const double c) {
  const double limits[9] = {0, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1, 1};
  int i;

  for (i = 0; i < 9; ++i)
    if (c <= limits[i])
      return i;
  return 9;
}

/* was (old):
 * function evaluation |
 * noise-free fitness - Fopt (7.948000000000e+01) |
 * best noise-free fitness - Fopt |
 * measured fitness |
 * best measured fitness |
 * x1 | x2...
 was (bbob-new):
    "f evaluations | "
    "g evaluations | "
    "best noise-free fitness - Fopt | "
    "noise-free fitness - Fopt (%13.12e) | "
    "measured fitness | "
    "best measured fitness | "
    "x1 | "
    "x2...\n";
 */
static const char *bbob_new_file_header_str = "%% "
    "f evaluations | "
    "g evaluations | "
    "best noise-free fitness - Fopt (%13.12e) + sum g_i+ | "
    "measured fitness | "
    "best measured fitness or single-digit g-values | "
    "x1 | "
    "x2...\n";

static const char *logger_name_new = "bbob-new";
static const char *data_format_new = "bbob-new2"; /* back to 5 columns, 5-th column writes single digit constraint values */

/**
 * adds a formated line to a data file
 */
static void logger_bbob_new_write_data(FILE *target_file,
                                   size_t number_of_f_evaluations,
                                   size_t number_of_cons_evaluations,
                                   double fvalue,
                                   double best_fvalue,
                                   double best_value,
                                   const double *x,
                                   size_t number_of_variables,
                                   size_t number_of_integer_variables,
                                   const double *constraints,
                                   size_t number_of_constraints,
                                   const int log_discrete_as_int) {
  size_t i;
  /* for some reason, it's %.0f in the old code instead of the 10.9e
   * in the documentation
   */
  fprintf(target_file, "%lu %lu %+10.9e %+10.9e ",
          (unsigned long) number_of_f_evaluations,
    	  (unsigned long) number_of_cons_evaluations,
          best_fvalue - best_value,
    	  fvalue);

  if (number_of_constraints > 0)
    for (i = 0; i < number_of_constraints; ++i)
      fprintf(target_file, "%d",
              constraints ? single_digit_constraint_value_new(constraints[i])
                          : (int) (i % 10)); /* print 01234567890123..., may happen in last line of .tdat */
  else
    fprintf(target_file, "%+10.9e", best_fvalue);

  if ((number_of_variables - number_of_integer_variables) < 22) {
    for (i = 0; i < number_of_variables; i++) {
      if ((i < number_of_integer_variables) && (log_discrete_as_int))
        fprintf(target_file, " %d", coco_double_to_int(x[i]));
      else
        fprintf(target_file, " %+5.4e", x[i]);
    }
  }
  fprintf(target_file, "\n");

  /* Flush output so that impatient users can see progress.
   * Otherwise it can take a long time until the output appears.
   */
  fflush(target_file);
}

/**
 * @brief Opens the file in append mode
 */
static void logger_bbob_open_file(FILE **file, const char *file_path) {
  if (*file == NULL) {
    *file = fopen(file_path, "a+");
    if (*file == NULL) {
      coco_error("logger_bbob_open_file(): Error opening file: %s\nError: %d", file_path, errno);
    }
  }
}

/**
 * Creates the data files or simply opens it
 */
static void logger_bbob_new_open_dataFile(FILE **target_file,
                                      const char *path,
                                      const char *dataFile_path,
                                      const char *file_extension) {
  char file_path[COCO_PATH_MAX + 2] = { 0 };
  char relative_filePath[COCO_PATH_MAX + 2] = { 0 };
  strncpy(relative_filePath, dataFile_path,
  COCO_PATH_MAX - strlen(relative_filePath) - 1);
  strncat(relative_filePath, file_extension,
  COCO_PATH_MAX - strlen(relative_filePath) - 1);
  coco_join_path(file_path, sizeof(file_path), path, relative_filePath, NULL);
  logger_bbob_open_file(target_file, file_path);
}

/**
 * Creates the index file fileName_prefix+problem_id+file_extension in
 * folder_path
 */
static void logger_bbob_new_openIndexFile(logger_bbob_new_data_t *logger,
                                      const char *folder_path,
                                      const char *function,
                                      const char *dataFile_path,
                                      const char *suite_name) {
  /* to add the instance number TODO: this should be done outside to avoid redoing this for the .*dat files */
  char used_dataFile_path[COCO_PATH_MAX + 2] = { 0 };
  int newLine = 0; /* newLine is at 1 if we need a new line in the info file */
  char *function_string; /* TODO: consider adding them to logger */
  char file_name[COCO_PATH_MAX + 2] = { 0 };
  char file_path[COCO_PATH_MAX + 2] = { 0 };
  FILE **target_file;
  FILE *tmp_file;
  observer_bbob_new_data_t *observer_data;

  coco_debug("Started logger_bbob_new_openIndexFile()");

  assert(logger != NULL);
  assert(logger->observer != NULL);
  observer_data = ((observer_bbob_new_data_t *)((coco_observer_t *)logger->observer)->data);
  assert (observer_data != NULL);

  strncpy(used_dataFile_path, dataFile_path, COCO_PATH_MAX - strlen(used_dataFile_path) - 1);
  if (logger->info_file_first_instance == 0) {
    logger->info_file_first_instance = logger->instance;
  }
  function_string = coco_strdupf("%lu", (unsigned long) logger->function);
  logger->info_file_first_instance_string = coco_strdupf("%lu", (unsigned long) logger->info_file_first_instance);
  target_file = &(logger->info_file);
  tmp_file = NULL; /* to check whether the file already exists. Don't want to use target_file */
  strncpy(file_name, observer_data->prefix, COCO_PATH_MAX - strlen(file_name) - 1);
  strncat(file_name, "_f", COCO_PATH_MAX - strlen(file_name) - 1);
  strncat(file_name, function_string, COCO_PATH_MAX - strlen(file_name) - 1);
  strncat(file_name, "_i", COCO_PATH_MAX - strlen(file_name) - 1);
  strncat(file_name, logger->info_file_first_instance_string, COCO_PATH_MAX - strlen(file_name) - 1);
  strncat(file_name, ".info", COCO_PATH_MAX - strlen(file_name) - 1);
  coco_join_path(file_path, sizeof(file_path), folder_path, file_name, NULL);
  if (*target_file == NULL) {
    tmp_file = fopen(file_path, "r"); /* to check for existence */
    if ((tmp_file) && (observer_data->current_dim == logger->number_of_variables)
        && (observer_data->current_fun == logger->function)) {
        /* new instance of current funId and current dim */
      newLine = 0;
      logger_bbob_open_file(target_file, file_path);
      fclose(tmp_file);
    } else { /* either file doesn't exist (new funId) or new Dim */
      /* check that the dim was not already present earlier in the file, if so, create a new info file */
      if (observer_data->current_dim != logger->number_of_variables) {
        int i, j;
        for (i = 0;
            i < logger->number_of_dimensions && logger->dimensions_in_current_info_file[i] != 0
                && logger->dimensions_in_current_info_file[i] != logger->number_of_variables; i++) {
          ; /* checks whether dimension already present in the current infoFile */
        }
        if (i < logger->number_of_dimensions && logger->dimensions_in_current_info_file[i] == 0) {
          /* new dimension seen for the first time */
          logger->dimensions_in_current_info_file[i] = logger->number_of_variables;
          newLine = 1;
        } else {
          if (i < logger->number_of_dimensions) { /* dimension already present, need to create a new file */
            newLine = 0;
            file_path[strlen(file_path) - strlen(logger->info_file_first_instance_string) - 7] = 0; /* truncate the instance part */
            logger->info_file_first_instance = logger->instance;
            coco_free_memory(logger->info_file_first_instance_string);
            logger->info_file_first_instance_string = coco_strdupf("%lu", (unsigned long) logger->info_file_first_instance);
            strncat(file_path, "_i", COCO_PATH_MAX - strlen(file_name) - 1);
            strncat(file_path, logger->info_file_first_instance_string, COCO_PATH_MAX - strlen(file_name) - 1);
            strncat(file_path, ".info", COCO_PATH_MAX - strlen(file_name) - 1);
          } else { /* we have all dimensions */
            newLine = 1;
          }
          for (j = 0; j < logger->number_of_dimensions; j++) { /* new info file, reinitialize list of dims */
            logger->dimensions_in_current_info_file[j] = 0;
          }
          logger->dimensions_in_current_info_file[i] = logger->number_of_variables;
        }
      } else {
        if (observer_data->current_fun != logger->function) {
          /*new function in the same file */
          newLine = 1;
        }
      }
      /* in any case, we append */
      logger_bbob_open_file(target_file, file_path);
      if (tmp_file) { /* File already exists, new dim so just a new line. Also, close the tmp_file */
        if (newLine) {
          fprintf(*target_file, "\n");
        }
        fclose(tmp_file);
      }
      /* data_format = coco_strdup("bbob-constrained"); */
      fprintf(*target_file,
              "suite = '%s', funcId = %d, DIM = %lu, Precision = %.3e, algId = '%s', coco_version = '%s', logger = '%s', data_format = '%s'\n",
              suite_name,
              (int) strtol(function, NULL, 10),
              (unsigned long) logger->number_of_variables,
              pow(10, -8),
              logger->observer->algorithm_name,
              coco_version,
              logger_name_new,
              data_format_new);

      fprintf(*target_file, "%%\n");
      strncat(used_dataFile_path, "_i", COCO_PATH_MAX - strlen(used_dataFile_path) - 1);
      strncat(used_dataFile_path, logger->info_file_first_instance_string,
      COCO_PATH_MAX - strlen(used_dataFile_path) - 1);
      fprintf(*target_file, "%s.dat", used_dataFile_path); /* dataFile_path does not have the extension */
      observer_data->current_dim = logger->number_of_variables;
      observer_data->current_fun = logger->function;
    }
  }
  coco_free_memory(function_string);

  coco_debug("Ended   logger_bbob_new_openIndexFile()");
}

/**
 * Generates the different files and folder needed by the logger to store the
 * data if these don't already exist
 */
static void logger_bbob_new_initialize(logger_bbob_new_data_t *logger, coco_problem_t *inner_problem) {
  /*
   Creates/opens the data and index files
   */
  char dataFile_path[COCO_PATH_MAX + 2] = { 0 }; /* relative path to the .dat file from where the .info file is */
  char folder_path[COCO_PATH_MAX + 2] = { 0 };
  char *tmpc_funId; /* serves to extract the function id as a char *. There should be a better way of doing this! */
  char *tmpc_dim; /* serves to extract the dimension as a char *. There should be a better way of doing this! */
  observer_bbob_new_data_t *observer_data;

  coco_debug("Started logger_bbob_new_initialize()");

  assert(logger != NULL);
  assert(logger->observer != NULL);
  observer_data = logger->observer->data;
  assert (observer_data != NULL);

  assert(inner_problem != NULL);
  assert(inner_problem->problem_id != NULL);

  tmpc_funId = coco_strdupf("%lu", (unsigned long) coco_problem_get_suite_dep_function(inner_problem));
  tmpc_dim = coco_strdupf("%lu", (unsigned long) inner_problem->number_of_variables);

  /* prepare paths and names */
  strncpy(dataFile_path, "data_f", COCO_PATH_MAX);
  strncat(dataFile_path, tmpc_funId,
  COCO_PATH_MAX - strlen(dataFile_path) - 1);
  coco_join_path(folder_path, sizeof(folder_path), logger->observer->result_folder, dataFile_path,
  NULL);
  coco_create_directory(folder_path);
  strncat(dataFile_path, "/bbobexp_f",
  COCO_PATH_MAX - strlen(dataFile_path) - 1);
  strncat(dataFile_path, tmpc_funId,
  COCO_PATH_MAX - strlen(dataFile_path) - 1);
  strncat(dataFile_path, "_DIM", COCO_PATH_MAX - strlen(dataFile_path) - 1);
  strncat(dataFile_path, tmpc_dim, COCO_PATH_MAX - strlen(dataFile_path) - 1);

  /* index/info file */
  assert(coco_problem_get_suite(inner_problem));
  logger_bbob_new_openIndexFile(logger, logger->observer->result_folder, tmpc_funId,
      dataFile_path, coco_problem_get_suite(inner_problem)->suite_name);
  fprintf(logger->info_file, ", %lu", (unsigned long) coco_problem_get_suite_dep_instance(inner_problem));
  /* data files */
  /* TODO: definitely improvable but works for now */
  strncat(dataFile_path, "_i", COCO_PATH_MAX - strlen(dataFile_path) - 1);
  strncat(dataFile_path, logger->info_file_first_instance_string,
  COCO_PATH_MAX - strlen(dataFile_path) - 1);

  logger_bbob_new_open_dataFile(&(logger->dat_file), logger->observer->result_folder, dataFile_path, ".dat");
  fprintf(logger->dat_file, bbob_new_file_header_str, logger->optimal_fvalue);

  logger_bbob_new_open_dataFile(&(logger->tdat_file), logger->observer->result_folder, dataFile_path, ".tdat");
  fprintf(logger->tdat_file, bbob_new_file_header_str, logger->optimal_fvalue);

  logger_bbob_new_open_dataFile(&(logger->rdat_file), logger->observer->result_folder, dataFile_path, ".rdat");
  fprintf(logger->rdat_file, bbob_new_file_header_str, logger->optimal_fvalue);
  logger->is_initialized = 1;
  coco_free_memory(tmpc_dim);
  coco_free_memory(tmpc_funId);
  coco_free_memory(logger->info_file_first_instance_string);

  coco_debug("Ended   logger_bbob_new_initialize()");
}

/**
 * Layer added to the transformed-problem evaluate_function by the logger
 */
static void logger_bbob_new_evaluate(coco_problem_t *problem, const double *x, double *y) {
  size_t i;
  double y_logged, max_fvalue, sum_cons;
  double *cons = NULL;
  logger_bbob_new_data_t *logger = (logger_bbob_new_data_t *) coco_problem_transformed_get_data(problem);
  coco_problem_t *inner_problem = coco_problem_transformed_get_inner_problem(problem);
  const int is_feasible = problem->number_of_constraints <= 0
                            || coco_is_feasible(inner_problem, x, NULL);

  coco_debug("Started logger_bbob_new_evaluate()");

  if (!logger->is_initialized) {
    logger_bbob_new_initialize(logger, inner_problem);
  }
  if ((coco_log_level >= COCO_DEBUG) && logger->number_of_evaluations == 0) {
    coco_debug("%4lu: ", (unsigned long) inner_problem->suite_dep_index);
    coco_debug("on problem %s ... ", coco_problem_get_id(inner_problem));
  }

  coco_evaluate_function(inner_problem, x, y); /* fulfill contract as "being" a coco evaluate function */

  logger->number_of_evaluations_constraints = coco_problem_get_evaluations_constraints(problem);
  logger->number_of_evaluations++; /* could be != coco_problem_get_evaluations(problem) for non-anytime logging? */
  logger->written_last_eval = 0; /* flag whether the current evaluation was logged? */
  logger->last_fvalue = y[0]; /* asma: should be: max(y[0], logger->optimal_fvalue) */

  y_logged = y[0];
  if (coco_is_nan(y_logged))
    y_logged = NAN_FOR_LOGGING;
  else if (coco_is_inf(y_logged))
    y_logged = INFINITY_FOR_LOGGING;
  /* do sanity check */
  if (is_feasible)  /* infeasible solutions can have much better y0 values */
    assert(y_logged + 1e-13 >= logger->optimal_fvalue);

  /* Evaluate the constraints */
  if (problem->number_of_constraints > 0) {
    cons = coco_allocate_vector(problem->number_of_constraints);
    inner_problem->evaluate_constraint(inner_problem, x, cons);
  }

  /* Compute the sum of positive constraint values */
  sum_cons = 0;
  for (i = 0; i < problem->number_of_constraints; ++i) {
    if (cons[i] > 0)
        sum_cons += cons[i];
  }
  sum_cons *= LOGGER_BBOB_WEIGHT_CONSTRAINTS;  /* do this before the checks */
  if (coco_is_nan(sum_cons))
    sum_cons = NAN_FOR_LOGGING;
  else if (coco_is_inf(sum_cons))
    sum_cons = INFINITY_FOR_LOGGING;

  max_fvalue =  y_logged > logger->optimal_fvalue ? y_logged : logger->optimal_fvalue;

  /* Update logger state.
   *   at logger->number_of_evaluations == 1 the logger->best_fvalue is not initialized,
   *   also compare to y_logged to not potentially be thrown off by weird values in y[0]
   */
  if (logger->number_of_evaluations == 1 || (max_fvalue + sum_cons < logger->best_fvalue)) {
    logger->best_fvalue = max_fvalue + sum_cons;
    for (i = 0; i < problem->number_of_variables; i++)
      logger->best_solution[i] = x[i]; /* may well be infeasible */

    /* Add a line in the .dat file for each logging target reached
     * by a feasible solution and always at evaluation one
     */
    if (logger->number_of_evaluations == 1 || coco_observer_targets_trigger(logger->targets,
                                        logger->best_fvalue - logger->optimal_fvalue)) {
      logger_bbob_new_write_data(
          logger->dat_file,
          logger->number_of_evaluations,
          logger->number_of_evaluations_constraints,
          y_logged,
          logger->best_fvalue,
          logger->optimal_fvalue,
          x,
          problem->number_of_variables,
          problem->number_of_integer_variables,
          cons,
          problem->number_of_constraints,
          logger->log_discrete_as_int);
    }
  }

  /* Add a line in the .tdat file each time an fevals trigger is reached.*/
  if (coco_observer_evaluations_trigger(logger->evaluations,
        logger->number_of_evaluations + logger->number_of_evaluations_constraints)) {
    logger_bbob_new_write_data(
        logger->tdat_file,
        logger->number_of_evaluations,
        logger->number_of_evaluations_constraints,
        y_logged,
        logger->best_fvalue,
        logger->optimal_fvalue,
        x,
        problem->number_of_variables,
        problem->number_of_integer_variables,
        cons,
        problem->number_of_constraints,
        logger->log_discrete_as_int);
    logger->written_last_eval = 1;
  }

  /* Free allocated memory */
  if (problem->number_of_constraints > 0)
    coco_free_memory(cons);

  coco_debug("Ended   logger_bbob_new_evaluate()");

}  /* end logger_bbob_evaluate */

/**
 * Also serves as a finalize run method so. Must be called at the end
 * of Each run to correctly fill the index file
 *
 * TODO: make sure it is called at the end of each run or move the
 * writing into files to another function
 */
static void logger_bbob_new_free(void *stuff) {
  /* TODO: do all the "non simply freeing" stuff in another function
   * that can have problem as input
   */
  logger_bbob_new_data_t *logger = (logger_bbob_new_data_t *) stuff;

  coco_debug("Started logger_bbob_new_free()");

  if ((coco_log_level >= COCO_DEBUG) && logger && logger->number_of_evaluations > 0) {
    coco_debug("best f=%e after %lu fevals (done observing)\n", logger->best_fvalue,
    		(unsigned long) logger->number_of_evaluations);
  }
  if (logger->info_file != NULL) {
    fprintf(logger->info_file, ":%lu|%.1e",
            (unsigned long) logger->number_of_evaluations,
            logger->best_fvalue - logger->optimal_fvalue);
    fclose(logger->info_file);
    logger->info_file = NULL;
  }
  if (logger->dat_file != NULL) {
    fclose(logger->dat_file);
    logger->dat_file = NULL;
  }
  if (logger->tdat_file != NULL) {
    /* TODO: make sure it handles restarts well. i.e., it writes
     * at the end of a single run, not all the runs on a given
     * instance. Maybe start with forcing it to generate a new
     * "instance" of problem for each restart in the beginning
     */
    if (!logger->written_last_eval) {
      logger_bbob_new_write_data(logger->tdat_file,
          logger->number_of_evaluations,
          logger->number_of_evaluations_constraints,
          logger->best_fvalue,
          logger->best_fvalue,
          logger->optimal_fvalue,
          logger->best_solution,
          logger->number_of_variables,
          logger->number_of_integer_variables,
          NULL,
          0,
          logger->log_discrete_as_int);
	}
    fclose(logger->tdat_file);
    logger->tdat_file = NULL;
  }

  if (logger->rdat_file != NULL) {
    fclose(logger->rdat_file);
    logger->rdat_file = NULL;
  }

  if (logger->best_solution != NULL) {
    coco_free_memory(logger->best_solution);
    logger->best_solution = NULL;
  }

  if (logger->dimensions_in_current_info_file != NULL) {
    coco_free_memory(logger->dimensions_in_current_info_file);
    logger->dimensions_in_current_info_file = NULL;
  }

  if (logger->targets != NULL){
    coco_free_memory(logger->targets);
    logger->targets = NULL;
  }

  if (logger->evaluations != NULL){
    coco_observer_evaluations_free(logger->evaluations);
    logger->evaluations = NULL;
  }

  coco_debug("Ended   logger_bbob_new_free()");
}

static coco_problem_t *logger_bbob_new(coco_observer_t *observer, coco_problem_t *inner_problem) {
  coco_problem_t *problem;
  logger_bbob_new_data_t *logger_data;
  size_t i;

  coco_debug("Started logger_bbob_new()");

  logger_data = (logger_bbob_new_data_t *) coco_allocate_memory(sizeof(*logger_data));
  logger_data->observer = observer;

  if (inner_problem->number_of_objectives != 1) {
    coco_warning("logger_bbob_new(): The bbob logger shouldn't be used to log a problem with %d objectives",
        inner_problem->number_of_objectives);
  }

  /* This is the name of the folder which happens to be the algName */
  /*logger->path = coco_strdup(observer->output_folder);*/
  logger_data->info_file = NULL;
  logger_data->dat_file = NULL;
  logger_data->tdat_file = NULL;
  logger_data->rdat_file = NULL;
  logger_data->number_of_variables = inner_problem->number_of_variables;
  logger_data->number_of_integer_variables = inner_problem->number_of_integer_variables;
  if (inner_problem->best_value == NULL) {
    /* coco_error("Optimal f value must be defined for each problem in order for the logger to work properly"); */
    /* Setting the value to 0 results in the assertion y>=optimal_fvalue being susceptible to failure */
    coco_warning("undefined optimal f value. Set to 0");
    logger_data->optimal_fvalue = 0;
  } else {
    logger_data->optimal_fvalue = *(inner_problem->best_value);
  }

  logger_data->number_of_evaluations = 0;
  logger_data->number_of_evaluations_constraints = 0;
  logger_data->best_solution = coco_allocate_vector(inner_problem->number_of_variables);
  /* TODO: the following inits are just to be in the safe side and
   * should eventually be removed. Some fields of the bbob_logger struct
   * might be useless
   */
  logger_data->function = coco_problem_get_suite_dep_function(inner_problem);
  logger_data->instance = coco_problem_get_suite_dep_instance(inner_problem);
  logger_data->written_last_eval = 0;
  logger_data->last_fvalue = DBL_MAX;
  logger_data->is_initialized = 0;
  logger_data->log_discrete_as_int = observer->log_discrete_as_int;

  assert(inner_problem);
  assert(inner_problem->suite);
  logger_data->info_file_first_instance = 0;
  logger_data->info_file_first_instance_string = NULL;
  logger_data->number_of_dimensions = ((coco_suite_t *) inner_problem->suite)->number_of_dimensions;
  logger_data->dimensions_in_current_info_file = coco_allocate_vector_size_t(logger_data->number_of_dimensions);
  for (i = 0; i < logger_data->number_of_dimensions; i++)
    logger_data->dimensions_in_current_info_file[i] = 0;
    
  /* Initialize triggers based on target values and number of evaluations */
  logger_data->targets = coco_observer_targets(observer->number_target_triggers, observer->target_precision);
  logger_data->evaluations = coco_observer_evaluations(observer->base_evaluation_triggers, inner_problem->number_of_variables);

  problem = coco_problem_transformed_allocate(inner_problem, logger_data, logger_bbob_new_free, observer->observer_name);

  problem->evaluate_function = logger_bbob_new_evaluate;

  coco_debug("Ended   logger_bbob_new()");
  return problem;
}

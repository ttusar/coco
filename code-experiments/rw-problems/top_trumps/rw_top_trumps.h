#ifndef RW_TOP_TRUMPS_H_
#define RW_TOP_TRUMPS_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void evaluate_rw_top_trumps(char *suite_name, size_t number_of_objectives, size_t function,
    size_t instance, size_t dimension, const double *x, double *y);

void rw_top_trumps_bounds(size_t instance, size_t size_x, double *lower_bounds, double *upper_bounds);

void rw_top_trumps_test(void);

#ifdef __cplusplus
}
#endif

#endif /* RW_TOP_TRUMPS_H_ */

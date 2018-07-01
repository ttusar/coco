/*
 * rw_gan_mario_test.c
 *
 *  Created on: 1. jul. 2018
 *      Author: Tea Tusar
 */
#include <Python.h>
#include "ganmariolib.h"

int main() {
  Py_Initialize();
  PyInit_ganmariolib();
  rw_gan_mario_evaluate();
  Py_Finalize();

  if (PyErr_Occurred())
  {
      PyErr_Print();
      return -1;
  }
  return 0;
}


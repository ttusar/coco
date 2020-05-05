#!/usr/bin/env python
"""
An example experiment for real-world problems that use sockets for evaluating solutions.
Showcases the use of two observers (the 'standard' bbob(-biobj) and the rw observer), batch
evaluations and restarts.

By default (without arguments), performs an experiment with the toy-socket suite.

Arguments:
    suite=SUITE_NAME                   # Suite name (default toy-socket)
    suite_options=SUITE_OPTIONS        # Suite options (default '')
    observer=OBSERVER_NAME             # Name of the observer where 'bbob(-biobj)', 'rw' and 'both'
                                       # are supported (default 'both')
    observer_options=OBSERVER_OPTIONS  # Observer options (default '')
    budget_multiplier=BUDGET           # Budget multiplier (default 10)
    batches=BATCHES                    # Number of all batches to parallelize the experiment
                                       # (default 1)
    batch=BATCH                        # This batch (default 1)
    start_port=PORT                    # Port for the first batch (default 7000)

Example:
    # Runs the 1st of 12 batches of the rw-top-trumps suite
    `python rw_example_experiment.py suite_name=rw-top-trumps batches=12 batch=1`

The socket server needs to be running when this script is called. This is done via
`python do.py run-toy-socket-server-c port=7001`
After the experiment has ended, the socket servers can be stopped via
`python do.py stop-socket-servers port=7001

To avoid having to run and stop the socket servers manually, instead of this script rather call
`python do.py test-socket <ARGUMENTS>`
where the arguments are given in the same form as shown above.
"""
import cocoex
from cocoex.solvers import random_search
import sys


def _get_socket_port(suite_name, start_port, current_batch):
    """Returns the used port based on the given parameters
    The same function is used in do.py. If this one changes, the other has to change too.
    """
    port_py_inc = 200
    if ('toy-socket' in suite_name) or ('rw-top-trumps' in suite_name):
        return start_port + current_batch
    elif 'rw-mario-gan' in suite_name:
        return start_port + port_py_inc + current_batch
    else:
        raise ValueError('Suite {} not supported'.format(suite_name))


if __name__ == '__main__':
    # These defaults should match those from do.py (and the documentation above)
    suite_name = 'toy-socket'
    current_batch = 1
    start_port = 7000
    # Other defaults
    suite_options = ''
    observer_name = 'both'
    observer_options = ''
    budget_multiplier = 10
    batches = 1
    solver = random_search  # COCO's random search only evaluates feasible solutions
    # Parse the command line arguments
    for arg in sys.argv[1:]:
        if arg[:6] == 'suite=':
            suite_name = arg[6:]
        elif arg[:14] == 'suite_options=':
            suite_options = arg[14:]
        elif arg[:9] == 'observer=':
            observer_name = arg[9:]
        elif arg[:17] == 'observer_options=':
            observer_options = arg[17:]
        elif arg[:18] == 'budget_multiplier=':
            budget_multiplier = arg[18:]
        elif arg[:8] == 'batches=':
            batches = arg[8:]
        elif arg[:6] == 'batch=':
            current_batch = arg[6:]
        if arg[:11] == 'start_port=':
            start_port = arg[11:]
    # Get the right port for this suite
    port = _get_socket_port(suite_name, start_port, current_batch)
    # Prepare the suite
    suite_options = 'port: {} {}'.format(port, suite_options)
    suite = cocoex.Suite(suite_name, '', suite_options)
    num_obj = suite[0].number_of_objectives
    # Prepare the observers
    if observer_name in ['bbob', 'bbob-biobj', 'rw']:
        observer_names = [observer_name]
    elif observer_name == 'both':
        observer_names = ['bbob', 'rw'] if num_obj == 1 else ['bbob-biobj', 'rw']
    else:
        raise ValueError('Observer name {} not supported'.format(observer_name))
    observers = [cocoex.Observer(observer_n, 'result_folder: {}-{}'.format(suite_name, observer_n))
                 for observer_n in observer_names]
    # Use minimal printing
    minimal_print = cocoex.utilities.MiniPrint()

    # Run the solver on the problems of the suite in the current batch
    for problem_index, problem in enumerate(suite):
        # Skip problems not in this batch
        if (problem_index + current_batch - 1) % batches:
            continue
        # Use all chosen observers
        for observer in observers:
            problem.observe_with(observer)
        lb = problem.lower_bounds
        ub = problem.upper_bounds
        # Equalize the bounds of integer variables
        num_int = problem.number_of_integer_variables
        if num_int > 0:
            lb[:num_int] -= 0.5
            ub[:num_int] += 0.5
        # Restart the solver while neither the problem is solved nor the budget is exhausted
        while (problem.evaluations < problem.dimension * budget_multiplier
               and not problem.final_target_hit):
            for observer in observers:
                if b'bbob' in observer.name:
                    observer.signal_restart(problem)
            solver(problem, lb, ub, budget=budget_multiplier * problem.dimension / 2)
        minimal_print(problem, final=problem.index == len(suite) - 1)

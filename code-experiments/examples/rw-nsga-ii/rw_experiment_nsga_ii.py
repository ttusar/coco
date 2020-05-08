#!/usr/bin/env python
"""
An experiment using an algorithm implemented in JMetal (NSGA-II) to solve real-world COCO problems
that use sockets for evaluating solutions.

Arguments:
    suite=SUITE_NAME                   # Suite name (default top-trumps-biobj)
    suite_options=SUITE_OPTIONS        # Suite options (default '')
    observer=OBSERVER_NAME             # Name of the observer (default 'bbob-biobj')
    observer_options=OBSERVER_OPTIONS  # Observer options (default '')
    budget_multiplier=BUDGET           # Budget multiplier (default 1)
    batches=BATCHES                    # Number of all batches to parallelize the experiment
                                       # (default 1)
    batch=BATCH                        # This batch (default 1)
    start_port=PORT                    # Port for the first batch (default 7000)

Example:
    # Runs the 1st of 4 batches of the rw-top-trumps-biobj suite
    `python rw_experiment_nsga_ii.py batches=4 batch=1`

The socket server needs to be running when this script is called. This is done via
`python do.py run-rw-top-trumps-server port=7001 batch=1`
After the experiment has ended, the socket servers can be stopped via
`python do.py stop-socket-servers port=7001
"""
import cocoex
import sys
from coco_problem import CocoProblem
from jmetal.algorithm.multiobjective import NSGAII
from jmetal.util.termination_criterion import StoppingByEvaluations
from jmetal.operator import SBXCrossover, PolynomialMutation
from jmetal.util.comparator import DominanceComparator


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
    
    
def parse_arguments(argv):
    """Parses the command-line arguments and returns a dictionary with all the arguments"""
    # These defaults should match those from do.py (and the documentation above)
    suite_name = 'rw-top-trumps-biobj'
    current_batch = 1
    start_port = 7000
    # Other defaults
    suite_options = ''
    observer_name = 'bbob-biobj'
    observer_options = ''
    budget_multiplier = 10
    batches = 1
    # Parse the command line arguments
    for arg in argv[1:]:
        if arg[:6] == 'suite=':
            suite_name = arg[6:]
        elif arg[:14] == 'suite_options=':
            suite_options = arg[14:]
        elif arg[:9] == 'observer=':
            observer_name = arg[9:]
        elif arg[:17] == 'observer_options=':
            observer_options = arg[17:]
        elif arg[:18] == 'budget_multiplier=':
            budget_multiplier = int(arg[18:])
        elif arg[:8] == 'batches=':
            batches = int(arg[8:])
        elif arg[:6] == 'batch=':
            current_batch = int(arg[6:])
        elif arg[:11] == 'start_port=':
            start_port = int(arg[11:])
    # Get the right port for this suite
    port = _get_socket_port(suite_name, start_port, current_batch)
    # Print out the options
    print('RW experiment ran with the following options:')
    print('suite = {}'.format(suite_name))
    print('suite_options = {}'.format(suite_options))
    print('observer = {}'.format(observer_name))
    print('observer_options = {}'.format(observer_options))
    print('budget_multiplier = {}'.format(budget_multiplier))
    print('batches = {}'.format(batches))
    print('current_batch = {}'.format(current_batch))
    return dict(suite_name=suite_name, suite_options=suite_options,
                observer_name=observer_name, observer_options=observer_options,
                budget_multiplier=budget_multiplier, batches=batches,
                current_batch=current_batch, port=port)


if __name__ == '__main__':
    # Parse the command-line arguments
    params = parse_arguments(sys.argv)
    suite_name = params['suite_name']
    suite_options = params['suite_options']
    observer_name = params['observer_name']
    observer_options = params['observer_options']
    budget_multiplier = params['budget_multiplier']
    batches = params['batches']
    current_batch = params['current_batch']
    port = params['port']

    # Prepare the suite
    suite_options = 'port: {} {}'.format(params['port'], suite_options)
    suite = cocoex.Suite(suite_name, '', suite_options)
    num_obj = suite[0].number_of_objectives
    # Prepare the observer
    observer = cocoex.Observer(observer_name, '{} result_folder: {}-{}{}'.format(
        observer_options, suite_name, 'NSGA-II',
        '-batch-{}'.format(current_batch) if batches > 1 else ''))
    # Use minimal printing
    minimal_print = cocoex.utilities.MiniPrint()

    # Run the solver on the problems of the suite in the current batch
    for problem_index, problem in enumerate(suite):
        # Skip problems not in this batch
        if (problem_index + current_batch - 1) % batches:
            continue
        # Observe the problem
        problem.observe_with(observer)
        # Population size should be an even number
        population_size = int(problem.dimension / 2)
        population_size += 1 if population_size % 2 != 0 else 0
        # Use jMetalPy's NSGA-II algorithm
        algorithm = NSGAII(
            problem=CocoProblem(problem),
            population_size=population_size,
            offspring_population_size=population_size,
            mutation=PolynomialMutation(
                probability=1.0 / problem.dimension, distribution_index=20),
            crossover=SBXCrossover(probability=1.0, distribution_index=20),
            termination_criterion=StoppingByEvaluations(
                max_evaluations=budget_multiplier * problem.dimension),
            dominance_comparator=DominanceComparator()
        )
        # Restart the solver while neither the problem is solved nor the budget is exhausted
        while (problem.evaluations < problem.dimension * budget_multiplier
               and not problem.final_target_hit):
            observer.signal_restart(problem)
            algorithm.run()
        minimal_print(problem, final=problem.index == len(suite) - 1)

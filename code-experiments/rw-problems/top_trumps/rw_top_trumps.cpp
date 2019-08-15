/*
 * rw_top_trumps.cpp
 *
 *  Created on: 29. jun. 2018
 *      Author: Tea Tusar
 */
#include "rw_top_trumps.h"

#include "Simulation/Game.h"
#include <assert.h>
#include <iostream>

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int m=4;
int rep = 2000;
int players = 2;

/**
 * An evaluator for problems from the toy-socket suite to demonstrate external evaluation in C
 */
void evaluate_top_trumps(char *suite_name, size_t number_of_objectives, size_t function,
    size_t instance, size_t dimension, const double *x, double *y)
{
  int seed = (int) instance;
  srand(seed);
  int n = (int) dimension / m;

  double * lower_bounds = (double *) malloc(dimension * sizeof(double));
  double * upper_bounds = (double *) malloc(dimension * sizeof(double));

  std::vector<double> x_vector(x, x + dimension);
  top_trumps_bounds(instance, dimension, lower_bounds, upper_bounds);
  std::vector<double> min(m);
  std::vector<double> max(m);
  double maxHyp = 1;
  double maxSD = 50;
  for(size_t i = 0; i < dimension; i++){
    if(x[i] < lower_bounds[i] || upper_bounds[i] < x[i]){
      for (size_t i = 0; i < dimension; i++)
        y[i] = 0; //return high number
      return;
    } else {
      min[i] = lower_bounds[i];
      max[i] = upper_bounds[i];
      maxHyp = maxHyp * (max[i]-min[i]);
    }    
  }

  fprintf(stdout, "test: suite %s, function %lu, instance %lu, dimension %lu, objectives %lu", suite_name, function, instance, dimension, number_of_objectives);

  Deck deck(x_vector, n, m, min, max);
  if ((strcmp(suite_name, "top-trumps") == 0) && (number_of_objectives == 1) && (function <=2)) {
    if (function == 1) {
      y[0] = -deck.getHV()/maxHyp;
    } else if (function == 2) {
      y[0] = -deck.getSD()/maxSD;
    }
  } else if ((strcmp(suite_name, "top-trumps-biobj") == 0) && (number_of_objectives == 2) && (function<=1)) {
    if (function == 1) {
      y[0] = -deck.getHV()/maxHyp;
      y[1] = -deck.getSD()/maxSD;
    } 
  } else{
		std::vector<Agent> agents(players);
    std::vector<int> playerLevel1(4, 0);
    agents[0] = Agent(playerLevel1, deck);
    std::vector<int> playerLevel2(4, 1);
    agents[1] = Agent(playerLevel2, deck);

    Game game(deck, players, agents, seed);
    Outcome out(rep);
    for (int i = 0; i < rep; i++) {
      out = game.run(out, 0);
    }
	
		if ((strcmp(suite_name, "top-trumps") == 0) && (number_of_objectives == 1)) {
		  if (function == 3) {
		    y[0] = -out.getFairAgg();
		  } else if (function == 4) {
		    y[0] = -players * out.getLeadChangeAgg()/n;
		  } else if (function == 5) {
		    y[0] = out.getTrickDiffAgg()-1;
		  } else {
				fprintf(stderr, "evaluate(): suite %s does not have function %lu", suite_name, function);
				exit(EXIT_FAILURE);
		  }
		} else if ((strcmp(suite_name, "top-trumps-biobj") == 0) && (number_of_objectives == 2)) {
			if (function == 2) {
			  y[0] = -out.getFairAgg();
			  y[1] = -players * out.getLeadChangeAgg()/n;
			} else if (function == 3) {
			  y[0] = -out.getFairAgg();
			  y[1] = out.getTrickDiffAgg()-1;
	 		} else {
				fprintf(stderr, "evaluate(): suite %s does not have function %lu", suite_name, function);
				exit(EXIT_FAILURE);
	  	}
		} else {
		  fprintf(stderr, "evaluate(): suite %s cannot have %lu objectives", suite_name, number_of_objectives);
		  exit(EXIT_FAILURE);
		}
	}
}


void top_trumps_bounds(size_t instance, size_t size_x,
    double *lower_bounds, double *upper_bounds) {
    
  int seed = (int) instance;
  srand(seed);
    //set box constraints based on seed, i.e. depending on instance. Return large value if outside.
  double lbound = 0;
  double ubound=100;
  std::vector<double> min(m);
  std::vector<double> max(m);
  for(int i=0; i<m; i++){
    double a = lbound + (double)rand()/RAND_MAX*(ubound-lbound);
    double b = lbound + (double)rand()/RAND_MAX*(ubound-lbound);
    double box_min = std::min(a,b);
    double box_max = std::max(a,b);
    min[i] = std::round(box_min);
    max[i] = std::round(box_max);
  }

  for(size_t i = 0; i < size_x; i++){
    lower_bounds[i] = min[i%m];
    upper_bounds[i] = max[i%m];
    //std::cout << "lower " << lower_bounds[i] << " , upper" << upper_bounds[i] << std::endl;
  }
}

void top_trumps_test(void) {
  std::cout << "Top trumps is working!\n";
}


#ifdef __cplusplus
}
#endif


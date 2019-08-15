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

  std::vector<double> y_vector(number_of_objectives);
  std::vector<double> x_vector(x, x + dimension);

//set box constraints based on seed, i.e. depending on instance. Return large value if outside.
  double lbound = 0;
  double ubound=100;
  bool outOfBounds=false;
  std::vector<double> min(m);
  std::vector<double> max(m);
  double maxHyp = 1;
  double maxSD = 50;
  for(int i=0; i<m; i++){
    double a = lbound + (double)rand()/RAND_MAX*(ubound-lbound);
    double b = lbound + (double)rand()/RAND_MAX*(ubound-lbound);
    double box_min = std::min(a,b);
    double box_max = std::max(a,b);
    maxHyp = maxHyp * (box_max -box_min);
    min[i] = std::round(box_min);
    max[i] = std::round(box_max);
    //std::cout << "random bounds [" << min[i] << ", " << max[i] <<"]"<< std::endl;
    for(int j=0; j<n; j++){
        if(x_vector[j*m+i] <min[i] || x_vector[j*m+i] > max[i]){
            //std::cout << "boundary on " << j*m+i << std::endl;
            outOfBounds=true;
        }
    }
      
  }
  if(outOfBounds){
	  fprintf(stdout, "out of bounds");
    for (size_t i = 0; i < number_of_objectives; i++)
        y[i] = 0; //return high number
    return;
  }




  Deck deck(x_vector, n, m, min, max);
  if ((strcmp(suite_name, "top-trumps") == 0) && (number_of_objectives == 1) && (function <=2)) {
    if (function == 1) {
      y_vector[0] = -deck.getHV()/maxHyp;
    } else if (function == 2) {
      y_vector[0] = -deck.getSD()/maxSD;
    }
  } else if ((strcmp(suite_name, "top-trumps-biobj") == 0) && (number_of_objectives == 2) && (function<=1)) {
    if (function == 1) {
      y_vector[0] = -deck.getHV()/maxHyp;
      y_vector[1] = -deck.getSD()/maxSD;
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
		    y_vector[0] = -out.getFairAgg();
		  } else if (function == 4) {
		    y_vector[0] = -players * out.getLeadChangeAgg()/n;
		  } else if (function == 5) {
		    y_vector[0] = out.getTrickDiffAgg()-1;
		  } else {
				fprintf(stderr, "evaluate(): suite %s does not have function %lu", suite_name, function);
				exit(EXIT_FAILURE);
		  }
		} else if ((strcmp(suite_name, "top-trumps-biobj") == 0) && (number_of_objectives == 2)) {
			if (function == 2) {
			  y_vector[0] = -out.getFairAgg();
			  y_vector[1] = -players * out.getLeadChangeAgg()/n;
			} else if (function == 3) {
			  y_vector[0] = -out.getFairAgg();
			  y_vector[1] = out.getTrickDiffAgg()-1;
	 		} else {
				fprintf(stderr, "evaluate(): suite %s does not have function %lu", suite_name, function);
				exit(EXIT_FAILURE);
	  	}
		} else {
		  fprintf(stderr, "evaluate(): suite %s cannot have %lu objectives", suite_name, number_of_objectives);
		  exit(EXIT_FAILURE);
		}
	}
  for (size_t i = 0; i < number_of_objectives; i++)
    y[i] = y_vector[i];
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


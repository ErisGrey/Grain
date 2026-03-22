#pragma once
#include "../header/instance.h"
#include <chrono>
#include <ilcplex/ilocplex.h>

class Solution {
public:
    Solution() {
        obj = 0;
        bound = 0;
        time = 0;
        status = "No Solution";
    }

    double obj;
    double bound;
    double time; //in second;
    std::string status; //no solution, feasible or optimal
    std::string name;
    
    double time_phase1 = 0;
    float gap = 100;
    int lower_bound = 0;

};

class Model {
public:
    virtual Solution run() = 0;
};
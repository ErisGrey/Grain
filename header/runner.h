#pragma once
#include "../header/model.h"
#include "../header/params.h"
#include "fstream"

class Runner
{
private:
    Model *model;

public:
    Runner(Model *model_)
    {
        model = model_;
    }
    Solution get_sol()
    {
        return model->run();
    }
    void run()
    {
        // std::ofstream out;
        // out.open(path, std::ios_base::app);
        Solution sol = model->run();
        // out << sol.name << "," << sol.status << "," << sol.obj << "," << sol.bound << "," << sol.time << "\n";
        // std::cout << "TIME is: " << sol.time << std::endl;
    }
};
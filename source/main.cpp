#include "../header/instance.h"
#include "../header/params.h"
#include "../header/runner.h"
#include "../header/milp.h"
#include "../header/model.h"

int main(int argc, char *argv[])
{
    std::string ins_path = "/home/orlab/code/Thanh/Grain/grain_input.csv";
    while (--argc)
    {
        argv++;
        std::string s(argv[0]);
        if (s == "-i")
        {
            ins_path = std::string(argv[1]);
        }
    }
    Instance *instance = Instance::getInstance(ins_path);
    std::cout << instance->nb_grain << "\n";

    for (int g = 0; g < instance->nb_grain; g++)
    {
        std::cout << instance->grains[g].cost << "\n";
    }

    Model *model = nullptr;
    model = new MILP();
    Runner runner(model);
    runner.run();
    return 0;
}
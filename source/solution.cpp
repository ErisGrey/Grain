// #include "../header/solution.h"
// #include "../header/instance.h"

// Sol::Sol()
// {
//     Instance *instance = Instance::getInstance();
//     truck_makespan = 0;
//     drone_makespan = 0;
//     make_span = 0;
//     truck_tour = {0, 0};
//     truck_tour.reserve(instance->num_customers);

//     resupply_drone.resize(1); // num drone = 1
//     drone_tour.resize(1);
// }

// // Sol& Sol::operator=(const Sol& s) {
// //     // Assignment operator code here
// //     return *this;
// // }

// std::vector<int> Sol::getTruckTour()
// {
//     return truck_tour;
// }

// void Sol::setTruckTour(const std::vector<int> &tour)
// {
//     truck_tour = tour;
// }

// std::vector<std::vector<std::vector<int>>> Sol::getResupplyDrone()
// {
//     return resupply_drone;
// }

// void Sol::setResupplyDrone(const std::vector<std::vector<std::vector<int>>> &drone)
// {
//     resupply_drone = drone;
// }

// std::vector<std::vector<int>> Sol::getDroneTour()
// {
//     return drone_tour;
// }

// void Sol::setDroneTour(const std::vector<std::vector<int>> &tour)
// {
//     drone_tour = tour;
// }

// // Define getObjective, isFeasible, greedy, etc., here

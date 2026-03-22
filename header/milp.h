#pragma once
#include "../header/model.h"
#include "../header/params.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

typedef IloArray<IloArray<IloNumVarArray>> NumVar3D;
typedef IloArray<IloArray<IloNumArray>> Num3D;
typedef IloArray<IloNumVarArray> NumVar2D;
typedef IloArray<IloNumArray> Num2D;

typedef std::unordered_map<int, int> MapSol2D;
typedef std::vector<std::unordered_map<int, int>> MapSol3D;
typedef std::vector<std::tuple<int, int>> Sol2D;
typedef std::vector<std::tuple<int, int, int>> Sol3D;
class MILP : public Model
{
private:
    IloEnv env;
    IloNumVarArray x;
    NumVar3D y;
    NumVar2D z;
    int nb_grain;
    int nb_flour;

public:
    MILP()
    {
        Instance *instance = Instance::getInstance();
        nb_grain = instance->nb_grain;
        nb_flour = instance->nb_flour;
        env = IloEnv();

        x = IloNumVarArray(env, nb_grain);
        y = NumVar3D(env, nb_grain);
        z = NumVar2D(env, nb_grain);
        std::stringstream name;
        // x^g
        for (int g = 0; g < nb_grain; g++)
        {
            name << "x^" << g;
            x[g] = IloNumVar(env, 0, IloInfinity, ILOFLOAT, name.str().c_str());
            name.str("");
        }

        // y^gbf
        for (int g = 0; g < nb_grain; g++)
        {
            y[g] = NumVar2D(env, 3);
            for (int b = 0; b < 3; b++)
            {
                y[g][b] = IloNumVarArray(env, nb_flour);
                for (int f = 0; f < nb_flour; f++)
                {
                    name << "y^" << g << "." << b << "." << f;
                    y[g][b][f] = IloNumVar(env, 0, 1, ILOFLOAT, name.str().c_str());
                    name.str("");
                }
            }
        }

        // z^gb
        for (int g = 0; g < nb_grain; g++)
        {
            z[g] = IloNumVarArray(env, 3);
            for (int b = 0; b < 3; b++)
            {
                name << "z^" << g << "." << b;
                z[g][b] = IloNumVar(env, 0, IloInfinity, ILOFLOAT, name.str().c_str());
                name.str("");
            }
        }

        std::cerr << "Done create var\n";
    }
    IloModel createModel()
    {
        Instance *instance = Instance::getInstance();

        IloModel model(env);
        IloExpr objExpr(env);
        for (int g = 0; g < nb_grain; g++)
            objExpr += x[g] * instance->grains[g].cost;
        model.add(IloMinimize(env, objExpr));

        for (int g = 0; g < nb_grain; g++)
            for (int b = 0; b < 3; b++)
                model.add(x[g] >= z[g][b] / instance->grains[g].specs[b].Rate);

        for (int f = 0; f < nb_flour; f++)
        {
            IloExpr sum_rate(env);
            for (int g = 0; g < nb_grain; g++)
                for (int b = 0; b < 3; b++)
                    sum_rate += y[g][b][f];

            model.add(sum_rate == 1);
        }

        for (int g = 0; g < nb_grain; g++)
        {
            for (int b = 0; b < 3; b++)
            {
                IloExpr total_weight(env);
                for (int f = 0; f < nb_flour; f++)
                    total_weight += y[g][b][f] * instance->flours[f].capacity;
                model.add(z[g][b] == total_weight);
            }
        }

        for (int f = 0; f < nb_flour; f++)
        {
            for (const std::string &name : instance->spec_names)
            {
                IloExpr total_spec(env);
                int idx = instance->spec_to_idx[name];
                for (int g = 0; g < nb_grain; g++)
                    for (int b = 0; b < 3; b++)
                        total_spec += y[g][b][f] * instance->grains[g].specs[b][idx];

                model.add(total_spec >= instance->limits[instance->flours[f].name][name].min);
                if (instance->limits[instance->flours[f].name][name].max < 999999)
                    model.add(total_spec <= instance->limits[instance->flours[f].name][name].max);
            }
        }

        // model.add(x[0] >= 10); // Ràng buộc thử nghiệm (bỏ comment nếu muốn)

        return model;
    }

    Solution run() override
    {
        IloModel model = createModel();
        IloCplex cplex(model);

        // Các thiết lập cấu hình CPLEX (bỏ comment nếu cần)
        // cplex.setParam(IloCplex::Param::TimeLimit, 3600);
        // cplex.setParam(IloCplex::Param::Threads, 4);

        Solution sol;
        Instance *instance = Instance::getInstance();

        try
        {
            std::ofstream solution_file;
            solution_file = std::ofstream("./output.txt");
            if (!solution_file.is_open())
            {
                std::cerr << "Cannot open output file" << std::endl;
                return sol;
            }
            if (cplex.solve())
            {

                solution_file << "====================================================\n";
                solution_file << "GIAI QUYET THANH CONG!\n";
                solution_file << "Trang thai: " << cplex.getStatus() << std::endl;
                solution_file << "Tong chi phi (Objective Value): " << cplex.getObjValue() << std::endl;
                solution_file << "====================================================\n\n";

                // 1. In ra lượng lúa cần mua (biến x)
                solution_file << "--- 1. KHOI LUONG LUA CAN MUA ---\n";
                for (int g = 0; g < nb_grain; g++)
                {
                    double val_x = cplex.getValue(x[g]);
                    // Dùng sai số 1e-5 để bỏ qua các giá trị 0 (hoặc xấp xỉ 0 do sai số dấu phẩy động)
                    if (val_x > 1e-5)
                    {
                        solution_file << "- Lua " << instance->grains[g].name
                                      << " (x^" << g << "): " << val_x << " tan\n";
                    }
                }

                // 2. In ra lượng lúa tách ra ở từng luồng F1, F2, F3 (biến z)
                solution_file << "\n--- 2. KHOI LUONG TUNG PHAN TACH RA (z) ---\n";
                for (int g = 0; g < nb_grain; g++)
                {
                    for (int b = 0; b < 3; b++)
                    {
                        double val_z = cplex.getValue(z[g][b]);
                        if (val_z > 1e-5)
                        {
                            solution_file << "- Lua " << instance->grains[g].name
                                          << " | Phan " << b + 1
                                          << " (z^" << g << "." << b << "): " << val_z << " tan\n";
                        }
                    }
                }

                // 3. In ra tỷ lệ phối trộn vào từng loại bột (biến y)
                solution_file << "\n--- 3. CONG THUC PHOI TRON (y) ---\n";
                for (int f = 0; f < nb_flour; f++)
                {
                    solution_file << ">> Bot thanh pham: " << instance->flours[f].name
                                  << " (San luong: " << instance->flours[f].capacity << ")\n";
                    for (int g = 0; g < nb_grain; g++)
                    {
                        for (int b = 0; b < 3; b++)
                        {
                            double val_y = cplex.getValue(y[g][b][f]);
                            if (val_y > 1e-5)
                            {
                                solution_file << "   + Dung " << (val_y * 100.0) << "% tu "
                                              << instance->grains[g].name << " (Phan " << b + 1 << ")\n";
                            }
                        }
                    }
                }

                solution_file << "====================================================\n";

                // TODO: Nếu struct `Solution` của bạn có các trường lưu dữ liệu,
                // bạn có thể gán giá trị vào `sol` ở đây.
                // Ví dụ: sol.objective = cplex.getObjValue();
            }
            else
            {
                solution_file << "Infeasible or Unbounded solution!\n";
            }
        }
        catch (IloException &e)
        {
            std::cerr << "Error CPLEX: " << e << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unidentified error." << std::endl;
        }

        cplex.end();

        return sol;
    }
};
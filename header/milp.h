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
            {
                double rate = instance->grains[g].specs[b].Rate;
                if (rate > 1e-9)  // ← chỉ thêm ràng buộc khi Rate hợp lệ
                    model.add(x[g] >= z[g][b] / rate);
            }

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

        // ---------------------------------------------------------------------------------
        // 5. RÀNG BUỘC: Ma trận tương thích (Compatibility)
        // Nếu lúa 'g' không được phép dùng cho bột 'f' (compatibility = 0)
        // thì tỷ lệ trộn y[g][b][f] = 0 đối với mọi phần 'b'
        // ---------------------------------------------------------------------------------
        if (!instance->compatibility.empty())
        {
            for (int g = 0; g < nb_grain; g++)
            {
                for (int f = 0; f < nb_flour; f++)
                {
                    if (instance->compatibility[g][f] == 0)
                    {
                        for (int b = 0; b < 3; b++)
                        {
                            model.add(y[g][b][f] == 0);
                        }
                    }
                }
            }
        }

        // model.add(x[0] >= 10); // Ràng buộc thử nghiệm (bỏ comment nếu muốn)

        return model;
    }

    Solution run() override
    {
        IloModel model = createModel();
        IloCplex cplex(model);

        Solution sol;
        Instance *instance = Instance::getInstance();

        try
        {
            std::ofstream f("./output.csv");
            if (!f.is_open())
            {
                std::cerr << "Khong the mo file output.csv" << std::endl;
                return sol;
            }

            if (cplex.solve())
            {
                // ================== 1. PHẦN GHI CHÚ NGẮN Ở ĐẦU ==================
                f << "# ========================================================================\n";
                f << "# KET QUA GIAI BAI TOAN PHOI TRON BOT\n";
                f << "# ========================================================================\n";
                f << "# [GHI CHU Y NGHIA CAC LOAI BIEN (Cot 'type')]\n";
                f << "# - x: Khoi luong lua tho (tan) can thu mua.\n";
                f << "# - z: Khoi luong tung phan (Phan 1, 2, 3) thu duoc sau khi tach lua tho.\n";
                f << "# - y: Ty le (%) cua tung phan lua duoc dung de phoi tron tao ra bot.\n";
                f << "# ------------------------------------------------------------------------\n";
                f << "# => TRANG THAI: " << cplex.getStatus() << "\n";
                f << "# => TONG CHI PHI: " << cplex.getObjValue() << "\n";
                f << "# ========================================================================\n\n";

                // ================== 2. PHẦN BẢNG DỮ LIỆU CHÍNH (CSV) ==================
                f << "type,flour_name,flour_capacity,grain_name,grain_idx,part,value\n";

                // Ghi dữ liệu biến x^g
                for (int g = 0; g < nb_grain; g++)
                {
                    double val = cplex.getValue(x[g]);
                    if (val > 1e-5)
                        f << "x,,," << instance->grains[g].name << "," << g << ",," << val << "\n";
                }
                // Ghi dữ liệu biến z^gb
                for (int g = 0; g < nb_grain; g++)
                {
                    for (int b = 0; b < 3; b++)
                    {
                        double val = cplex.getValue(z[g][b]);
                        if (val > 1e-5)
                            f << "z,,," << instance->grains[g].name << "," << g << "," << (b + 1) << "," << val << "\n";
                    }
                }
                // Ghi dữ liệu biến y^gbf
                for (int fi = 0; fi < nb_flour; fi++)
                {
                    for (int g = 0; g < nb_grain; g++)
                    {
                        for (int b = 0; b < 3; b++)
                        {
                            double val = cplex.getValue(y[g][b][fi]);
                            if (val > 1e-5)
                                f << "y," << instance->flours[fi].name << "," << instance->flours[fi].capacity << ","
                                  << instance->grains[g].name << "," << g << "," << (b + 1) << "," << (val * 100.0) << "\n";
                        }
                    }
                }

                // ================== 3. PHẦN BÁO CÁO BẰNG LỜI (DƯỚI ĐÁY FILE) ==================
                f << "\n# ========================================================================\n";
                f << "# BAO CAO CHI TIET KE HOACH SAN XUAT (Dich tu cac con so)\n";
                f << "# ========================================================================\n";

                f << "# [1. KE HOACH THU MUA LUA THO]\n";
                for (int g = 0; g < nb_grain; g++)
                {
                    double val = cplex.getValue(x[g]);
                    if (val > 1e-5)
                        f << "# - Mua " << val << " tan lua [" << instance->grains[g].name << "].\n";
                }

                f << "#\n# [2. KE HOACH XAY XAT / TACH PHAN]\n";
                for (int g = 0; g < nb_grain; g++)
                {
                    for (int b = 0; b < 3; b++)
                    {
                        double val = cplex.getValue(z[g][b]);
                        if (val > 1e-5)
                            f << "# - Lua [" << instance->grains[g].name << "] - Phan " << (b + 1) << ": Thu duoc " << val << " tan.\n";
                    }
                }

                f << "#\n# [3. CONG THUC PHOI TRON BOT THANH PHAM]\n";
                for (int fi = 0; fi < nb_flour; fi++)
                {
                    f << "# >> De san xuat " << instance->flours[fi].capacity << " tan bot [" << instance->flours[fi].name << "], can:\n";
                    bool has_recipe = false;
                    for (int g = 0; g < nb_grain; g++)
                    {
                        for (int b = 0; b < 3; b++)
                        {
                            double val = cplex.getValue(y[g][b][fi]);
                            if (val > 1e-5)
                            {
                                f << "#    - Dung " << (val * 100.0) << "% tu lua [" << instance->grains[g].name << "] (Phan " << (b + 1) << ").\n";
                                has_recipe = true;
                            }
                        }
                    }
                    if (!has_recipe)
                        f << "#    (Khong can san xuat loai bot nay)\n";
                }
                f << "# ========================================================================\n";

                std::cerr << "Da xuat file output.csv (co kem bao cao) thanh cong! Objective: " << cplex.getObjValue() << "\n";
            }
            else
            {
                f << "# BAI TOAN KHONG CO LOI GIAI (Infeasible hoac Unbounded)!\n";
                f << "type,flour_name,flour_capacity,grain_name,grain_idx,part,value\n";
                f << "infeasible,,,,,,\n";
                std::cerr << "Infeasible or Unbounded solution!\n";
            }
        }
        catch (IloException &e)
        {
            std::cerr << "Loi CPLEX: " << e << std::endl;
        }
        catch (...)
        {
            std::cerr << "Loi khong xac dinh." << std::endl;
        }

        cplex.end();
        return sol;
    }
};
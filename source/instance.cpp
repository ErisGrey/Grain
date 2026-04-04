#include "../header/instance.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

// Hàm phụ trợ làm sạch chuỗi (xóa \r và khoảng trắng dư thừa)
void clean_string(std::string &s)
{
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
    // Xóa khoảng trắng ở đầu và cuối chuỗi (nếu có)
    if (!s.empty())
    {
        size_t start = s.find_first_not_of(" \t");
        size_t end = s.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos)
            s = s.substr(start, end - start + 1);
        else
            s = "";
    }
}

// Chuyển đổi an toàn (không bao giờ gây crash)
double to_val(std::string s)
{
    clean_string(s);
    if (s == "inf" || s.empty())
        return 999999.0;
    try
    {
        return std::stod(s);
    }
    catch (...)
    {
        return 999999.0; // Tránh crash nếu gặp chữ lạ trong bảng Min/Max
    }
}

Instance::Instance(const std::string filename)
{
    read_input(filename);
    print_summary();
}

void Instance::init_spec_map()
{
    for (int i = 0; i < (int)spec_names.size(); ++i)
    {
        spec_to_idx[spec_names[i]] = i + 1;
    }
}

void Instance::read_input(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        std::cerr << "File input is missing or wrong !!\n";
    std::string line, cell;

    // 1. Đọc Grain
    std::getline(file, line);
    std::stringstream ss_nb(line);
    std::getline(ss_nb, cell, ',');
    std::getline(ss_nb, cell, ',');
    clean_string(cell);
    nb_grain = cell.empty() ? 0 : std::stoi(cell);

    for (int i = 0; i < nb_grain; ++i)
    {
        Grain g;
        std::getline(file, line);
        std::stringstream ss_g(line);
        std::getline(ss_g, g.name, ',');
        clean_string(g.name);

        std::getline(ss_g, cell, ',');
        clean_string(cell);
        try
        {
            g.cost = cell.empty() ? 0.0 : std::stod(cell);
        }
        catch (...)
        {
            g.cost = 0;
        }

        for (int j = 0; j < 3; ++j)
        {
            std::getline(file, line);
            std::stringstream ss_s(line);
            std::vector<double> v;
            while (std::getline(ss_s, cell, ','))
            {
                clean_string(cell);
                if (!cell.empty())
                {
                    try
                    {
                        v.push_back(std::stod(cell));
                    }
                    catch (...)
                    {
                    }
                }
            }
            // Bọc an toàn nếu dữ liệu lúa thiếu cột
            while (v.size() < 8)
                v.push_back(0.0);
            g.specs.push_back({v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]});
        }
        grains.push_back(g);
    }

    // 2. Đọc Nb of flour
    while (std::getline(file, line))
    {
        if (line.find("Nb of flour") != std::string::npos)
        {
            std::stringstream ss_f(line);
            std::getline(ss_f, cell, ',');
            std::getline(ss_f, cell, ',');
            clean_string(cell);
            nb_flour = cell.empty() ? 0 : std::stoi(cell);
            break;
        }
    }

    for (int i = 0; i < nb_flour; ++i)
    {
        std::getline(file, line);
        std::stringstream ss_f(line);
        Flour f;
        std::getline(ss_f, f.name, ',');
        clean_string(f.name);
        flours.push_back(f);
    }

    // 3. Đọc Capacity
    while (std::getline(file, line))
    {
        if (line.find(flours[0].name) != std::string::npos && line.find(",,") == 0)
        {
            std::getline(file, line);
            std::stringstream ss_cap(line);
            std::getline(ss_cap, cell, ',');
            std::getline(ss_cap, cell, ',');
            for (int i = 0; i < nb_flour; ++i)
            {
                std::getline(ss_cap, cell, ',');
                clean_string(cell);
                try
                {
                    flours[i].capacity = cell.empty() ? 0.0 : std::stod(cell);
                }
                catch (...)
                {
                    flours[i].capacity = 0.0;
                }
            }
            break;
        }
    }

    // 4. Đọc bảng Min/Max
    while (std::getline(file, line))
    {
        if (line.find("Compatibility") != std::string::npos)
            break;

        clean_string(line);
        if (line.empty() || line.find_first_not_of(',') == std::string::npos)
            continue;

        std::stringstream ss_l(line);
        std::string paramName, type;
        std::getline(ss_l, paramName, ',');
        clean_string(paramName);
        std::getline(ss_l, type, ',');
        clean_string(type);

        if (type == "Min" || type == "Max")
        {
            for (int i = 0; i < nb_flour; ++i)
            {
                std::getline(ss_l, cell, ',');
                if (type == "Min")
                    limits[flours[i].name][paramName].min = to_val(cell);
                else
                    limits[flours[i].name][paramName].max = to_val(cell);
            }
        }
    }

    // 5. Đọc Ma trận tương thích
    if (line.find("Compatibility") != std::string::npos)
    {
        compatibility.assign(nb_grain, std::vector<int>(nb_flour, 1));

        if (std::getline(file, line))
        {
            std::stringstream ss_header(line);
            std::string flour_name;
            std::getline(ss_header, flour_name, ',');

            std::vector<int> col_to_flour_idx;
            while (std::getline(ss_header, flour_name, ','))
            {
                clean_string(flour_name);
                if (flour_name.empty())
                    continue;

                int f_idx = -1;
                for (int i = 0; i < nb_flour; ++i)
                {
                    if (flours[i].name == flour_name)
                    {
                        f_idx = i;
                        break;
                    }
                }
                col_to_flour_idx.push_back(f_idx);
            }

            while (std::getline(file, line))
            {
                clean_string(line);
                if (line.empty() || line.find_first_not_of(',') == std::string::npos)
                    continue;

                std::stringstream ss_comp(line);
                std::string grain_name;
                std::getline(ss_comp, grain_name, ',');
                clean_string(grain_name);

                int g_idx = -1;
                for (int i = 0; i < nb_grain; ++i)
                {
                    if (grains[i].name == grain_name)
                    {
                        g_idx = i;
                        break;
                    }
                }

                if (g_idx == -1)
                    continue;

                size_t col_index = 0;
                while (std::getline(ss_comp, cell, ',') && col_index < col_to_flour_idx.size())
                {
                    clean_string(cell);
                    if (!cell.empty())
                    {
                        int f_idx = col_to_flour_idx[col_index];
                        if (f_idx != -1)
                        {
                            try
                            {
                                compatibility[g_idx][f_idx] = std::stoi(cell);
                            }
                            catch (...)
                            {
                            }
                        }
                    }
                    col_index++;
                }
            }
        }
    }
}

void Instance::print_summary()
{
    if (spec_to_idx.empty())
        init_spec_map();

    std::cout << "=== FLOURS & LIMITS (WITH INDEX) ===\n";
    for (auto &f : flours)
    {
        std::cout << "\nFlour: " << f.name << "\n";

        for (const std::string &name : spec_names)
        {
            int idx = spec_to_idx[name];
            double mn = limits[f.name][name].min;
            double mx = limits[f.name][name].max;

            std::cout << "  " << idx << ". " << std::left << std::setw(8) << name
                      << ": [" << mn << " - " << (mx >= 999999 ? "inf" : std::to_string(mx)) << "]\n";
        }
    }

    if (!compatibility.empty())
    {
        std::cout << "\n=== COMPATIBILITY MATRIX (GRAIN x FLOUR) ===\n";
        std::cout << std::setw(10) << " ";
        for (int j = 0; j < nb_flour; ++j)
            std::cout << std::setw(10) << flours[j].name;
        std::cout << "\n";

        for (int i = 0; i < nb_grain; ++i)
        {
            std::cout << std::setw(10) << grains[i].name;
            for (int j = 0; j < nb_flour; ++j)
            {
                std::cout << std::setw(10) << compatibility[i][j];
            }
            std::cout << "\n";
        }
    }
}

Instance *Instance::singleton_ = nullptr;
Instance *Instance::getInstance(const std::string filename)
{
    if (singleton_ == nullptr)
    {
        singleton_ = new Instance(filename);
    }
    return singleton_;
}
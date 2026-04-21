#include "../header/instance.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <set>

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
    // =========================================================================
    // BƯỚC 0: Quét trước file CSV để thu thập tên các thành phần từ bảng Min/Max
    // Mục đích: Biết trước danh sách spec_names TRƯỚC khi đọc dữ liệu grain
    // =========================================================================
    {
        std::ifstream pre_scan(filename);
        if (!pre_scan.is_open())
        {
            std::cerr << "File input is missing or wrong !!\n";
            return;
        }
        std::string line, cell;
        // Tập hợp để theo dõi thứ tự xuất hiện (dùng vector + set)
        std::set<std::string> seen_specs;
        while (std::getline(pre_scan, line))
        {
            clean_string(line);
            if (line.find("Compatibility") != std::string::npos)
                break; // Kết thúc vùng Min/Max

            // Phát hiện vùng Min/Max: dòng có cột thứ 2 là "Min" hoặc "Max"
            std::stringstream ss(line);
            std::string col1, col2;
            std::getline(ss, col1, ',');
            clean_string(col1);
            std::getline(ss, col2, ',');
            clean_string(col2);

            if (col2 == "Min" || col2 == "Max")
            {
                if (!col1.empty() && seen_specs.find(col1) == seen_specs.end())
                {
                    spec_names.push_back(col1);
                    seen_specs.insert(col1);
                }
            }
        }
        pre_scan.close();
    }

    nb_spec = (int)spec_names.size();
    init_spec_map();

    std::cerr << "Da phat hien " << nb_spec << " thanh phan tu CSV: ";
    for (const auto &n : spec_names)
        std::cerr << n << " ";
    std::cerr << "\n";

    // =========================================================================
    // BƯỚC 1: Đọc chính thức toàn bộ dữ liệu
    // =========================================================================
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

            // Đọc tổng quát: cột đầu = Rate, các cột sau = spec values
            // Đảm bảo có đủ giá trị (pad thêm 0 nếu thiếu)
            int expected_cols = 1 + nb_spec; // Rate + nb_spec giá trị
            while ((int)v.size() < expected_cols)
                v.push_back(0.0);

            FlourSpecs fs;
            fs.Rate = v[0];
            fs.values.assign(v.begin() + 1, v.begin() + 1 + nb_spec);
            g.specs.push_back(fs);
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

    // 4. Đọc bảng Min/Max (tổng quát — đọc tất cả spec từ CSV)
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
    std::cout << "So thanh phan: " << nb_spec << "\n";
    for (auto &f : flours)
    {
        std::cout << "\nFlour: " << f.name << "\n";

        for (const std::string &name : spec_names)
        {
            int idx = spec_to_idx[name];
            double mn = limits[f.name][name].min;
            double mx = limits[f.name][name].max;

            std::cout << "  " << idx << ". " << std::left << std::setw(12) << name
                      << ": [" << mn << " - " << (mx >= 999999 ? "inf" : std::to_string(mx)) << "]\n";
        }
    }

    // In thông tin grain specs
    std::cout << "\n=== GRAIN SPECS ===\n";
    for (int g = 0; g < nb_grain; ++g)
    {
        std::cout << "Grain: " << grains[g].name << " (cost=" << grains[g].cost << ")\n";
        for (int b = 0; b < 3; ++b)
        {
            std::cout << "  Part " << (b + 1) << ": Rate=" << grains[g].specs[b].Rate;
            for (int s = 0; s < nb_spec; ++s)
            {
                std::cout << ", " << spec_names[s] << "=" << grains[g].specs[b].values[s];
            }
            std::cout << "\n";
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
#include "../header/instance.h"
#include <fstream>
#include <sstream>
#include <iomanip>
double to_val(const std::string &s)
{
    if (s == "inf" || s.empty())
        return 999999.0;
    return std::stod(s);
}

Instance::Instance(const std::string filename)
{
    read_input(filename);
    print_summary();
}

void Instance::init_spec_map()
{
    // Duyệt qua vector tên và gán giá trị từ 1 đến 7
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
    nb_grain = std::stoi(cell);

    for (int i = 0; i < nb_grain; ++i)
    {
        Grain g;
        std::getline(file, line);
        std::stringstream ss_g(line);
        std::getline(ss_g, g.name, ',');
        std::getline(ss_g, cell, ',');
        g.cost = std::stod(cell);

        for (int j = 0; j < 3; ++j)
        {
            std::getline(file, line);
            std::stringstream ss_s(line);
            std::vector<double> v;
            while (std::getline(ss_s, cell, ','))
                if (!cell.empty())
                    v.push_back(std::stod(cell));
            g.specs.push_back({v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]});
        }
        grains.push_back(g);
    }

    // 2. Đọc Nb of flour và danh sách tên bột
    while (std::getline(file, line))
    {
        if (line.find("Nb of flour") != std::string::npos)
        {
            std::stringstream ss_f(line);
            std::getline(ss_f, cell, ',');
            std::getline(ss_f, cell, ',');
            nb_flour = std::stoi(cell);
            break;
        }
    }

    for (int i = 0; i < nb_flour; ++i)
    {
        std::getline(file, line);
        std::stringstream ss_f(line);
        Flour f;
        std::getline(ss_f, f.name, ',');
        flours.push_back(f);
    }

    // 3. Đọc dòng chứa giá trị capacity (ví dụ: 50, 25, 10)
    while (std::getline(file, line))
    {
        if (line.find(flours[0].name) != std::string::npos && line.find(",,") == 0)
        {
            // Đây là dòng tiêu đề BP3_1, BP3_2... bỏ qua
            std::getline(file, line); // Đọc dòng tiếp theo chứa số 50, 25, 10
            std::stringstream ss_cap(line);
            std::getline(ss_cap, cell, ',');
            std::getline(ss_cap, cell, ','); // Bỏ 2 cột đầu
            for (int i = 0; i < nb_flour; ++i)
            {
                std::getline(ss_cap, cell, ',');
                flours[i].capacity = std::stod(cell);
            }
            break;
        }
    }

    // 4. Đọc bảng Min/Max
    while (std::getline(file, line))
    {
        if (line.empty() || line.find_first_not_of(',') == std::string::npos)
            continue;
        std::stringstream ss_l(line);
        std::string paramName, type;
        std::getline(ss_l, paramName, ',');
        std::getline(ss_l, type, ',');

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

void Instance::print_summary()
{
    // Đảm bảo map đã được khởi tạo
    if (spec_to_idx.empty())
        init_spec_map();

    std::cout << "=== FLOURS & LIMITS (WITH INDEX) ===\n";
    for (auto &f : flours)
    {
        std::cout << "\nFlour: " << f.name << "\n";

        for (const std::string &name : spec_names)
        {
            int idx = spec_to_idx[name]; // Lấy số 1->7 từ map
            double mn = limits[f.name][name].min;
            double mx = limits[f.name][name].max;

            // In ra kèm theo index
            std::cout << "  " << idx << ". " << std::left << std::setw(8) << name
                      << ": [" << mn << " - " << (mx >= 999999 ? "inf" : std::to_string(mx)) << "]\n";
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
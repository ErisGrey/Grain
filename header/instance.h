#ifndef INSTANCE_H
#define INSTANCE_H

#include <iostream>
#include <vector>
#include <string>
#include <map>

// Cấu trúc chứa các chỉ số chất lượng cho mỗi dòng (F1, F2, hoặc F3)
struct FlourSpecs
{
    double Rate;   // F1, F2, F3
    double Dam;    // Index 1
    double WG;     // Index 2
    double Tro;    // Index 3
    double Do_roi; // Index 4
    double Muoi;   // Index 5
    double Duong;  // Index 6
    double Nuoc;   // Index 7

    // Nạp chồng (overload) toán tử []
    // Chữ 'const' ở cuối đảm bảo hàm này không vô tình làm thay đổi dữ liệu
    double operator[](int idx) const
    {
        switch (idx)
        {
        case 1:
            return Dam;
        case 2:
            return WG;
        case 3:
            return Tro;
        case 4:
            return Do_roi;
        case 5:
            return Muoi;
        case 6:
            return Duong;
        case 7:
            return Nuoc;
        default:
            std::cerr << "Canh bao: Truy cap index " << idx << " khong hop le trong FlourSpecs!\n";
            return 0.0;
        }
    }
};

struct Grain
{
    std::string name;
    double cost;
    std::vector<FlourSpecs> specs;
};

struct Flour
{
    std::string name;
    double capacity; // Giá trị 50, 25, 10 ở dòng cuối
};

// Cấu trúc lưu giới hạn Min/Max cho một loại bột
struct Limit
{
    double min;
    double max;
};
class Instance
{
protected:
    Instance(const std::string filename);
    static Instance *singleton_;

public:
    int nb_grain;
    int nb_flour;
    std::vector<Grain> grains;
    std::vector<Flour> flours;
    std::vector<std::vector<int>> compatibility;
    // Lưu giới hạn: limits[Tên_Bột][Tên_Chỉ_Số] = {min, max}
    // Ví dụ: limits["BP3_1"]["Dam"].min = 13.0
    std::map<std::string, std::map<std::string, Limit>> limits;
    std::map<std::string, int> spec_to_idx;
    // Danh sách tên để duyệt (theo đúng thứ tự)
    std::vector<std::string> spec_names = {
        "Dam", "WG", "Tro", "Do_roi", "Muoi", "Duong", "Nuoc"};

    void init_spec_map(); // Hàm khởi tạo map
    void read_input(const std::string &filename);
    void print_summary();

    Instance(Instance &other) = delete;
    void operator=(const Instance &) = delete;
    static Instance *getInstance(const std::string input = "");
};

#endif
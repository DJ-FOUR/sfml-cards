#pragma once

#include <string>

struct SaveData
{
    int         slot   = 0;
    int         level  = 1;
    int         wins   = 0;
    int         losses = 0;
    bool        exists = false;
    std::string name;       // 存档名称, 默认 "存档 N"
};

class SaveManager
{
public:
    static constexpr int MAX_SLOTS = 3;

    SaveData loadSlot(int n) const;
    void     saveSlot(const SaveData& data);
    void     deleteSlot(int n);

private:
    std::string path(int n) const;
};

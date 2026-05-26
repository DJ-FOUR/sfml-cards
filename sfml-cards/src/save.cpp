#include "save.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <direct.h>
#define mkdir(p) _mkdir(p)
#else
#include <sys/stat.h>
#endif

std::string SaveManager::path(int n) const
{
    return "saves/slot" + std::to_string(n) + ".sav";
}

SaveData SaveManager::loadSlot(int n) const
{
    SaveData d;
    d.slot = n;
    d.name = "存档 " + std::to_string(n);

    auto fp = std::fopen(path(n).c_str(), "r");
    if (!fp) return d;

    d.exists = true;
    char line[256];
    while (std::fgets(line, sizeof(line), fp)) {
        char key[32];
        int  val;
        if (std::sscanf(line, "%31[^=]=%d", key, &val) == 2) {
            std::string ks(key);
            if (ks == "level")  d.level  = val;
            if (ks == "wins")   d.wins   = val;
            if (ks == "losses") d.losses = val;
        }
        // name 单独处理 (可能含中文)
        char nameBuf[128];
        if (std::sscanf(line, "name=%127[^\n]", nameBuf) == 1)
            d.name = nameBuf;
    }
    std::fclose(fp);
    return d;
}

void SaveManager::saveSlot(const SaveData& data)
{
    mkdir("saves");
    auto fp = std::fopen(path(data.slot).c_str(), "w");
    if (!fp) return;
    std::fprintf(fp, "level=%d\n", data.level);
    std::fprintf(fp, "wins=%d\n",  data.wins);
    std::fprintf(fp, "losses=%d\n", data.losses);
    std::fprintf(fp, "name=%s\n",   data.name.c_str());
    std::fclose(fp);
}

void SaveManager::deleteSlot(int n)
{
    std::remove(path(n).c_str());
}

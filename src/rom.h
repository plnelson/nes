#pragma once

#include "mem.h"

#include <vector>

const u32 PRG_ROM_BANK_SIZE = 0x4000;
const u32 PRG_RAM_UNIT_SIZE = 0x2000;
const u32 CHR_ROM_BANK_SIZE = 0x2000;

enum class NameTableMirroring : u8
{
    SingleScreenUpper = 0,
    SingleScreenLower = 1,
    Vertical = 2,
    Horizontal = 3
};

#pragma pack()
struct INesHeader 
{
    // 'N' 'E' 'S' '\x1a'
    u8 magic[4];
    u8 PrgRomSize;
    u8 ChrRomSize;
    u8 flags6;
    u8 flags7;
    u8 PrgRamSize;
    u8 flags9;
    u8 flags10;
    u8 zero[5];

    bool ValidateHeader()
    {
        if (strncmp((char*)magic, "NES\x1a", 4) != 0)
        {
            return false;
        }

        if (strncmp(&((char*)this)[7], "DiskDude!", 9) == 0)
        {
            // Header Garbage, clear rest of header
            flags7 = 0;
            PrgRamSize = 0;
            flags9 = 0;
            flags10 = 0;
            memset(zero, 0, sizeof(zero));
        }

        return true;
    }

    bool HasTrainer()
    {
        return (flags6 & (1 << 2)) != 0;
    }

    bool HasSaveRam()
    {
        return (flags6 & (1 << 1)) != 0;
    }

    u32 MapperNumber()
    {
        u8 lo = (flags6 >> 4) & 0x0f;
        u8 hi = (flags7 >> 4) & 0x0f;

        return (hi << 4) | lo;
    }

    NameTableMirroring Mirroring()
    {
        return (flags6 & 0x1) == 0 ? NameTableMirroring::Horizontal : NameTableMirroring::Vertical;
    }
};

class Rom : public ISaveState
{
public:
    Rom();
    ~Rom();

    bool Load(std::string romPath);
    const fs::path& Path();

public:
    void SaveState(std::ofstream& ofs);
    void LoadState(std::ifstream& ifs);

private:
    void SaveGame();
    void LoadGame();
    std::unique_ptr<fs::path> GetSaveGamePath();

public:
    INesHeader Header;
    std::vector<u8> PrgRom;
    std::vector<u8> PrgRam;
    std::vector<u8> ChrRom;

private:
    fs::path _path;
};
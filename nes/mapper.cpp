#include "stdafx.h"
#include "mapper.h"
#include "rom.h"

IMapper::IMapper(std::shared_ptr<Rom> rom)
    : _rom(rom)
{
    Mirroring = _rom->Header.Mirroring();
}

IMapper::~IMapper()
{
}

std::shared_ptr<IMapper> IMapper::CreateMapper(std::shared_ptr<Rom> rom)
{
    switch (rom->Header.MapperNumber())
    {
    case 0:
        return std::make_shared<NRom>(rom);
    case 1:
        return std::make_shared<SxRom>(rom);
    case 2:
        return std::make_shared<UNRom>(rom);
    case 3:
        return std::make_shared<CNRom>(rom);
    default:
        printf("Unsupported mapper: %d\n", rom->Header.MapperNumber());
        return nullptr;
    }
}

void IMapper::Save(std::ofstream& ofs)
{
    Util::WriteBytes((u8)Mirroring, ofs);
    _rom->Save(ofs);
}

void IMapper::Load(std::ifstream& ifs)
{
    Util::ReadBytes((u8&)Mirroring, ifs);
    _rom->Load(ifs);
}

/// NRom

NRom::NRom(std::shared_ptr<Rom> rom)
    : IMapper(rom)
{
    if (_rom->Header.ChrRomSize > 0)
    {
        _chrBuf = &_rom->ChrRom[0];
    }
    else
    {
        _chrBuf = _chrRam;
    }
}

NRom::~NRom()
{
}

u8 NRom::prg_loadb(u16 addr)
{
    if (addr < 0x8000)
    {
        return _rom->PrgRam[addr & 0x1fff];
    }
    else
    {
        if (_rom->Header.PrgRomSize == 1)
        {
            return _rom->PrgRom[addr & 0x3fff];
        }
        else
        {
            return _rom->PrgRom[addr & 0x7fff];
        }
    }
}

void NRom::prg_storeb(u16 addr, u8 val)
{
    if (addr < 0x8000)
    {
        _rom->PrgRam[addr & 0x1fff] = val;
    }
}

u8 NRom::chr_loadb(u16 addr)
{
    return _chrBuf[addr]; // this will return ChrRom if present or ChrRam if not
}

void NRom::chr_storeb(u16 addr, u8 val)
{
    _chrRam[addr] = val; // This will only ever store to ChrRam
}

void NRom::Save(std::ofstream& ofs)
{
    IMapper::Save(ofs);
    ofs.write((char*)_chrRam, sizeof(_chrRam));
}

void NRom::Load(std::ifstream& ifs)
{
    IMapper::Load(ifs);
    ifs.read((char*)_chrRam, sizeof(_chrRam));
}

/// SxRom (Mapper #1)

SxRom::SxRom(std::shared_ptr<Rom> rom)
    : IMapper(rom)
    , _prgSize(PrgSize::Size16k)
    , _chrMode(ChrMode::Mode8k)
    , _slotSelect(true)
    , _chrBank0(0)
    , _chrBank1(0)
    , _prgBank(0)
    , _accumulator(0)
    , _writeCount(0)
{
    if (rom->Header.ChrRomSize > 0)
    {
        _chrBuf = &rom->ChrRom[0];
    }
    else
    {
        _chrRam.resize(0x2000);
        _chrBuf = &_chrRam[0];
    }
}

SxRom::~SxRom()
{
}

u8 SxRom::prg_loadb(u16 addr)
{
    if (addr < 0x8000)
    {
        return _rom->PrgRam[addr & 0x1fff];
    }
    else
    {
        if (_prgSize == PrgSize::Size32k)
        {
            return _rom->PrgRom[((_prgBank >> 1) * 0x4000 * 2) + (addr & 0x7fff)];
        }
        else if (_prgSize == PrgSize::Size16k)
        {
            if (addr < 0xc000)
            {
                if (!_slotSelect)
                {
                    return _rom->PrgRom[addr & 0x3fff];
                }
                else
                {
                    return _rom->PrgRom[(_prgBank * 0x4000) + (addr & 0x3fff)];
                }
            }
            else
            {
                if (!_slotSelect)
                {
                    return _rom->PrgRom[(_prgBank * 0x4000) + (addr & 0x3fff)];
                }
                else
                {
                    return _rom->PrgRom[((_rom->Header.PrgRomSize - 1) * 0x4000) + (addr & 0x3fff)];
                }
            }
        }
        else
        {
            // can't happen
            __debugbreak();
            return 0;
        }
    }
}

void SxRom::prg_storeb(u16 addr, u8 val)
{
    if (addr < 0x8000)
    {
        _rom->PrgRam[addr & 0x1fff] = val;
        return;
    }

    if ((val & (1 << 7)) != 0)
    {
        _writeCount = 0;
        _accumulator = 0;
        _prgSize = PrgSize::Size16k;
        _slotSelect = true;
        return;
    }

    _accumulator = _accumulator | ((val & 1) << _writeCount++);
    if (_writeCount == 5)
    {
        _writeCount = 0;

        if (addr <= 0x9fff)
        {
            switch (_accumulator & 0x3)
            {
            case 0: Mirroring = NameTableMirroring::SingleScreenLower; break;
            case 1: Mirroring = NameTableMirroring::SingleScreenUpper; break;
            case 2: Mirroring = NameTableMirroring::Vertical; break;
            case 3: Mirroring = NameTableMirroring::Horizontal; break;
            }
            _slotSelect = (_accumulator & (1 << 2)) != 0;
            _prgSize = (_accumulator & (1 << 3)) == 0 ? PrgSize::Size32k : PrgSize::Size16k;
            _chrMode = (_accumulator & (1 << 4)) == 0 ? ChrMode::Mode8k : ChrMode::Mode4k;
        }
        else if (addr <= 0xbfff)
        {
            _chrBank0 = _accumulator & 0x1f;
        }
        else if (addr <= 0xdfff)
        {
            _chrBank1 = _accumulator & 0x1f;
        }
        else
        {
            _prgBank = (_accumulator & 0xf);
        }

        _accumulator = 0;
    }
}

u8 SxRom::chr_loadb(u16 addr)
{
    return _chrBuf[ChrBufAddress(addr)];
}

void SxRom::chr_storeb(u16 addr, u8 val)
{
    _chrBuf[ChrBufAddress(addr)] = val;
}

u32 SxRom::ChrBufAddress(u16 addr)
{
    if (_chrMode == ChrMode::Mode4k)
    {
        if (addr < 0x1000)
        {
            return (_chrBank0 * 0x1000) + addr;
        }
        else
        {
            return (_chrBank1 * 0x1000) + (addr & 0xfff);
        }
    }
    else
    {
        return ((_chrBank0 >> 1) * 0x2000) + addr;
    }
}

void SxRom::Save(std::ofstream& ofs)
{
    IMapper::Save(ofs);
    Util::WriteBytes((u8)_prgSize, ofs);
    Util::WriteBytes((u8)_chrMode, ofs);
    Util::WriteBytes(_slotSelect, ofs);
    Util::WriteBytes(_chrBank0, ofs);
    Util::WriteBytes(_chrBank1, ofs);
    Util::WriteBytes(_prgBank, ofs);
    Util::WriteBytes(_accumulator, ofs);
    Util::WriteBytes(_writeCount, ofs);
    ofs.write((char*)&_chrRam[0], _chrRam.size());
}

void SxRom::Load(std::ifstream& ifs)
{
    IMapper::Load(ifs);
    Util::ReadBytes((u8&)_prgSize, ifs);
    Util::ReadBytes((u8&)_chrMode, ifs);
    Util::ReadBytes(_slotSelect, ifs);
    Util::ReadBytes(_chrBank0, ifs);
    Util::ReadBytes(_chrBank1, ifs);
    Util::ReadBytes(_prgBank, ifs);
    Util::ReadBytes(_accumulator, ifs);
    Util::ReadBytes(_writeCount, ifs);
    ifs.read((char*)&_chrRam[0], _chrRam.size());
}

/// UNRom (Mapper #2)
UNRom::UNRom(std::shared_ptr<Rom> rom)
    : NRom(rom)
    , _prgBank(0)
{
    _lastBankOffset = (_rom->Header.PrgRomSize - 1) * PRG_ROM_BANK_SIZE;
}

UNRom::~UNRom()
{
}

void UNRom::prg_storeb(u16 addr, u8 val)
{
    // UNROM has 8 switchable banks so mask bank switch with lower 3 bits
    _prgBank = val & 0x07;
}

u8 UNRom::prg_loadb(u16 addr)
{
    if (addr > 0xC000)
    {
        return _rom->PrgRom[_lastBankOffset + (addr & 0x3FFF)];
    }
    else
    {
        return _rom->PrgRom[(_prgBank * PRG_ROM_BANK_SIZE) + (addr & 0x3FFF)];
    }
}

/// CNRom (Mapper #3)

CNRom::CNRom(std::shared_ptr<Rom> rom)
    : NRom(rom)
    , _chrBank(0)
{
}

CNRom::~CNRom()
{
}

void CNRom::prg_storeb(u16 addr, u8 val)
{
    // CNROM only supports 32KB of CHR ROM, but some games write values
    // such as $FF to switch to bank 3 so we need to mask with 0x03
    _chrBank = val & 0x03;
}

u8 CNRom::chr_loadb(u16 addr)
{
    return _rom->ChrRom[(_chrBank * CHR_ROM_BANK_SIZE) + addr];
}
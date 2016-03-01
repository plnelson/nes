#pragma once

#include "stdafx.h"
#include "mem.h"
#include "ppu.h"
#include "apu.h"
#include "input.h"

/*
    Ram
*/
Ram::Ram() 
{
    ZeroMemory(_ram, sizeof(_ram));
}

Ram::~Ram()
{

}

// IMem
u8 Ram::loadb(u16 addr)
{
    return _ram[addr & 0x7ff];
}

void Ram::storeb(u16 addr, u8 val)
{
    _ram[addr & 0x7ff] = val;
}

/*
    MemoryMap
*/
MemoryMap::MemoryMap(Ppu& ppu, Apu& apu, Input& input, std::shared_ptr<IMapper> mapper)
    : _ppu(ppu)
    , _apu(apu)
    , _input(input)
    , _mapper(mapper)
{

}

MemoryMap::~MemoryMap()
{

}

// IMem
u8 MemoryMap::loadb(u16 addr)
{
    if (addr < 0x2000)
    {
        return _ram.loadb(addr);
    }
    else if (addr < 0x4000)
    {
        return _ppu.loadb(addr);
    }
    else if (addr < 0x4016)
    {
        return _apu.loadb(addr);
    }
    else if (addr < 0x4020)
    {
        return _input.loadb(addr);
    }
    else if (addr < 0x6000)
    {
        // TODO: Expansion ROM
        // TODO: SRAM
        return 0;
    }
    else
    {
        return _mapper->prg_loadb(addr);
    }
}

void MemoryMap::storeb(u16 addr, u8 val)
{
    if (addr < 0x2000)
    {
        _ram.storeb(addr, val);
    }
    else if (addr < 0x4000)
    {
        _ppu.storeb(addr, val);
    }
    else if (addr < 0x4016)
    {
        _apu.storeb(addr, val);
    }
    else if (addr < 0x4020)
    {
        _input.storeb(addr, val);
    }
    else if (addr < 0x6000)
    {
        // TODO: Expansion ROM
        // TODO: SRAM
    }
    else
    {
        _mapper->prg_storeb(addr, val);
    }
}
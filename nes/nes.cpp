// nes.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "cpu.h"
#include "new_ppu.h"
#include "mem.h"
//#include "ppu.h"
#include "new_ppu.h"
#include "input.h"
#include "rom.h"
#include "apu.h"
#include "gfx.h"
#include "mapper.h"

#include <chrono>

void calc_fps(std::chrono::time_point<std::chrono::high_resolution_clock>& last_time, u32& frames)
{
    auto now = std::chrono::high_resolution_clock::now();

    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count();
    if (seconds >= 1)
    {
        printf("%d\n", frames);
        frames = 0;
        last_time = now;
    }
    else 
    {
        frames++;
    }
}

#if defined(RENDER_NAMETABLE)
u8 nt_screen[256 * 240 * 3];
#endif


int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("No ROM specified\n");
        return -1;
    }

    Gfx gfx(3);

    Rom rom;
    if (!rom.Load(std::string(argv[1])))
    {
        printf("Incompatible ROM\n");
        return -1;
    }

    std::shared_ptr<IMapper> mapper = IMapper::CreateMapper(rom);
    if (mapper == nullptr)
    {
        return -1;
    }

    VRam vram(mapper);

    Ppu ppu(vram);
    Apu apu(false /* isPal */);
    Input input;
    MemoryMap mem(ppu, apu, input, mapper);
    Cpu cpu(&mem);

    cpu.Reset();
    
    apu.StartAudio(44100);

    auto last_time = std::chrono::high_resolution_clock::now();
    u32 frames = 0;

    ApuStepResult apuResult;
    PpuStepResult ppuResult;
    for (;;)
    {
        apuResult.Reset();
        ppuResult.Reset();

        input.CheckInput();

        cpu.Step();

        apu.Step(cpu.Cycles, apuResult);
        ppu.Step(cpu.Cycles, ppuResult);
        if (ppuResult.VBlankNmi)
        {
            cpu.Nmi();
        } 
        // else if IRQ

        if (ppuResult.NewFrame)
        {
#if defined(RENDER_NAMETABLE)
            for (int i = 0; i < 4; i++)
            {
                ppu.RenderNameTable(nt_screen, i);
                gfx.BlitNameTable(nt_screen, i);
            }
#endif
            gfx.Blit(ppu.Screen);
            calc_fps(last_time, frames);
        }

        cpu.Cycles = 0;
    }

    apu.StopAudio();

    return 0;
}
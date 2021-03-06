/*
    armvfp.c - ARM VFPv3 emulation unit
    Copyright (C) 2003 Skyeye Develop Group
    for help please send mail to <skyeye-developer@lists.gro.clinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* Note: this file handles interface with arm core and vfp registers */

#include "common/common.h"
#include "common/logging/log.h"

#include "core/arm/skyeye_common/armdefs.h"
#include "core/arm/skyeye_common/vfp/asm_vfp.h"
#include "core/arm/skyeye_common/vfp/vfp.h"

unsigned VFPInit(ARMul_State* state)
{
    state->VFP[VFP_FPSID] = VFP_FPSID_IMPLMEN<<24 | VFP_FPSID_SW<<23 | VFP_FPSID_SUBARCH<<16 |
                            VFP_FPSID_PARTNUM<<8 | VFP_FPSID_VARIANT<<4 | VFP_FPSID_REVISION;
    state->VFP[VFP_FPEXC] = 0;
    state->VFP[VFP_FPSCR] = 0;

    return 0;
}

void VMSR(ARMul_State* state, ARMword reg, ARMword Rt)
{
    if (reg == 1)
    {
        state->VFP[VFP_FPSCR] = state->Reg[Rt];
    }
    else if (reg == 8)
    {
        state->VFP[VFP_FPEXC] = state->Reg[Rt];
    }
}

void VMOVBRS(ARMul_State* state, ARMword to_arm, ARMword t, ARMword n, ARMword* value)
{
    if (to_arm)
    {
        *value = state->ExtReg[n];
    }
    else
    {
        state->ExtReg[n] = *value;
    }
}

void VMOVBRRD(ARMul_State* state, ARMword to_arm, ARMword t, ARMword t2, ARMword n, ARMword* value1, ARMword* value2)
{
    if (to_arm)
    {
        *value2 = state->ExtReg[n*2+1];
        *value1 = state->ExtReg[n*2];
    }
    else
    {
        state->ExtReg[n*2+1] = *value2;
        state->ExtReg[n*2] = *value1;
    }
}
void VMOVBRRSS(ARMul_State* state, ARMword to_arm, ARMword t, ARMword t2, ARMword n, ARMword* value1, ARMword* value2)
{
    if (to_arm)
    {
        *value1 = state->ExtReg[n+0];
        *value2 = state->ExtReg[n+1];
    }
    else
    {
        state->ExtReg[n+0] = *value1;
        state->ExtReg[n+1] = *value2;
    }
}

void VMOVI(ARMul_State* state, ARMword single, ARMword d, ARMword imm)
{
    if (single)
    {
        state->ExtReg[d] = imm;
    }
    else
    {
        /* Check endian please */
        state->ExtReg[d*2+1] = imm;
        state->ExtReg[d*2] = 0;
    }
}
void VMOVR(ARMul_State* state, ARMword single, ARMword d, ARMword m)
{
    if (single)
    {
        state->ExtReg[d] = state->ExtReg[m];
    }
    else
    {
        /* Check endian please */
        state->ExtReg[d*2+1] = state->ExtReg[m*2+1];
        state->ExtReg[d*2] = state->ExtReg[m*2];
    }
}

/* Miscellaneous functions */
int32_t vfp_get_float(ARMul_State* state, unsigned int reg)
{
    LOG_TRACE(Core_ARM11, "VFP get float: s%d=[%08x]\n", reg, state->ExtReg[reg]);
    return state->ExtReg[reg];
}

void vfp_put_float(ARMul_State* state, int32_t val, unsigned int reg)
{
    LOG_TRACE(Core_ARM11, "VFP put float: s%d <= [%08x]\n", reg, val);
    state->ExtReg[reg] = val;
}

uint64_t vfp_get_double(ARMul_State* state, unsigned int reg)
{
    uint64_t result = ((uint64_t) state->ExtReg[reg*2+1])<<32 | state->ExtReg[reg*2];
    LOG_TRACE(Core_ARM11, "VFP get double: s[%d-%d]=[%016llx]\n", reg * 2 + 1, reg * 2, result);
    return result;
}

void vfp_put_double(ARMul_State* state, uint64_t val, unsigned int reg)
{
    LOG_TRACE(Core_ARM11, "VFP put double: s[%d-%d] <= [%08x-%08x]\n", reg * 2 + 1, reg * 2, (uint32_t)(val >> 32), (uint32_t)(val & 0xffffffff));
    state->ExtReg[reg*2] = (uint32_t) (val & 0xffffffff);
    state->ExtReg[reg*2+1] = (uint32_t) (val>>32);
}

/*
 * Process bitmask of exception conditions. (from vfpmodule.c)
 */
void vfp_raise_exceptions(ARMul_State* state, u32 exceptions, u32 inst, u32 fpscr)
{
    LOG_TRACE(Core_ARM11, "VFP: raising exceptions %08x\n", exceptions);

    if (exceptions == VFP_EXCEPTION_ERROR) {
        LOG_TRACE(Core_ARM11, "unhandled bounce %x\n", inst);
        exit(-1);
        return;
    }

    /*
     * If any of the status flags are set, update the FPSCR.
     * Comparison instructions always return at least one of
     * these flags set.
     */
    if (exceptions & (FPSCR_NFLAG|FPSCR_ZFLAG|FPSCR_CFLAG|FPSCR_VFLAG))
        fpscr &= ~(FPSCR_NFLAG|FPSCR_ZFLAG|FPSCR_CFLAG|FPSCR_VFLAG);

    fpscr |= exceptions;

    state->VFP[VFP_FPSCR] = fpscr;
}

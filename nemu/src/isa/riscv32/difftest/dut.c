/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"

// 将NEMU的寄存器状态拷贝给REF
void isa_difftest_regcpy(void *dut_reg, bool direction) {
  if (direction == DIFFTEST_TO_DUT) { // REF -> DUT
    for (int i = 0; i < 32; i++) {
      cpu.gpr[i] = ((uint64_t *)dut_reg)[i];
    }
    cpu.pc = ((uint64_t *)dut_reg)[32];
  } else { // DUT -> REF
    for (int i = 0; i < 32; i++) {
      ((uint64_t *)dut_reg)[i] = cpu.gpr[i];
    }
    ((uint64_t *)dut_reg)[32] = cpu.pc;
  }
}
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  // 检查32个通用寄存器
  for (int i = 0; i < 32; i++) {
    if (ref_r->gpr[i] != cpu.gpr[i]) {
      Log("difftest: GPR[%d] (%s) is different after executing instruction at pc = 0x%08x",
          i, reg_name(i), pc);
      Log("           REF = 0x%08x, DUT = 0x%08x", ref_r->gpr[i], cpu.gpr[i]);
      return false; // 发现不一致, 返回false
    }
  }

  // 检查PC
  if (ref_r->pc != cpu.pc) {
    Log("difftest: PC is different after executing instruction at pc = 0x%08x", pc);
    Log("           REF = 0x%08x, DUT = 0x%08x", ref_r->pc, cpu.pc);
    return false; // 发现不一致, 返回false
  }

  return true; // 所有寄存器都一致, 返回true
}

void isa_difftest_attach() {
}

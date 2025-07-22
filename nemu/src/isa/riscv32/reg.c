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
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  int i;
  // 循环打印 x0 到 x31 通用寄存器
  for (i = 0; i < 32; i++) {
    printf("%-4s: 0x%-10x %-10d\n", regs[i], cpu.gpr[i], cpu.gpr[i]);
  }
  // 单独打印程序计数器 pc
  printf("pc  : 0x%-10x %-10d\n", cpu.pc, cpu.pc);
}

#include <isa.h>
#include "local-include/reg.h" // 确保包含了 reg.h，其中有 regs[] 数组的声明

// regs[] 数组应该已经在这个文件或者它包含的头文件里定义好了
// static const char *regs[] = { ... };

word_t isa_reg_str2val(const char *s, bool *success) {
  // 默认设置为成功
  *success = true;

  // 1. 首先，单独检查是否是程序计数器 "pc"
  if (strcmp(s, "pc") == 0) {
    return cpu.pc;
  }

  // 2. 如果不是 "pc"，则遍历通用寄存器名称数组
  int i;
  for (i = 0; i < 32; i++) {
    // 使用 strcmp 比较传入的字符串和寄存器名称
    if (strcmp(s, regs[i]) == 0) {
      // 找到匹配项，返回 gpr 数组中对应下标的值
      return cpu.gpr[i];
    }
  }

  // 3. 如果循环结束还没有找到匹配项，说明寄存器名无效
  *success = false;
  printf("Error: Unknown register name '%s'\n", s);
  return 0; // 返回一个无意义的值
}

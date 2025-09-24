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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include "memory/paddr.h"
#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S,TYPE_J,TYPE_B,TYPE_R,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immB() do { *imm = SEXT(((BITS(i, 31, 31) << 12) | (BITS(i, 7, 7) << 11) | (BITS(i, 30, 25) << 5) | (BITS(i, 11, 8) << 1)), 13); } while(0)
#define immJ() do { *imm = SEXT(((BITS(i, 31, 31) << 20) | (BITS(i, 19, 12) << 12) | (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1)), 21); } while(0)
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_N: break;
    case TYPE_J:                   immJ(); break;
    case TYPE_R: src1R(); src2R();         break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    default: panic("unsupported type = %d", type);
  }
}

static int decode_exec(Decode *s)
{
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)         \
  {                                                                  \
    int rd = 0;                                                      \
    word_t src1 = 0, src2 = 0, imm = 0;                              \
    decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
    __VA_ARGS__;                                                     \
  }

  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R(rd) = src1 + imm);
  // slti
  INSTPAT("????? ????? ????? 010 ????? 0010011", slti, I, R(rd) = ((sword_t)src1 < (sword_t)imm) ? 1 : 0);
  // xori
  INSTPAT("????? ????? ????? 100 ????? 0010011", xori, I, R(rd) = src1 ^ imm);
  // ori
  INSTPAT("????? ????? ????? 110 ????? 0010011", ori, I, R(rd) = src1 | imm);
  // andi
  INSTPAT("????? ????? ????? 111 ????? 0010011", andi, I, R(rd) = src1 & imm);
  // slli
  INSTPAT("0000000 ????? ????? 001 ????? 0010011", slli, I, R(rd) = src1 << (imm & 0x1f));
  // srli
  INSTPAT("0000000 ????? ????? 101 ????? 0010011", srli, I, R(rd) = src1 >> (imm & 0x1f));
  // srai
  INSTPAT("0100000 ????? ????? 101 ????? 0010011", srai, I, R(rd) = (sword_t)src1 >> (imm & 0x1f));
  // sub
  INSTPAT("0100000 ????? ????? 000 ????? 0110011", sub, R, R(rd) = src1 - src2);
  // sll
  INSTPAT("0000000 ????? ????? 001 ????? 0110011", sll, R, R(rd) = src1 << (src2 & 0x1f));
  // slt
  INSTPAT("0000000 ????? ????? 010 ????? 0110011", slt, R, R(rd) = ((sword_t)src1 < (sword_t)src2) ? 1 : 0);
  // srl
  INSTPAT("0000000 ????? ????? 101 ????? 0110011", srl, R, R(rd) = src1 >> (src2 & 0x1f));
  // sra
  INSTPAT("0100000 ????? ????? 101 ????? 0110011", sra, R, R(rd) = (sword_t)src1 >> (src2 & 0x1f));
  // and
  INSTPAT("0000000 ????? ????? 111 ????? 0110011", and, R, R(rd) = src1 & src2);
  // lui (Load Upper Immediate) - U-Type
  INSTPAT("????? ????? ????? ??? ????? 0110111", lui, U, R(rd) = imm);
  // blt
  INSTPAT("????? ????? ????? 100 ????? 1100011", blt, B, {
    if ((sword_t)src1 < (sword_t)src2)
    {
      s->dnpc = s->pc + imm;
    }
  });
  // bge
  INSTPAT("????? ????? ????? 101 ????? 1100011", bge, B, {
    if ((sword_t)src1 >= (sword_t)src2)
    {
      s->dnpc = s->pc + imm;
    }
  });
  // bgeu
  INSTPAT("????? ????? ????? 111 ????? 1100011", bgeu, B, {
    if (src1 >= src2)
    {
      s->dnpc = s->pc + imm;
    }
  });
  // bltu
  INSTPAT("????? ????? ????? 110 ????? 1100011", bltu, B, {
    if (src1 < src2)
    {
      s->dnpc = s->pc + imm;
    }
  });
  // lb
  INSTPAT("??????? ????? ????? 000 ????? 0000011", lb, I, {
    R(rd) = SEXT(Mr(src1 + imm, 1), 8);
  });
  // lh
  INSTPAT("??????? ????? ????? 001 ????? 0000011", lh, I, {
    R(rd) = SEXT(Mr(src1 + imm, 2), 16);
  });
  // lhu
  INSTPAT("??????? ????? ????? 101 ????? 0000011", lhu, I, {
    R(rd) = Mr(src1 + imm, 2);
  });
  // sh
  INSTPAT("??????? ????? ????? 001 ????? 0100011", sh, S, Mw(src1 + imm, 2, src2));
  //// jal (Jump and Link) - J-Type
  INSTPAT("????? ????? ????? ??? ????? 1101111", jal, J, {
    // 1. Link: 如果目标寄存器 rd 不是 x0, 就保存返回地址 (pc + 4)
    if (rd != 0)
    {
      R(rd) = s->pc + 4;
    }
    // 2. Jump: 设置下一条指令的PC为当前PC + 立即数
    s->dnpc = s->pc + imm;
  });
  //// jalr (Jump and Link Register) - I-Type
  INSTPAT("????? ????? ????? 000 ????? 1100111", jalr, I, {
    // 暂存一下pc + 4, 作为可能的返回地址
    word_t link_addr = s->pc + 4;

    // 直接使用 src1 (代表rs1寄存器的值) 和 imm (代表立即数的值)
    // 计算跳转目标地址，并确保最低位为0
    s->dnpc = (src1 + imm) & ~1;

    // 如果rd不是x0, 才需要保存返回地址
    if (rd != 0)
    {
      // 这里我们假设可以直接用 rd (寄存器编号) 和 cpu.gpr 来写入
      // 这与您之前成功的修复风格一致
      R(rd) = link_addr;
    }
  });
  //// sw (Store Word) - S-Type
  INSTPAT("??????? ????? ????? 010 ????? 0100011", sw, S, Mw(src1 + imm, 4, src2));
  //// lw (Load Word) - I-Type
  INSTPAT("????? ????? ????? 010 ????? 0000011", lw, I, {
    // 使用 Mr 宏从 (rs1的值 + imm的值) 地址读取4个字节
    R(rd) = Mr(src1 + imm, 4);
  });
  //// add (Add) - R-Type
  INSTPAT("0000000 ????? ????? 000 ????? 0110011", add, R, {
    R(rd) = src1 + src2; // 直接使用 src1 和 src2 的值
  });
  //// sltu (Set if Less Than Unsigned) - R-Type
  INSTPAT("0000000 ????? ????? 011 ????? 0110011", sltu, R, {
    // src1 和 src2 都是 word_t (无符号),可以直接比较
    R(rd) = (src1 < src2) ? 1 : 0;
  });

  //// xor (Exclusive Or) - R-Type
  INSTPAT("0000000 ????? ????? 100 ????? 0110011", xor, R, {
    R(rd) = src1 ^ src2;
  });

  //// or (Or) - R-Type
  INSTPAT("0000000 ????? ????? 110 ????? 0110011", or, R, {
    R(rd) = src1 | src2;
  });

  // [ I-Type Arithmetic/Logic ]
  //// sltiu (Set if Less Than Immediate Unsigned) - I-Type
  // 用于实现 seqz 伪指令
  INSTPAT("????? ????? ????? 011 ????? 0010011", sltiu, I, {
    R(rd) = (src1 < imm) ? 1 : 0;
  });

  // [ Branch ]
  //// beq (Branch if Equal) - B-Type
  // 用于实现 beqz 伪指令
  INSTPAT("????? ????? ????? 000 ????? 1100011", beq, B, {
    if (src1 == src2)
    {
      s->dnpc = s->pc + imm;
    }
  });

  //// bne (Branch if Not Equal) - B-Type
  INSTPAT("????? ????? ????? 001 ????? 1100011", bne, B, {
    if (src1 != src2)
    {
      s->dnpc = s->pc + imm;
    }
  });
  // mul
  INSTPAT("0000001 ????? ????? 000 ????? 0110011", mul, R, R(rd) = src1 * src2);
  // mulh
  INSTPAT("0000001 ????? ????? 001 ????? 0110011", mulh, R, R(rd) = ((int64_t)(sword_t)src1 * (int64_t)(sword_t)src2) >> 32);
  // mulhsu
  INSTPAT("0000001 ????? ????? 010 ????? 0110011", mulhsu, R, R(rd) = ((int64_t)(sword_t)src1 * (uint64_t)src2) >> 32);
  // mulhu
  INSTPAT("0000001 ????? ????? 011 ????? 0110011", mulhu, R, R(rd) = ((uint64_t)src1 * (uint64_t)src2) >> 32);
  // div
  INSTPAT("0000001 ????? ????? 100 ????? 0110011", div, R, {
    if (src2 == 0)
    {
      R(rd) = -1;
    }
    else if (src1 == 0x80000000 && src2 == 0xffffffff)
    {
      R(rd) = 0x80000000;
    }
    else
    {
      R(rd) = (sword_t)src1 / (sword_t)src2;
    }
  });
  // divu
  INSTPAT("0000001 ????? ????? 101 ????? 0110011", divu, R, {
    if (src2 == 0)
    {
      R(rd) = -1;
    }
    else
    {
      R(rd) = src1 / src2;
    }
  });
  // rem
  INSTPAT("0000001 ????? ????? 110 ????? 0110011", rem, R, {
    if (src2 == 0)
    {
      R(rd) = src1;
    }
    else if (src1 == 0x80000000 && src2 == 0xffffffff)
    {
      R(rd) = 0;
    }
    else
    {
      R(rd) = (sword_t)src1 % (sword_t)src2;
    }
  });
  // remu
  INSTPAT("0000001 ????? ????? 111 ????? 0110011", remu, R, {
    if (src2 == 0)
    {
      R(rd) = src1;
    }
    else
    {
      R(rd) = src1 % src2;
    }
  });
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));

  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s)
{
  s->isa.inst = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}

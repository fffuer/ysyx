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
#include <ftrace.h>
#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include "../monitor/sdb/sdb.h"
#include <memory/paddr.h>
/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10
// 定义环形缓冲区的大小
#define IRINGBUF_SIZE 16
// 定义每条日志的最大长度
#define LOG_ENTRY_SIZE 128

// 环形缓冲区的数据结构 (声明为 static, 成为文件私有)
static char iringbuf[IRINGBUF_SIZE][LOG_ENTRY_SIZE];
static int p_head = 0;
static bool is_full = false;

// 将日志写入缓冲区的函数 (声明为 static)
static void iringbuf_write(const char *log) {
  // 使用 snprintf 安全地将日志内容写入环形缓冲区
  // 它能防止溢出并保证字符串以'\0'结尾
  snprintf(iringbuf[p_head], LOG_ENTRY_SIZE, "%s", log);

  // 清理可能存在的换行符
  char *p = strchr(iringbuf[p_head], '\n');
  if (p) {
    *p = '\0';
  }

  // 更新头指针
  p_head = (p_head + 1) % IRINGBUF_SIZE;
  if (p_head == 0) {
    is_full = true;
  }
}


// 打印缓冲区内容的函数 (声明为 static)
static void iringbuf_display() {
  printf("\nInstruction Ring Buffer (last %d instructions):\n", IRINGBUF_SIZE);
  printf("--------------------------------------------------\n");

  if (!is_full && p_head == 0) {
    printf("(Buffer is empty)\n");
    return;
  }

  int start_pos = is_full ? p_head : 0;
  int count = is_full ? IRINGBUF_SIZE : p_head;

  for (int i = 0; i < count; i++) {
    int index = (start_pos + i) % IRINGBUF_SIZE;
    if (index == (p_head - 1 + IRINGBUF_SIZE) % IRINGBUF_SIZE) {
      // 用箭头指向最新执行的指令
      printf("--> %s\n", iringbuf[index]);
    } else {
      printf("    %s\n", iringbuf[index]);
    }
  }
  printf("--------------------------------------------------\n");
}


CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

void device_update();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); iringbuf_write(_this->logbuf);}
#endif
// --- FTRACE START: 仿照itrace的模式 ---
#ifdef CONFIG_FTRACE
  char ftrace_buf[128];
  // a. 准备ftrace日志
  uint32_t inst = paddr_read(_this->pc, 4);
  if (ftrace_log(ftrace_buf, sizeof(ftrace_buf), _this->pc, dnpc, inst)) {
    // b. 如果生成了日志, 直接打印到控制台
    puts(ftrace_buf); 
  }
#endif
// --- FTRACE END ---
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
  // 检查所有监视点
  if (check_watchpoints()) {
    // 如果有监视点被触发，设置状态为 NEMU_STOP
    nemu_state.state = NEMU_STOP;
  }
}

static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst;
#ifdef CONFIG_ISA_x86
  for (i = 0; i < ilen; i ++) {
#else
  for (i = ilen - 1; i >= 0; i --) {
#endif
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst, ilen);
#endif
}

static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst ++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
  isa_reg_display();
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT: case NEMU_QUIT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      iringbuf_display();
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT: statistic();
  }
}

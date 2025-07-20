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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "si [N] - Step N instructions (default 1)", cmd_si },
  { "info", "info [r|w] - Print program state (r: registers, w: watchpoints)", cmd_info },
  { "x", "x N EXPR - Scan N 4-byte words from memory address EXPR", cmd_x },
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  uint64_t n = 1; // 当没有给出参数时，缺省为 1

  // args 是 sdb_mainloop 传递过来的参数字符串
  // 如果命令是 "si", args 为 NULL
  // 如果命令是 "si 10", args 指向字符串 "10"
  if (args != NULL) {
    // 使用 C 标准库函数 atoi() 将参数字符串转换为整数
    n = atoi(args);
  }

  // 一个小健壮性处理：如果用户输入 "si 0" 或 "si abc"，
  // atoi 会返回 0，此时我们不执行任何指令。
  if (n == 0 && args != NULL) {
      printf("Si n, where n is an integer greater than or equal to 1\n");
      // 保持沉默或给个提示
      return 0;
  }

  cpu_exec(n);

  return 0;
}

// 和其他 cmd_* 函数放在一起
static int cmd_info(char *args) {
  if (args == NULL) {
    printf("Argument required. Usage: info [r|w]\n");
    return 0;
  }

  if (strcmp(args, "r") == 0) {
    // 调用我们刚刚为 riscv32 实现的函数
    isa_reg_display();
  }
  else if (strcmp(args, "w") == 0) {
    printf("Watchpoint status is not implemented yet.\n");
  }
  else {
    printf("Unknown subcommand '%s' for 'info'. Use 'r' for registers or 'w' for watchpoints.\n", args);
  }

  return 0;
}

static int cmd_x(char *args) {
  char *arg_n = strtok(args, " ");
  char *arg_expr = strtok(NULL, " ");

  if (arg_n == NULL || arg_expr == NULL) {
    printf("Usage: x N EXPR (e.g., x 10 0x80000000)\n");
    return 0;
  }

  // 1. 解析参数 N
  int n = atoi(arg_n);

  // 2. 解析表达式 EXPR (十六进制数)
  // 使用 strtol 函数，它可以将字符串以指定进制转换为长整型
  // 第三个参数 16 表示按十六进制解析
  paddr_t start_addr = strtol(arg_expr, NULL, 16);

  // 3. 循环扫描并打印内存
  int i;
  for (i = 0; i < n; i++) {
    paddr_t current_addr = start_addr + i * 4;
    word_t data = paddr_read(current_addr, 4);

    // 打印地址和数据
    // 0x%08x: 打印8位十六进制数，不足8位前面补0
    // 0x%08x: 同样格式打印读取到的4字节数据
    printf("0x%08x: 0x%08x\n", current_addr, data);
  }

  return 0;
}
void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}

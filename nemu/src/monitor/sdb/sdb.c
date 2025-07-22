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

static int cmd_p(char *args);

static int cmd_test(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

word_t expr(char *e, bool *success);

WP* new_wp();

void free_wp(int no);

void display_watchpoints();


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
  { "p", "p EXPR - Evaluate expression EXPR", cmd_p },
  { "test", "test FILE - Automated test for expression evaluation from a file", cmd_test },
  { "w", "w EXPR - Set a watchpoint on an expression", cmd_w },
  { "d", "d N - Delete watchpoint N", cmd_d },
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
    display_watchpoints();
  }
  else {
    printf("Unknown subcommand '%s' for 'info'. Use 'r' for registers or 'w' for watchpoints.\n", args);
  }

  return 0;
}

static int cmd_x(char *args) {
  // 1. 解析第一个参数 N (要扫描的数量)
  char *arg_n = strtok(args, " ");
  if (arg_n == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }
  int n = atoi(arg_n);
  if (n <= 0) {
    printf("Error: N must be a positive integer.\n");
    return 0;
  }

  // 2. 获取表达式 EXPR 的字符串
  // args 字符串在被 strtok 处理后，"N" 和 EXPR 之间被 '\0' 分隔。
  // arg_expr 指向 N 后面的第一个字符，也就是表达式的开头。
  char *arg_expr = arg_n + strlen(arg_n) + 1;
  if (*arg_expr == '\0') {
      printf("Usage: x N EXPR\n");
      return 0;
  }

  // 3. 调用 expr() 函数来计算表达式的值，得到起始地址
  bool success = true;
  paddr_t start_addr = expr(arg_expr, &success);

  // 检查表达式求值是否成功
  if (!success) {
    printf("Invalid expression for start address.\n");
    return 0;
  }

  // 4. 循环扫描并打印内存 (这部分逻辑保持不变)
  printf("Scanning memory from address 0x%08x:\n", start_addr);
  int i;
  for (i = 0; i < n; i++) {
    paddr_t current_addr = start_addr + i * 4;
    // 使用 paddr_read 读取4个字节的数据
    word_t data = paddr_read(current_addr, 4);

    // 打印地址和数据
    printf("0x%08x: 0x%08x\n", current_addr, data);
  }

  return 0;
}
// 声明我们将要使用的表达式求值函数


// 实现 test 命令的处理函数
static int cmd_test(char *args) {
    if (args == NULL) {
        printf("Usage: test FILE_PATH\n");
        return 0;
    }

    FILE *fp = fopen(args, "r");
    if (fp == NULL) {
        printf("Cannot open file '%s'\n", args);
        return 0;
    }

    printf("Starting automated test from file '%s'...\n", args);
    int line_num = 0;
    char line[65536];
    bool all_passed = true;

    // 逐行读取文件
    while (fgets(line, sizeof(line), fp) != NULL) {
        line_num++;
        // 去掉行尾的换行符
        line[strcspn(line, "\n")] = 0;

        // 1. 解析出文件中的正确结果和表达式
        uint32_t correct_result;
        char expression_str[65536];
        sscanf(line, "%u %[^\n]", &correct_result, expression_str);

        // 2. 使用我们自己实现的 expr() 函数计算
        bool success = true;
        uint32_t my_result = expr(expression_str, &success);
        
        // 3. 对比结果
        if (!success || my_result != correct_result) {
            printf("Validation Failed at line %d!\n", line_num);
            printf("Expression: %s\n", expression_str);
            printf("Expected Result: %u\n", correct_result);
            printf("Your Result: %u\n", my_result);
            all_passed = false;
            break; // 遇到第一个错误就停止
        }
    }

    fclose(fp);

    if (all_passed) {
        printf("Congratulations! All %d tests from file '%s' passed!\n", line_num, args);
    }
    return 0;
}


static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }
  bool success = true;
  word_t result = expr(args, &success);
  if (success) {
    printf("Expression value: %u (0x%x)\n", result, result);
  } else {
    printf("Invalid expression\n");
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

static int cmd_w(char *args) {
  if (args == NULL) {
    printf("Usage: w EXPR\n");
    return 0;
  }

  WP *wp = new_wp();
  if (wp == NULL) {
    return 0; // new_wp 内部会打印错误信息
  }

  // 记录表达式
  strncpy(wp->expr_str, args, sizeof(wp->expr_str) - 1);
  wp->expr_str[sizeof(wp->expr_str) - 1] = '\0';

  // 计算初始值
  bool success = true;
  wp->old_val = expr(args, &success);

  if (!success) {
    printf("Invalid expression. Watchpoint creation failed.\n");
    free_wp(wp->NO); // 归还刚刚申请的节点
    return 0;
  }

  printf("Watchpoint %d: %s\n", wp->NO, wp->expr_str);
  return 0;
}

// d 命令: 删除监视点
static int cmd_d(char *args) {
  if (args == NULL) {
    printf("Usage: d N\n");
    return 0;
  }
  int no = atoi(args);
  free_wp(no);
  return 0;
}




void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}

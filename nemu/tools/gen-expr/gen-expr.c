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



/*
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

// Recursive helper function to generate expressions
static void gen_recursive(int level) {
  // Base case: when recursion is deep, generate a number
  // The chance of generating a number increases with depth
  if (level > 4 || (level > 0 && rand() % 4 == 0) ) {
    char num_str[10];
    // Generate a non-zero number from 1 to 100 to avoid division by zero issues
    unsigned int num = rand() % 100 + 1;
    sprintf(num_str, "%u", num);
    strcat(buf, num_str);
    return;
  }

  // Recursive step: generate (sub-expression op sub-expression)
  strcat(buf, "(");
  gen_recursive(level + 1); // Left operand

  // Add a random operator
  char op_str[4];
  switch (rand() % 4) {
    case 0: sprintf(op_str, " + "); break;
    case 1: sprintf(op_str, " - "); break;
    case 2: sprintf(op_str, " * "); break;
    default: sprintf(op_str, " / "); break;
  }
  strcat(buf, op_str);

  gen_recursive(level + 1); // Right operand
  strcat(buf, ")");
}

// This function is called by main to generate a random expression
static void gen_rand_expr() {
  // Clear the buffer before generating a new expression
  buf[0] = '\0';
  // Start the recursive generation from the top level (level 0)
  gen_recursive(0);
}

int main(int argc, char *argv[]) {
    int seed = time(0);
    srand(seed);
    int loop = 1;
    if (argc > 1) {
        sscanf(argv[1], "%d", &loop);
    }
    int i;
    for (i = 0; i < loop; i++) {
        gen_rand_expr();

        sprintf(code_buf, code_format, buf);

        FILE *fp = fopen("/tmp/.code.c", "w");
        assert(fp != NULL);
        fputs(code_buf, fp);
        fclose(fp);

        int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
        if (ret != 0) {
            // 如果编译失败(比如生成了不合法的C代码)，重试
            continue;
        }

        fp = popen("/tmp/.expr", "r");
        assert(fp != NULL);

        unsigned  result;
        // --- 修改开始 ---
        int fscanf_ret = fscanf(fp, "%d", &result);
        int pclose_ret = pclose(fp);

        // 检查 fscanf 是否成功读取 (返回值应为1)
        // 并且检查子进程是否正常退出 (pclose 返回0代表正常)
        if (fscanf_ret != 1 || pclose_ret != 0) {
            // 如果任一条件不满足，说明子进程很可能因为除0等错误而崩溃
            // 我们丢弃这个测试用例，并通过 i-- 来重新生成一个，以确保最终生成loop个有效的用例
            //printf("Skipping invalid expression: %s\n", buf); // (可选) 打印被跳过的表达式
            //i--;
            continue;
        }
        // --- 修改结束 ---

        printf("%u %s\n", result, buf);
    }
    return 0;
}
*/

/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
* http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
* http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

// 缓冲区用于存放生成的表达式字符串
static char buf[65536] = {};

// `gen_final` 在生成字符串的同时，返回计算结果
// 它通过返回一个特殊的标记来表示是否遇到了除零
#define DIV_BY_ZERO_FLAG 0xFFFFFFFF
uint32_t gen_final(int level) {
  // 递归终点：当递归深度足够大，生成一个数字
  // 增加了当level > 0时的概率，以生成更短的表达式
  if (level > 4 || (level > 0 && rand() % 3 == 0) ) {
    uint32_t num = rand() % 100 + 1;
    sprintf(buf + strlen(buf), "%u", num);
    return num;
  }

  // 递归步骤
  strcat(buf, "(");
  uint32_t val1 = gen_final(level + 1);
  if (val1 == DIV_BY_ZERO_FLAG) return DIV_BY_ZERO_FLAG; // 传播错误标记

  strcat(buf, " ");
  
  // --- 修改核心：增加新的运算符 ---
  char op_str[5]; // 增加长度以容纳 "&&" 和 " != "
  char op_type;

  // 现在有7种运算符
  switch (rand() % 7) {
    case 0: op_type = '+'; sprintf(op_str, "+"); break;
    case 1: op_type = '-'; sprintf(op_str, "-"); break;
    case 2: op_type = '*'; sprintf(op_str, "*"); break;
    case 3: op_type = '/'; sprintf(op_str, "/"); break;
    case 4: op_type = 'e'; sprintf(op_str, "=="); break; // 'e' for equal
    case 5: op_type = 'n'; sprintf(op_str, "!="); break; // 'n' for not equal
    default: op_type = '&'; sprintf(op_str, "&&"); break; // '&' for logical and
  }
  
  strcat(buf, op_str);
  strcat(buf, " ");

  uint32_t val2 = gen_final(level + 1);
  if (val2 == DIV_BY_ZERO_FLAG) return DIV_BY_ZERO_FLAG; // 传播错误标记
  
  strcat(buf, ")");

  // 检查除零
  if (op_type == '/' && val2 == 0) {
    return DIV_BY_ZERO_FLAG; // 发现除零，返回错误标记
  }

  // --- 修改核心：增加新运算符的计算逻辑 ---
  switch (op_type) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/': return val1 / val2;
    case 'e': return val1 == val2; // ==
    case 'n': return val1 != val2; // !=
    case '&': return val1 && val2; // &&
    default: assert(0);
  }
}

int main(int argc, char *argv[]) {
    int seed = time(0);
    srand(seed);
    int loop = 1;
    if (argc > 1) {
        sscanf(argv[1], "%d", &loop);
    }
    int i;
    for (i = 0; i < loop; i++) {
        uint32_t result;
        // 循环直到生成一个有效的表达式
        while (1) {
            buf[0] = '\0'; // 清空缓冲区
            result = gen_final(0);
            if (result != DIV_BY_ZERO_FLAG) {
                // 生成成功，跳出循环
                break;
            }
        }

        // 打印合法的表达式及其正确结果
        printf("%u %s\n", result, buf);
    }
    return 0;
}

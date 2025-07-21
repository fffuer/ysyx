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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
// 缓冲区用于存放生成的表达式字符串
static char buf[65536] = {};

/* * 核心修改：这是一个新的递归函数。
 * 它不仅生成表达式字符串，还同时用无符号32位运算计算出其值。
 * 返回值: 生成的子表达式的正确计算结果。
 * 功能: 将生成的子表达式字符串追加到全局的 buf 中。
 */
static uint32_t gen_recursive_with_calc(int level) {
  // 递归终点：当递归深度足够大，或随机触发时，生成一个数字。
  if (level > 4 || (level > 0 && rand() % 3 == 0)) {
    // 生成一个1到100之间的随机数
    uint32_t num = rand() % 100 + 1;
    char num_str[10];
    sprintf(num_str, "%u", num);
    strcat(buf, num_str); // 追加到字符串
    return num;           // 返回其数值
  }

  // 递归步骤：生成 (左表达式 op 右表达式)
  strcat(buf, "(");
  uint32_t val1 = gen_recursive_with_calc(level + 1); // 递归生成左边，并获取其值
  strcat(buf, " "); // 在操作符前后加空格，更美观

  // --- 关键的预防逻辑在这里 ---
  char op;
  uint32_t val2;

  // 随机选择一个操作符
  switch (rand() % 4) {
    case 0: op = '+'; break;
    case 1: op = '-'; break;
    case 2: op = '*'; break;
    default: op = '/'; break;
  }

  // 递归生成右边，并获取其值
  val2 = gen_recursive_with_calc(level + 1);

  // 如果操作符是除法，并且我们预知到除数是0，则进行处理
  if (op == '/' && val2 == 0) {
    // 策略：将除法替换为加法，确保表达式合法
    // 这样既避免了除零，又保证了表达式的复杂度
    op = '+'; 
  }

  // 将操作符写入缓冲区
  char op_str[2] = {op, '\0'};
  strcat(buf, op_str);
  strcat(buf, " ");

  // 将右表达式追加到缓冲区 (这一步已经在递归调用中完成)
  // [注意] 我们需要重新构建字符串，因为val2可能是在另一个分支中生成的
  // 为了简化，我们直接在val2生成后拼接
  
  // 上述逻辑有误，我们应该先获取val2再决定操作符
  // 我们重新设计一下这部分的逻辑
  // (撤销之前的代码，用更清晰的逻辑重写)
  // [代码重构开始]
  
  // 实际上，我们应该先获取val2, 再决定如何拼接
  // 为了避免破坏字符串，我们用临时缓冲区
  // 但更简单的方法是，如果除法不行，就换个操作符

  // 让我们回到之前的逻辑，它更清晰
  // op已经选好了，val2也已经生成了，字符串也拼接好了
  // 我们现在只需要计算结果

  strcat(buf, ")");

  // 根据最终的操作符，计算结果
  switch (op) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/': return val1 / val2; // 因为我们已经保证了val2不为0
    default: assert(0);
  }
}

// 修改后的 `gen_recursive` 函数，确保不产生除零
// 通过返回一个布尔值来表示是否生成成功
static bool gen_expr_no_div_zero(int level) {
    if (level > 4 || (level > 0 && rand() % 2 == 0)) {
        uint32_t num = rand() % 100 + 1;
        sprintf(buf + strlen(buf), "%u", num);
        return true;
    }

    strcat(buf, "(");
    gen_expr_no_div_zero(level + 1);
    
    char op_str[4];
    switch (rand() % 4) {
        case 0: sprintf(op_str, " + "); break;
        case 1: sprintf(op_str, " - "); break;
        case 2: sprintf(op_str, " * "); break;
        default: sprintf(op_str, " / "); break;
    }
    strcat(buf, op_str);

    // 为了检查右侧是否为0，我们需要先生成它到一个临时缓冲区
    char right_buf[32768] = {};
    // 将`buf`的当前指针传给一个临时变量
    char *buf_ptr_before_right = buf + strlen(buf);
    // 递归生成右边表达式
    gen_expr_no_div_zero(level + 1);
    // 此时右表达式在 `buf` 的后半部分
    strcpy(right_buf, buf_ptr_before_right);
    
    // 计算右表达式的值
    // [注意] 这需要一个求值器，这与我们的初衷相悖
    // 所以，最佳实践就是我们之前的那个版本：预计算
    // 我将为您提供最终的、最健壮的“预计算”版本代码
    return true;
}


// --- 最终的、最健壮的实现 ---

// `gen_final` 在生成字符串的同时，返回计算结果
// 它通过返回一个特殊的标记来表示是否遇到了除零
#define DIV_BY_ZERO_FLAG 0xFFFFFFFF
uint32_t gen_final(int level) {
  if (level > 4) {
    uint32_t num = rand() % 100 + 1;
    sprintf(buf + strlen(buf), "%u", num);
    return num;
  }

  strcat(buf, "(");
  uint32_t val1 = gen_final(level + 1);
  if (val1 == DIV_BY_ZERO_FLAG) return DIV_BY_ZERO_FLAG; // 传播错误标记

  strcat(buf, " ");
  
  char op;
  switch (rand() % 4) {
    case 0: op = '+'; break;
    case 1: op = '-'; break;
    case 2: op = '*'; break;
    default: op = '/'; break;
  }
  
  char op_str[2] = {op, '\0'};
  strcat(buf, op_str);
  strcat(buf, " ");

  uint32_t val2 = gen_final(level + 1);
  if (val2 == DIV_BY_ZERO_FLAG) return DIV_BY_ZERO_FLAG; // 传播错误标记
  
  strcat(buf, ")");

  if (op == '/' && val2 == 0) {
    return DIV_BY_ZERO_FLAG; // 发现除零，返回错误标记
  }

  switch (op) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/': return val1 / val2;
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
            // 否则，buf已被污染，下一次循环会自动清空并重新生成
        }

        // 此时我们已经有了一个合法的表达式及其正确结果
        // 不再需要编译运行外部程序
        printf("%u %s\n", result, buf);
    }
    return 0;
}

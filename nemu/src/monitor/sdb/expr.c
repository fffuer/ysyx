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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdbool.h>
#include <memory/paddr.h>
enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM, TK_HEXNUM, TK_REG,TK_NEQ,   // Not Equal
  TK_LAND,  // Logical AND
  TK_DEREF, //
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"-", '-'},                // minus
  {"\\*", '*'},              // multiply
  {"/", '/'},                // divide
  {"\\(", '('},              // left parenthesis
  {"\\)", ')'},
  {"!=", TK_NEQ},
  {"&&", TK_LAND},            // right parenthesis
  {"\\$[a-zA-Z0-9]+", TK_REG},
  {"0x[0-9a-fA-F]+", TK_HEXNUM}, // hex number
  {"[0-9]+", TK_NUM},         // decimal number
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

bool check_parentheses(int p, int q);
int find_main_op(int p, int q);
/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[1024] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;
        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        switch (rules[i].token_type) {
          case TK_NOTYPE: break; // 忽略空格

          case '*': { // 当匹配到 * 时，需要特殊处理
            bool is_deref = false;
            // 如果是第一个token，或者前一个token是操作符或左括号，则为解引用
            if (nr_token == 0) {
              is_deref = true;
            } else {
              int prev_type = tokens[nr_token - 1].type;
              if (prev_type != TK_NUM && prev_type != TK_HEXNUM &&
                  prev_type != TK_REG && prev_type != ')') {
                is_deref = true;
              }
            }
          
            if (is_deref) {
              tokens[nr_token].type = TK_DEREF;
            } else {
              tokens[nr_token].type = '*';
            }
            nr_token++;
            break;
          }
        
          default:
            if (nr_token >= 1024) { // 使用您之前定义的MAX_TOKENS
              printf("Expression too long\n");
              return false;
            }
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


// static word_t eval(int p, int q, bool *success) {
//   if (p > q) {
//     /* Bad expression */
//     *success = false;
//     return 0;
//   }
//   else if (p == q) {
//     /* Single token.
//      * For now this token should be a number.
//      * Return the value of the number.
//      */
//     word_t val = 0;
//     switch(tokens[p].type) {
//         case TK_NUM:
//             sscanf(tokens[p].str, "%u", &val);
//             return val;
//         case TK_HEXNUM:
//             sscanf(tokens[p].str, "%x", &val);
//             return val;
//         case TK_REG:
//             // 使用框架中的 isa_reg_str2val 函数将寄存器名转换为值
//             return isa_reg_str2val(tokens[p].str + 1, success); // +1 to skip '$'
//         default:
//             *success = false;
//             return 0;
//     }
//   }
//   else if (check_parentheses(p, q) == true) {
//     /* The expression is surrounded by a matched pair of parentheses.
//      * If so, call eval() recursively to evaluate the sub-expression
//      * inside the parentheses.
//      */
//     return eval(p + 1, q - 1, success);
//   }
//   else {
//     int op = find_main_op(p, q); // 寻找主操作符
//     word_t val1 = eval(p, op - 1, success);
//     word_t val2 = eval(op + 1, q, success);

//     switch (tokens[op].type) {
//       case '+': return val1 + val2;
//       case '-': return val1 - val2;
//       case '*': return val1 * val2;
//       case '/': 
//         if (val2 == 0) {
//             printf("Error: Division by zero\n");
//             *success = false;
//             return 0;
//         }
//         return val1 / val2;
//       case TK_EQ: return val1 == val2;
//       default: assert(0);
//     }
//   }
// }
// 在 src/monitor/sdb/expr.c
static word_t eval(int p, int q, bool *success) {
  if (p > q) {
    *success = false;
    return 0;
  }
  else if (p == q) { 
    word_t val = 0;
      switch(tokens[p].type) {
          case TK_NUM:
              sscanf(tokens[p].str, "%u", &val);
              return val;
          case TK_HEXNUM:
              sscanf(tokens[p].str, "%x", &val);
              return val;
          case TK_REG:
             // 使用框架中的 isa_reg_str2val 函数将寄存器名转换为值
             return isa_reg_str2val(tokens[p].str + 1, success); // +1 to skip '$'
          default:
             *success = false;
              return 0;
      }
  }
  else if (check_parentheses(p, q) == true) {
    return eval(p + 1, q - 1, success);
  }
  else {
    int op = find_main_op(p, q);
    
    // --- 处理一元运算符（解引用） ---
    if (op == p && tokens[op].type == TK_DEREF) {
      // 递归计算*后面的表达式，得到地址
      word_t address = eval(p + 1, q, success);
      if (!*success) return 0;
      // 从该地址读取4字节数据
      return paddr_read(address, 4);
    }
    
    // --- 处理二元运算符 ---
    word_t val1 = eval(p, op - 1, success);
    word_t val2 = eval(op + 1, q, success);

    if(!*success) return 0; // 如果子表达式求值失败，立即返回

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': 
        if (val2 == 0) {
            printf("Error: Division by zero\n");
            *success = false;
            return 0;
        }
        return val1 / val2;
      case TK_EQ: return val1 == val2;

      /* 新增的求值逻辑 */
      case TK_NEQ: return val1 != val2;
      case TK_LAND: return val1 && val2;

      default: assert(0);
    }
  }
}

// 主入口函数
word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  *success = true;
  return eval(0, nr_token - 1, success);
}

// 以下是 eval 需要的辅助函数，也放在 expr.c 中

bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }
  int balance = 0;
  for (int i = p + 1; i < q; i++) {
    if (tokens[i].type == '(') balance++;
    if (tokens[i].type == ')') balance--;
    if (balance < 0) return false;
  }
  return balance == 0;
}

int find_main_op(int p, int q) {
    int op = -1;
    int balance = 0;
    int precedence = 10; // 一个比所有操作符优先级都低的初始值

    for (int i = q; i >= p; i--) {
        if (tokens[i].type == ')') { balance++; continue; }
        if (tokens[i].type == '(') { balance--; continue; }
        if (balance != 0) continue; // 忽略括号内的操作符

        int current_precedence = 0;
        switch (tokens[i].type) {
            // 定义优先级
            case TK_LAND: current_precedence = 1; break;
            case TK_EQ: case TK_NEQ: current_precedence = 2; break;
            case '+': case '-': current_precedence = 3; break;
            case '*': case '/': current_precedence = 4; break;
            case TK_DEREF: current_precedence = 5; break; // 最高优先级
            default: continue;
        }

        // 寻找优先级最低的操作符
        if (current_precedence <= precedence) {
            precedence = current_precedence;
            op = i;
        }
    }
    return op;
}

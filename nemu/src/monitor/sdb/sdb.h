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

#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

word_t expr(char *e, bool *success);

#endif
// in src/monitor/sdb/sdb.h

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  // 新增成员
  char expr_str[256]; // 监视的表达式字符串
  uint32_t old_val;   // 表达式的上一次求值结果
} WP;

// 声明管理函数
void init_wp_pool();
WP* new_wp();
void free_wp(int no);
void display_watchpoints();
bool check_watchpoints();

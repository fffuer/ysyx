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

// #include "sdb.h"

// #define NR_WP 32

// typedef struct watchpoint {
//   int NO;
//   struct watchpoint *next;

//   /* TODO: Add more members if necessary */

// } WP;

// static WP wp_pool[NR_WP] = {};
// static WP *head = NULL, *free_ = NULL;

// void init_wp_pool() {
//   int i;
//   for (i = 0; i < NR_WP; i ++) {
//     wp_pool[i].NO = i;
//     wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
//   }

//   head = NULL;
//   free_ = wp_pool;
// }

/* TODO: Implement the functionality of watchpoint */

// in src/monitor/sdb/watchpoint.c

#include "sdb.h"
#include <isa.h>

#define NR_WP 32

// `WP` 结构体已经在 sdb.h 中定义
static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

// 从 free_ 链表中分配一个监视点
WP* new_wp() {
  if (free_ == NULL) {
    printf("Error: Watchpoint pool is full. Cannot create new watchpoint.\n");
    return NULL;
  }

  // 从 free_ 链表头部取出一个节点
  WP *new_node = free_;
  free_ = free_->next;

  // 将节点插入到 head 链表的头部
  new_node->next = head;
  head = new_node;

  return new_node;
}

// 根据序号释放一个监视点
void free_wp(int no) {
  if (head == NULL) {
    printf("Error: No watchpoints to delete.\n");
    return;
  }

  WP *wp = NULL;
  WP *prev = NULL;

  // 寻找要删除的节点
  if (head->NO == no) {
    wp = head;
    head = head->next;
  } else {
    prev = head;
    while (prev->next != NULL && prev->next->NO != no) {
      prev = prev->next;
    }
    if (prev->next != NULL) {
      wp = prev->next;
      prev->next = wp->next;
    }
  }

  if (wp == NULL) {
    printf("Error: Watchpoint %d not found.\n", no);
    return;
  }

  // 将节点归还到 free_ 链表的头部
  wp->expr_str[0] = '\0';
  wp->old_val = 0;
  wp->next = free_;
  free_ = wp;

  printf("Watchpoint %d deleted.\n", no);
}

// 打印所有正在使用的监视点
void display_watchpoints() {
  if (head == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  printf("Num\tType\t\tWhat\n");
  WP *p = head;
  while (p != NULL) {
    printf("%-d\twatchpoint\t%s\n", p->NO, p->expr_str);
    p = p->next;
  }
}

// 检查所有监视点的值是否发生变化
bool check_watchpoints() {
  bool triggered = false;
  WP *p = head;
  while (p != NULL) {
    bool success = true;
    uint32_t new_val = expr(p->expr_str, &success);

    if (success && new_val != p->old_val) {
      printf("\nWatchpoint %d: %s\n", p->NO, p->expr_str);
      printf("Old value = %u (0x%x)\n", p->old_val, p->old_val);
      printf("New value = %u (0x%x)\n", new_val, new_val);
      p->old_val = new_val;
      triggered = true;
    }
    p = p->next;
  }
  return triggered;
}

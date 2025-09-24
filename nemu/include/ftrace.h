#ifndef __MONITOR_FTRACE_H__
#define __MONITOR_FTRACE_H__

#include <common.h>

// 函数跟踪类型
enum { FTRACE_CALL, FTRACE_RET };

void init_ftrace(const char *elf_path);
bool ftrace_log(char *buf, size_t size, vaddr_t pc, vaddr_t dnpc, uint32_t inst);

#endif

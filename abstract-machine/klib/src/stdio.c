#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}
// 辅助函数：将一个整数转换为字符串
// base: 转换的进制 (例如 10 代表十进制)
// is_signed: 是否为有符号数
static char* itoa(int value, char *str, int base, bool is_signed) {
    char *ptr = str, *ptr1 = str, tmp_char;
    int tmp_value;
    bool negative = false;

    // 处理负数 (仅当is_signed为true)
    if (is_signed && value < 0) {
        negative = true;
        value = -value;
    }

    // 处理 value = 0 的特殊情况
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }

    // 从后向前生成数字字符
    while (value != 0) {
        tmp_value = value % base;
        *ptr++ = (tmp_value < 10) ? (tmp_value + '0') : (tmp_value - 10 + 'a');
        value /= base;
    }

    // 如果是负数, 添加负号
    if (negative) {
        *ptr++ = '-';
    }

    // 添加字符串结束符
    *ptr = '\0';

    // 反转字符串
    ptr--;
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return str;
}


int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  char *start = out;
  char temp_str[33]; // 用于整数转换的临时缓冲区

  va_start(ap, fmt);

  while (*fmt) {
    if (*fmt == '%') {
      fmt++; // 跳过 '%'
      switch (*fmt) {
        case 's': { // 格式化字符串
          char *s = va_arg(ap, char*);
          while (*s) {
            *out++ = *s++;
          }
          break;
        }
        case 'd': { // 格式化十进制整数
          int d = va_arg(ap, int);
          itoa(d, temp_str, 10, true);
          char *s = temp_str;
          while (*s) {
            *out++ = *s++;
          }
          break;
        }
        case '%': { // 处理 "%%"
          *out++ = '%';
          break;
        }
        default: { // 不支持的格式, 直接输出
          *out++ = '%';
          *out++ = *fmt;
          break;
        }
      }
    } else {
      *out++ = *fmt;
    }
    fmt++;
  }

  *out = '\0'; // 添加字符串结束符
  va_end(ap);

  return out - start; // 返回写入的字符数
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif

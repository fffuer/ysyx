#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t count = 0;
  while (*s != '\0') {
    count++;
    s++;
  }
  return count;
}

char *strcpy(char *dst, const char *src) {
  char *original_dst = dst;
  while (*src != '\0') {
    *dst = *src;
    dst++;
    src++;
  }
  *dst = '\0'; // 添加字符串结束符
  return original_dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *original_dst = dst;
  size_t i;

  // 1. 拷贝最多n个字符
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }

  // 2. 如果src长度小于n, 用'\0'填充剩余部分
  for (; i < n; i++) {
    dst[i] = '\0';
  }

  return original_dst;
}

char *strcat(char *dst, const char *src) {
  char *original_dst = dst;
  // 1. 移动到dst字符串的末尾
  while (*dst != '\0') {
    dst++;
  }

  // 2. 将src拷贝到dst的末尾
  while (*src != '\0') {
    *dst = *src;
    dst++;
    src++;
  }
  *dst = '\0'; // 添加新的字符串结束符

  return original_dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
    s1++;
    s2++;
  }
  // 在循环结束的地方比较字符差异
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if (n == 0) {
    return 0;
  }
  while (n-- > 1 && *s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *ptr = (unsigned char *)s;
  unsigned char value = (unsigned char)c;
  for (size_t i = 0; i < n; i++) {
    ptr[i] = value;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;

  if (d == s) {
    return dst;
  }

  // 判断内存区域是否重叠
  if (d < s || d >= s + n) {
    // 无重叠或源地址在后, 从前向后拷贝
    for (size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  } else {
    // 有重叠且目标地址在前, 从后向前拷贝
    for (size_t i = n; i > 0; i--) {
      d[i - 1] = s[i - 1];
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *d = (unsigned char *)out;
  const unsigned char *s = (const unsigned char *)in;
  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  if (n == 0) {
    return 0;
  }
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;

  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return p1[i] - p2[i];
    }
  }
  return 0;
}

#endif

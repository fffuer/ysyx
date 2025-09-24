#include <ftrace.h>
#include <elf.h>

// 用于存储符号表项
typedef struct {
  char name[64];
  paddr_t addr;
  unsigned int size;
} Elf_Symbol;

static Elf_Symbol func_symbols[1024];
static int nr_func_symbol = 0;
static int ftrace_indent = 0;

// 链接寄存器的编号 (ra = x1, t1 = x6 (备用, 但标准ABI中x5/t0更常见))
// 我们主要关注 x1 (ra)
#define LINK_REG_RA 1
#define LINK_REG_ALT 5 // x5 (t0) 是备用链接寄存器

static const char* find_func_name(paddr_t addr) {
  for (int i = 0; i < nr_func_symbol; i++) {
    if (addr >= func_symbols[i].addr && addr < func_symbols[i].addr + func_symbols[i].size) {
      return func_symbols[i].name;
    }
  }
  return "???";
}

void init_ftrace(const char *elf_path) {
  if (elf_path == NULL) { return; }
  // ... 此处省略之前已有的、正确的ELF解析代码 ...
  FILE *fp = fopen(elf_path, "rb");
  Assert(fp, "Cannot open ELF file '%s'", elf_path);

  Elf32_Ehdr elf_header;
  int ret = fread(&elf_header, sizeof(Elf32_Ehdr), 1, fp); assert(ret == 1);
  fseek(fp, elf_header.e_shoff, SEEK_SET);
  Elf32_Shdr section_headers[elf_header.e_shnum];
  ret = fread(section_headers, sizeof(Elf32_Shdr), elf_header.e_shnum, fp); assert(ret == elf_header.e_shnum);

  Elf32_Shdr *symtab = NULL, *strtab = NULL;
  for (int i = 0; i < elf_header.e_shnum; i++) {
    if (section_headers[i].sh_type == SHT_SYMTAB) symtab = &section_headers[i];
    if (section_headers[i].sh_type == SHT_STRTAB && i != elf_header.e_shstrndx) strtab = &section_headers[i];
  }
  assert(symtab != NULL && strtab != NULL);

  Elf32_Sym symbols[symtab->sh_size / sizeof(Elf32_Sym)];
  fseek(fp, symtab->sh_offset, SEEK_SET);
  ret = fread(symbols, sizeof(Elf32_Sym), symtab->sh_size / sizeof(Elf32_Sym), fp); assert(ret == symtab->sh_size / sizeof(Elf32_Sym));

  char str_buffer[strtab->sh_size];
  fseek(fp, strtab->sh_offset, SEEK_SET);
  ret = fread(str_buffer, 1, strtab->sh_size, fp); assert(ret == strtab->sh_size);

  for (int i = 0; i < symtab->sh_size / sizeof(Elf32_Sym); i++) {
    if (ELF32_ST_TYPE(symbols[i].st_info) == STT_FUNC) {
      strncpy(func_symbols[nr_func_symbol].name, &str_buffer[symbols[i].st_name], sizeof(func_symbols[0].name) - 1);
      func_symbols[nr_func_symbol].addr = symbols[i].st_value;
      func_symbols[nr_func_symbol].size = symbols[i].st_size;
      nr_func_symbol++;
      assert(nr_func_symbol < 1024);
    }
  }
  fclose(fp);
  Log("ftrace: Loaded %d function symbols from '%s'.", nr_func_symbol, elf_path);
}

// --- 采用新逻辑的日志准备函数 ---
bool ftrace_log(char *buf, size_t size, vaddr_t pc, vaddr_t dnpc, uint32_t inst) {
  if (nr_func_symbol == 0) return false;

  uint32_t opcode = inst & 0x7f;
  uint32_t rd = (inst >> 7) & 0x1f;
  uint32_t rs1 = (inst >> 15) & 0x1f;

  char indent_buf[128] = {0};

  // --- JALR 指令逻辑 ---
  if (opcode == 0b1100111) { // JALR
    bool rs1_is_link = (rs1 == LINK_REG_RA || rs1 == LINK_REG_ALT);
    bool rd_is_link = (rd == LINK_REG_RA || rd == LINK_REG_ALT);

    // 函数返回: 源是链接寄存器, 目标不是
    if (rs1_is_link && !rd_is_link) {
      ftrace_indent--;
      if (ftrace_indent < 0) ftrace_indent = 0; // 防止缩进变为负数
      for (int i = 0; i < ftrace_indent; ++i) strcat(indent_buf, "  ");
      snprintf(buf, size, "0x%08x: %sret  [%s -> %s]", pc, indent_buf, find_func_name(pc), find_func_name(dnpc));
      return true;
    }
    // 间接函数调用: 目标是链接寄存器, 源不是
    else if (rd_is_link && !rs1_is_link) {
      for (int i = 0; i < ftrace_indent; ++i) strcat(indent_buf, "  ");
      snprintf(buf, size, "0x%08x: %scall [indirect via r%d -> %s@0x%08x]", pc, indent_buf, rs1, find_func_name(dnpc), dnpc);
      ftrace_indent++;
      return true;
    }
  }
  // --- JAL 指令逻辑 ---
  else if (opcode == 0b1101111) { // JAL
    bool rd_is_link = (rd == LINK_REG_RA || rd == LINK_REG_ALT);

    // 函数调用: 目标是链接寄存器
    if (rd_is_link) {
      for (int i = 0; i < ftrace_indent; ++i) strcat(indent_buf, "  ");
      snprintf(buf, size, "0x%08x: %scall [%s@0x%08x]", pc, indent_buf, find_func_name(dnpc), dnpc);
      ftrace_indent++;
      return true;
    }
    // 普通跳转: 目标是 x0 (不打印)
    // 其他情况: 暂不处理
  }

  return false;
}

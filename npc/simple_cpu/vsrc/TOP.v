// `timescale 1ns / 1ps // (可选) 定义仿真时间单位和精度

// ===================================================================
// top.v: 顶层封装模块
// 职责:
// 1. 实例化处理器核心的三个主要单元: IFU, IDU, EXU。
// 2. 将这三个单元连接起来，形成一个完整的单周期处理器数据通路。
// 3. 提供与Verilator C++ Testbench交互的标准接口。
// ===================================================================

module top(
  // --- 基本信号 ---
  input         clk, // 时钟信号, 由C++ Testbench驱动
  input         rst_n, // 复位信号, 由C++ Testbench驱动

  // --- 指令存储器接口 ---
  output [31:0] i_pc,   // 输出: CPU当前要取指的PC地址
  input  [31:0] i_inst  // 输入: 外部存储器根据i_pc返回的指令
);

  // --- 模块间连线 (Wires) ---
  // 这些连线就像电路板上的导线，负责在模块间传递信号。

  // IDU -> EXU: 数据通路
  wire [31:0] rs1_data;      // 从寄存器堆读出的rs1的值
  wire [31:0] rs2_data;      // 从寄存器堆读出的rs2的值
  wire [31:0] imm;            // 解码出的立即数

  // IDU -> EXU: 控制信号
  wire        reg_write_en;   // 寄存器写使能信号
  wire [4:0]  rd_addr;        // 目标寄存器地址
  // (未来可以添加更多控制信号, e.g., alu_op, alu_src_sel)
  
  // EXU -> IDU: 写回数据通路
  wire [31:0] exu_result;     // EXU的计算结果，用于写回寄存器堆


  // --- 模块实例化 ---

  // 1. 取指单元 (Instruction Fetch Unit)
  // 它的职责是管理PC, 并将PC值输出用于取指。
  IFU u_ifu (
    .clk(clk),
    .rst_n(rst_n),
    // .branch_flag(),       // 为未来分支指令预留
    // .branch_target_addr(),// 为未来分支指令预留
    
    .mem_addr(i_pc)          // 将内部PC值连接到顶层输出 i_pc
  );

  // 2. 译码单元 (Instruction Decode Unit)
  // 它是处理器的大脑，负责指令解码、读寄存器堆、生成控制信号。
  IDU u_idu (
    .clk(clk),
    .rst_n(rst_n),
    .inst(i_inst),           // 从顶层接收指令输入
    .exu_result(exu_result), // 从EXU接收写回数据

    .rs1_data(rs1_data),   // 将读出的rs1值输出给EXU
    .rs2_data(rs2_data),   // 将读出的rs2值输出给EXU
    .imm(imm),               // 将生成的立即数输出给EXU
    .reg_write_en(reg_write_en)// 将控制信号输出给EXU
  );

  // 3. 执行单元 (Execution Unit)
  // 负责根据IDU的指令进行算术逻辑运算。
  EXU u_exu (
    .clk(clk),
    .rst_n(rst_n),
    .rs1_data(rs1_data),   // 从IDU接收rs1的值
    .rs2_data(rs2_data),   // 从IDU接收rs2的值
    .imm(imm),               // 从IDU接收立即数
    .exu_result(exu_result)  // 将计算结果输出给IDU用于写回
    // (未来实现lw/sw时需要连接数据存储器接口)
    // .dmem_addr(),
    // .dmem_wdata(),
    // .dmem_wen(),
    // .dmem_rdata()
  );

endmodule

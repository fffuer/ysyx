module EXU (
    input clk,
    input rst_n,
    //interface with IDU
    input [31:0] rs1_data,
    input [31:0] rs2_data,
    input [31:0] imm,
    //output
    output [31:0] exu_result
);
    assign exu_result = rs1_data + imm; // todo //
endmodule

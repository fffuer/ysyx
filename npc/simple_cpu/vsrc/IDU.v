module IDU (
    input          clk,
    input          rst_n,
    //interface with IFU
    input [31:0]   inst,
    //interface with EXU
    input [31:0]   exu_result,
    output         reg_write_en,
    output reg [31:0]  imm,
    output [31:0]  rs1_data,
    output [31:0]  rs2_data
);
    //inst fields
    wire [6:0] opcode = inst[6:0];
    wire [4:0] rd     = inst[11:7];
    wire [4:0] rs1    = inst[19:15];
    wire [4:0] rs2    = inst[24:20];
    wire [2:0] funct3 = inst[14:12];
    wire [6:0] funct7 = inst[31:25];
    //control
    assign reg_write_en = (opcode == 7'b0010011); // R-type
    //imm generation
    always @(*) begin
        case (opcode)
            7'b0010011: // I-type (addi, lw, jalr)
                imm = {{20{inst[31]}}, inst[31:20]};
                // ... 未来在这里扩展其他指令的立即数生成
            default:
                imm = 32'b0;
            endcase
    end
    //registers
    reg [31:0] regs [0:31];
    //output
    assign rs1_data = (rs1 == 0) ? 32'b0 : regs[rs1];
    assign rs2_data = (rs2 == 0) ? 32'b0 : regs[rs2];
    //write back
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            // reset all registers to 0
            integer i;
            for (i = 0; i < 32; i = i + 1) begin
                regs[i] <= 32'b0;
            end
        end
        else if (reg_write_en && rd != 0) begin
            regs[rd] <= exu_result;
        end
    end
endmodule

module IFU (
    input          clk,
    input          rst_n,
    
    output  [31:0] mem_addr,
    output         inst
);
    reg     [31:0] pc;
    wire    [31:0] pc_next;
    wire    [31:0] pc_plus4;
    //pc + 4
    assign pc_plus4 = pc + 4;
    //pc_next
    assign pc_next = pc_plus4; // todo //
    //pc updata
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pc <= 32'h80000000;
        end
        else begin
            pc <= pc_next;
        end
    end
    //output
    assign mem_addr = pc;
    //the "inst" is connected by the top-level, and here it is merely defining the ports.
endmodule

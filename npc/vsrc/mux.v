module mux (
    input [1:0] Y,
    input [1:0] X0,
    input [1:0] X1,
    input [1:0] X2,
    input [1:0] X3,
    output [1:0] F
);
    assign F =  (Y == 2'b00) ? X0 :
                (Y == 2'b01) ? X1 :
                (Y == 2'b10) ? X2 :
                (Y == 2'b11) ? X3 : 2'b00; // Default case, if needed
    // Uncomment the following line if you want to use a case statement instead
    // always @(*) case (Y) 2'b00: F = X0; 2'b01: F = X1; 2'b10: F = X2; 2'b11: F = X3; endcase
endmodule
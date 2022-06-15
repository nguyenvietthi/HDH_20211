module State(
    input clk1,
    input rst,
    input timeout30,
    input timeout25,
    output reg LR1,
    output reg LR2,
    output reg LG1,
    output reg LG2,
    output reg LY1,
    output reg LY2,
    output reg eLED01,
    output reg eLED23
    );
localparam [1:0] 
    NS_go = 2'b00,
    EW_go = 2'b01,
    WaitA  = 2'b10,
    WaitB   =2'b11;
reg[1:0] state_reg, state_next;

always@(posedge clk1, posedge rst)
begin
    if(rst)
        begin
        state_reg <= NS_go;
        end
    else
        begin
        state_reg <= state_next;
        end
end

always@(state_reg,timeout25, timeout30)
begin
    state_next = state_reg;
    LR1=0;LR2=0;LG1=0;LG2=0;LY1=0;LY2=0;    
    case(state_reg)
        NS_go: begin
            LR1=1;LR2=0;LG1=0;LG2=1;LY1=0;LY2=0;
            if(timeout25) begin
                state_next = WaitA;
                end
            else begin
                state_next = NS_go;
                end
        end
        EW_go: begin
            LR1=0;LR2=1;LG1=1;LG2=0;LY1=0;LY2=0;
            if(timeout25) begin
                state_next = WaitB;
            end
            else begin
                state_next = EW_go;
            end
        end
        WaitA: begin
            LR1=0;LR2=0;LG1=0;LG2=0;LY1=1;LY2=1;
            if(timeout30) begin
                state_next = EW_go;
            end
            else begin
                state_next = WaitA;
            end
        end
        WaitB: begin
            LR1=0;LR2=0;LG1=0;LG2=0;LY1=1;LY2=1;
            if(timeout30) begin
                state_next = NS_go;
            end
            else begin
                state_next = WaitB;
            end
        end
    endcase
end
always@(*)begin
    if(rst) begin
        eLED01 = 1'b0;
        eLED23 = 1'b0;
        end
    else begin
        eLED01 = 1'b1;
        eLED23 = 1'b1;
    end

end

endmodule
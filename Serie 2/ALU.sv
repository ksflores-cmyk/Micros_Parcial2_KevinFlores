// ==========================================================
// ALU 10-bit | 6 operaciones | I/O segun requerimiento
// A, B : 10 bits sin signo
// CTRL : 3 bits (decimal 1..6)
//        1=ADD, 2=SUB, 3=AND, 4=OR, 5=SHL, 6=SHR
// Salidas: Y[9:0], ZERO, NEGATIVE, OVERFLOW, CARRY
// - CARRY solo aplica a ADD (en otras ops se entrega 0).
// - OVERFLOW calculado estilo dos-complementos para ADD/SUB.
// - NEGATIVE = bit más significativo de Y (Y[9]).
// ==========================================================
`timescale 1ns/1ps

module alu10_core (
  input  logic [9:0] A,
  input  logic [9:0] B,
  input  logic [2:0] CTRL,   // 1..6
  output logic [9:0] Y,
  output logic       ZERO,
  output logic       NEGATIVE,
  output logic       OVERFLOW,
  output logic       CARRY
);
  localparam int N   = 10;
  localparam int SHW = (N <= 1) ? 1 : $clog2(N);

  // --- Operaciones pre-computadas
  logic [N:0] add_ext, sub_ext;
  logic [N-1:0] add_y, sub_y, and_y, or_y, shl_y, shr_y;
  logic add_carry, add_ovf, sub_ovf;

  // Suma (unsigned + carry) y overflow (dos-complementos)
  assign add_ext   = {1'b0, A} + {1'b0, B};
  assign add_y     = add_ext[N-1:0];
  assign add_carry = add_ext[N];
  assign add_ovf   = (A[N-1] == B[N-1]) && (add_y[N-1] != A[N-1]);

  // Resta (A - B) | overflow (dos-complementos)
  assign sub_ext = {1'b0, A} - {1'b0, B};
  assign sub_y   = sub_ext[N-1:0];
  assign sub_ovf = (A[N-1] ^ B[N-1]) && (sub_y[N-1] != A[N-1]); // (a^b)&(res^a)

  // Lógicas
  assign and_y = A & B;
  assign or_y  = A | B;

  // Shifts lógicos; cantidad desde LSBs de B
  wire [SHW-1:0] shamt = B[SHW-1:0];
  assign shl_y = A << shamt;
  assign shr_y = A >> shamt;

  // --- Selección por CTRL (1..6)
  always_comb begin
    Y        = '0;
    CARRY    = 1'b0;
    OVERFLOW = 1'b0;

    unique case (CTRL)
      3'd1: begin // ADD
        Y        = add_y;
        CARRY    = add_carry;
        OVERFLOW = add_ovf;
      end
      3'd2: begin // SUB
        Y        = sub_y;
        CARRY    = 1'b0;      // carry no aplica; si quisieras "no borrow", usa ~sub_ext[N]
        OVERFLOW = sub_ovf;
      end
      3'd3: Y = and_y;        // AND
      3'd4: Y = or_y;         // OR
      3'd5: Y = shl_y;        // SHL
      3'd6: Y = shr_y;        // SHR
      default: begin
        Y        = '0;        // CTRL fuera de rango
        CARRY    = 1'b0;
        OVERFLOW = 1'b0;
      end
    endcase
  end

  assign ZERO     = (Y == '0);
  assign NEGATIVE = Y[N-1];   // Si interpretas Y como dos-complementos

endmodule


// ----------------------------------------------------------
// TOP sintetizable (usa este como "Top" para Synthesis/Implementation)
// ----------------------------------------------------------
module top_alu10_synth (
  input  logic [9:0] A,
  input  logic [9:0] B,
  input  logic [2:0] CTRL,      // 1..6
  output logic [9:0] Y,
  output logic       ZERO,
  output logic       NEGATIVE,
  output logic       OVERFLOW,
  output logic       CARRY
);
  alu10_core u_core (
    .A(A), .B(B), .CTRL(CTRL),
    .Y(Y), .ZERO(ZERO), .NEGATIVE(NEGATIVE),
    .OVERFLOW(OVERFLOW), .CARRY(CARRY)
  );
endmodule


// ----------------------------------------------------------
// TESTBENCH (solo simulación) - Selección decimal 1..6
// En Vivado marca "tb_alu10" como top para Run Simulation
// ----------------------------------------------------------
module tb_alu10;
  logic [9:0] A, B;
  logic [2:0] CTRL;
  logic [9:0] Y;
  logic ZERO, NEGATIVE, OVERFLOW, CARRY;

  alu10_core dut (
    .A(A), .B(B), .CTRL(CTRL),
    .Y(Y), .ZERO(ZERO), .NEGATIVE(NEGATIVE),
    .OVERFLOW(OVERFLOW), .CARRY(CARRY)
  );

  task automatic show(string tag="");
    #1;
    $display("[%0t] %s CTRL=%0d  A=%0d (0x%0h)  B=%0d (0x%0h)  -> Y=%0d (0x%0h)  Z=%0b N=%0b OVF=%0b C=%0b",
             $time, tag, CTRL, A, A, B, B, Y, Y, ZERO, NEGATIVE, OVERFLOW, CARRY);
  endtask

  initial begin
    // ADD (1)
    A=10'd100; B=10'd23; CTRL=3'd1; show("ADD 100+23");
    // overflow/carry demo
    A=10'd1023; B=10'd2; CTRL=3'd1; show("ADD 1023+2");

    // SUB (2)  (asumimos A >= B)
    A=10'd700; B=10'd399; CTRL=3'd2; show("SUB 700-399");

    // AND (3)
    A=10'b1010101010; B=10'b1100110011; CTRL=3'd3; show("AND");

    // OR (4)
    A=10'b0011001100; B=10'b0101010101; CTRL=3'd4; show("OR");

    // SHL (5)  shift por B[3:0] (porque $clog2(10)=4)
    A=10'd11;  B=10'd3;   CTRL=3'd5; show("SHL A<<3");

    // SHR (6)
    A=10'd300; B=10'd2;   CTRL=3'd6; show("SHR A>>2");

    // CTRL inválido (0)
    A=10'd5; B=10'd1; CTRL=3'd0; show("CTRL=0 invalido");

    $finish;
  end
endmodule

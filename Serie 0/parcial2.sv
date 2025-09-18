`timescale 1ns / 1ps


// =====================================================================
//  FSM para cerradura con PIN de 4 bits
//  - Yo controlo la entrada X 
//  - El estado S = {A,B,C,D} vive en 4 flip-flops tipo D
//  - La lógica S' (próximo estado) 
// =====================================================================

module parcial2(

  input  logic clk,            // reloj
  input  logic rst_n,          // reset asíncrono activo en 0
  input  logic x,              // mi entrada de la tabla (0/1)
  output logic [3:0] state,    // {A,B,C,D} = Q de los FF (mi estado actual)
  output logic DA, DB, DC, DD, // por si quiero cada bit por separado
  output logic unlocked        // 1 cuando estoy en el estado de "abierto"
);

  // -------------------------------------------------------------------
  //  Mis constantes de estados especiales 
  // -------------------------------------------------------------------
  localparam logic [3:0] RESET_STATE   = 4'b0000; // aquí inicio
  localparam logic [3:0] UNLOCK_STATE  = 4'b0100; // ESTE es el estado "abierto"
  localparam logic [3:0] LOCKOUT_STATE = 4'b1111; // 3 errores -> bloqueo


  // Registrito para el próximo estado (S')
  logic [3:0] next_state;

  // -------------------------------------------------------------------
  //  BLOQUE DE ESTADO (S): FF tipo D con reset asíncrono
  //  Aquí está la retroalimentación formal: S se alimenta con S'
  // -------------------------------------------------------------------
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n)
      state <= RESET_STATE;     // me voy al inicio (0000)
    else
      state <= next_state;      // en flanco de reloj, cargo S'
  end

  // -------------------------------------------------------------------
  //  LÓGICA COMBINACIONAL DE PRÓXIMO ESTADO (S')
  //  1:1 con mi tabla de verdad (X, A B C D -> A' B' C' D').
  //  Hago case sobre {x, state} para que quede clarito y directo.
  // -------------------------------------------------------------------
  always_comb begin
    next_state = state;  // default: me quedo igual por si no matchea nada

    unique case ({x, state})

      // =======================
      //   Ramas con X = 0
      // =======================
      5'b0_0000: next_state = 4'b0101;
      5'b0_0001: next_state = 4'b0101;
      5'b0_0010: next_state = 4'b0101;
      5'b0_0011: next_state = 4'b0101;
      5'b0_0100: next_state = 4'b0100; // abierto se mantiene (cero o uno)
      5'b0_0101: next_state = 4'b1001;
      5'b0_0110: next_state = 4'b1001;
      5'b0_0111: next_state = 4'b1001;
      5'b0_1000: next_state = 4'b1001;
      5'b0_1001: next_state = 4'b1101;
      5'b0_1010: next_state = 4'b1101;
      5'b0_1011: next_state = 4'b1101;
      5'b0_1100: next_state = 4'b1101;
      5'b0_1101: next_state = 4'b1110;
      5'b0_1110: next_state = 4'b1111; // lockout
      5'b0_1111: next_state = 4'b0000; // desde lockout me regresa a inicio

      // =======================
      //   Ramas con X = 1
      // =======================
      5'b1_0000: next_state = 4'b0001;
      5'b1_0001: next_state = 4'b0010;
      5'b1_0010: next_state = 4'b0011;
      5'b1_0011: next_state = 4'b0100; // 4 unos seguidos -> abierto (0100)
      5'b1_0100: next_state = 4'b0100; // abierto se queda abierto
      5'b1_0101: next_state = 4'b0110;
      5'b1_0110: next_state = 4'b0111;
      5'b1_0111: next_state = 4'b1000;
      5'b1_1000: next_state = 4'b0100; // transición según mi tabla
      5'b1_1001: next_state = 4'b1010;
      5'b1_1010: next_state = 4'b1011;
      5'b1_1011: next_state = 4'b1100;
      5'b1_1100: next_state = 4'b0100; // vuelve al abierto según tabla
      5'b1_1101: next_state = 4'b1110;
      5'b1_1110: next_state = 4'b1111; // lockout
      5'b1_1111: next_state = 4'b0000; // desde lockout, regreso a inicio

      default:   next_state = RESET_STATE; // si algo raro, me curo regresando
    endcase
  end

  // -------------------------------------------------------------------
  //  SALIDAS (dependen solo de S)
  // -------------------------------------------------------------------
  assign {DA, DB, DC, DD} = state;               // igualito a mis Q
  assign unlocked          = (state == UNLOCK_STATE); // 1 cuando S = 0100

endmodule
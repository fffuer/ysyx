#include <stdlib.h>
#include <iostream>
#include <assert.h>

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vmux.h"

VerilatedContext* contextp = NULL;
VerilatedVcdC* tfp = NULL;

static Vmux* top;

void step_and_dump_wave(){
    top->eval();
    contextp->timeInc(1);
    tfp->dump(contextp->time());
}
void sim_init(){
    contextp = new VerilatedContext;
    tfp = new VerilatedVcdC;
    top = new Vmux;
    contextp->traceEverOn(true);
    top->trace(tfp, 5);
    tfp->open("waveform.vcd");
}

void sim_exit(){
    step_and_dump_wave();
    tfp->close();
    delete top;
    delete tfp;
    delete contextp;
    exit(EXIT_SUCCESS);
}

int main() {
    sim_init();

    while (contextp->time() < 200) { // Simulate for 200 time units
        top->Y = rand() & 3; // Random input a
        top->X0 = rand() & 3; // Random input b
        top->X1 = rand() & 3;
        top->X2 = rand() & 3;
        top->X3 = rand() & 3; // Random select signal
        step_and_dump_wave();
        assert(top->F == (  (top->Y == 0) ? top->X0 : 
                            (top->Y == 1) ? top->X1 :
                            (top->Y == 2) ? top->X2 :
                            (top->Y == 3) ? top->X3 : 0)); // Check mux output
    }
    sim_exit();
}

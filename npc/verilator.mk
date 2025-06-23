default: compile
TOP ?= mux

obj_dir/V$(TOP).mk:
	verilator -Wall vsrc/$(TOP).v csrc/mux_ver_main.cpp --cc --exe --trace --top-module $(TOP)

compile: obj_dir/V$(TOP).mk
	make -C obj_dir -f V$(TOP).mk V$(TOP)  

run:compile
	obj_dir/V$(TOP)
	
.PHONY: default compile clean run

clean:
	rm -rf obj_dir *.vcd
wave:
	gtkwave waveform.vcd

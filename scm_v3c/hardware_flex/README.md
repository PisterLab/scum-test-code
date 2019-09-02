# SCM3C Hardware Flex
Code for checking functionality of various pieces of hardware on the chip. This can involve software returning various values and/or require hardware probing (i.e. via oscilloscope or multimeter).

## User Instructions
Each function in `hardware_flex.c` comes in groups of 2 or more. In other words, for every `func_init()`, there is a corresponding `func_main()`. The former should be added to `../scm3C_hardware_interface.c/initialize_mote()`, and the latter should be added to `../main.c/main()`. For any function groupings larger than 3, please see the functions' specific documentation.

1. Add your desired `func_init()` to `../scm3C_hardware_interface.c/initialize_mote()` *above* the lines which load and write the software ASC to the hardware analog scan chain.
2. Add your desired `func_main()` to `../main.c/main()`.
3. Check the documentation of the functions from (1) and (2) to see if any other setup is required.
4. Compile your C code with Keil uVision. Be sure you have the necessary `#include` statements at the top of your C files.
5. Program SCM (see `../bootload.py`) and probe and/or read from the serial port as necessary.

## Developer Instructions
* *Abide by the function suffix convention*
* All functions should document:
	- Inputs: Name and a description of each input's purpose and/or correlation in hardware. For example, X is the coding to adjust the LDO voltage where 0 = highest voltage setting around 1.2V and all 1 = lowest voltage setting around 0.8V.
	- Outputs: Plain text description of the return value and any relevant conditions (e.g. tests X condition. 0 if X, 1 if Y)
	- Notes
		* Mention associated functions (e.g. `_init`, `_main`) and their order of operation (the init should be run before the main, etc.)
		* If a function has not been tested, indicate so.
		* Any known bugs or necessary preconditions (e.g. UART communication is necessary for this to work)
* 

### Function Suffixes
* `_init`: Any code to be run in `../scm3C_hardware_interface.c/initialize_mote()`.
* `_main`: Any code to be run in `../main.c/main()`.

* If two functions need to be run in the same function, e.g. `main.c`, call one `<funcName>_1_main()` and the other `<funcName>_2_main()`.
* If any code needs to be run outside of any of those locations, add to (don't modify or delete from) the list above.
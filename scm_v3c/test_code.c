#include <stdio.h>
#include <stdlib.h>
#include "Memory_Map.h"
#include "scm3_hardware_interface.h"
#include "scm3C_hardware_interface.h"

void test_get_asc_bit(void) {
	/*
	Functions tested:
		get_asc_bit()
	*/
	unsigned int ASC_bit;

	unsigned int i;
	for(i=0; i<1200; i++) {
		set_asc_bit(i);
		ASC_bit = get_asc_bit(i);
		if (ASC_bit != 1) {
			printf("Incorrect at index %d", i);
		}

		clear_asc_bit(i);
		ASC_bit = get_asc_bit(i);
		if (ASC_bit != 0) {
			printf("Incorrect at index %d", i);
		}
	}
}

void test_get_GPIO_enables(void) {
	/*
	Functions tested:
		get_GPI_enables()
		get_GPO_enables()
	*/
	unsigned int gpi_mask;
	unsigned int gpo_mask;

	unsigned int i;

	for(i=0; i<0xFFFF; i++) {
		GPI_enables(i);
		GPO_enables(~i);

		gpi_mask = get_GPI_enables();
		gpo_mask = get_GPO_enables();

		if (gpi_mask != i) {
			printf("GPI should be %X, got %X", i, gpi_mask);
		}

		if (gpo_mask != ~i) {
			printf("GPO should be %X, got %X", ~i, gpo_mask);
		}
	}

	return;

}

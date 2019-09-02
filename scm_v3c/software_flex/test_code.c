#include <stdio.h>
#include <stdlib.h>
#include "../Memory_Map.h"
#include "../scm3_hardware_interface.h"
#include "../scm3C_hardware_interface.h"

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
			printf("Incorrect at index %d \n", i);
		}
		else {
			printf("ok %d \n", i);
		}

		clear_asc_bit(i);
		ASC_bit = get_asc_bit(i);
		if (ASC_bit != 0) {
			printf("Incorrect at index %d \n", i);
		}
		else {
			printf("ok %d \n", i);
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
		GPO_enables(i);

		gpi_mask = get_GPI_enables();
		gpo_mask = get_GPO_enables();

		if (gpi_mask != i) {
			printf("GPI should be %X, got %X\n", i, gpi_mask);
		}
		else {
			printf("ok %X \n", gpi_mask);
		}

		if (gpo_mask != i) {
			printf("GPO should be %X, got %X\n", i, gpo_mask);
		}
		else {
			printf("ok %X \n", gpo_mask);
		}
	}

	return;
}

void test_get_GPO_control(void) {
	unsigned char gpo_bank;
	unsigned char i;
	for(i=0; i<16; i++) {
		// GPO control row 1
		GPO_control(i,0,0,0);
		gpo_bank = get_GPO_control(0);
		if (gpo_bank != i) {
			printf("GPO row 0 should be %X, got %X\n", (i&0xFF), (gpo_bank&0xFFFF));
		}
		else {
			printf("ok %X\n", (gpo_bank&0xFFFF));
		}

		// GPO control row 2
		GPO_control(0,i,0,0);
		gpo_bank = get_GPO_control(1);
		if (gpo_bank != i) {
			printf("GPO row 1 should be %X, got %X\n", (i&0xFF), (gpo_bank&0xFFFF));
		}
		else {
			printf("ok %X\n", (gpo_bank&0xFFFF));
		}

		// GPO control row 3
		GPO_control(0,0,i,0);
		gpo_bank = get_GPO_control(2);
		if (gpo_bank != i) {
			printf("GPO row 2 should be %X, got %X\n", (i&0xFF), (gpo_bank&0xFFFF));
		}
		else {
			printf("ok %X\n", (gpo_bank&0xFFFF));
		}

		// GPO control row 4
		GPO_control(0,0,0,i);
		gpo_bank = get_GPO_control(3);
		if (gpo_bank != i) {
			printf("GPO row 3 should be %X, got %X\n", (i&0xFF), (gpo_bank&0xFFFF));
		}
		else {
			printf("ok %X\n", (gpo_bank&0xFFFF));
		}
	}
}

void test_get_GPI_control(void) {
	unsigned char gpi_bank;
	unsigned char i;
	for(i=0; i<4; i++) {
		// GPI control row 1
		GPI_control(i,0,0,0);
		gpi_bank = get_GPI_control(0);
		if (gpi_bank != i) {
			printf("GPI row 0 should be %X, got %X\n", (i&0xFF), (gpi_bank&0xFF));
		}
		else {
			printf("ok %X\n",(gpi_bank&0xFF));
		}
		// GPI control row 2
		GPI_control(0,i,0,0);
		gpi_bank = get_GPI_control(1);

		if (gpi_bank != i) {
			printf("GPI row 1 should be %X, got %X\n", (i&0xFF), (gpi_bank&0xFF));
		}
		else {
			printf("ok %X\n",(gpi_bank&0xFF));
		}
		// GPI control row 2
		GPI_control(0,0,i,0);
		gpi_bank = get_GPI_control(2);

		if (gpi_bank != i) {
			printf("GPI row 2 should be %X, got %X\n", (i&0xFF), (gpi_bank&0xFF));
		}
		else {
			printf("ok %X\n",(gpi_bank&0xFF));
		}
		// GPI control row 3
		GPI_control(0,0,0,i);
		gpi_bank = get_GPI_control(3);
		if (gpi_bank != i) {
			printf("GPI row 3 should be %X, got %X\n", (i&0xFF), (gpi_bank&0xFF));
		}
		else {
			printf("ok %X\n",(gpi_bank&0xFF));
		}
	}
}

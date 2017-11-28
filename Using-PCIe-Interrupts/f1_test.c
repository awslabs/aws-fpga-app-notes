/*
 * Copyright 2017 Amazon.com, Inc. or its affiliates.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <fpga_pci.h>
#include <fpga_mgmt.h>
#include <utils/lcd.h>

#include <time.h>

// XDMA Target IDs
#define H2C_TGT 0x0
#define C2H_TGT 0x1
#define IRQ_TGT 0x2
#define CFG_TGT 0x3
#define H2C_SGDMA_TGT 0x4
#define C2H_SGDMA_TGT 0x5
#define SGDMA_CMN_TGT 0x6
#define MSIX_TGT 0x8

// XDMA H2C Channel Register Space
#define H2C_ID           0x00
#define H2C_CTRL0        0x04
#define H2C_CTRL1        0x08
#define H2C_CTRL2        0x0c
#define H2C_STAT0        0x40
#define H2C_STAT1        0x44
#define H2C_CMP_DESC_CNT 0x48
#define H2C_ALGN         0x4c
#define H2C_POLL_ADDR_LO 0x88
#define H2C_POLL_ADDR_HI 0x8c
#define H2C_INT_MSK0     0x90
#define H2C_INT_MSK1     0x94
#define H2C_INT_MSK2     0x98
#define H2C_PMON_CTRL    0xc0
#define H2C_PCYC_CNT0    0xc4
#define H2C_PCYC_CNT1    0xc4
#define H2C_PDAT_CNT0    0xcc
#define H2C_PDAT_CNT1    0xd0

// XDMA C2H Channel Register Space
#define C2H_ID           0x00
#define C2H_CTRL0        0x04
#define C2H_CTRL1        0x08
#define C2H_CTRL2        0x0c
#define C2H_STAT0        0x40
#define C2H_STAT1        0x44
#define C2H_CMP_DESC_CNT 0x48
#define C2H_ALGN         0x4c
#define C2H_POLL_ADDR_LO 0x88
#define C2H_POLL_ADDR_HI 0x8c
#define C2H_INT_MSK0     0x90
#define C2H_INT_MSK1     0x94
#define C2H_INT_MSK2     0x98
#define C2H_PMON_CTRL    0xc0
#define C2H_PCYC_CNT0    0xc4
#define C2H_PCYC_CNT1    0xc4
#define C2H_PDAT_CNT0    0xcc
#define C2H_PDAT_CNT1    0xd0

int verbose = 0;

#define NUM_OF_USER_INTS 16

uint32_t dma_reg_addr(uint32_t target, uint32_t channel, uint32_t offset) {
  return ((target << 12) | (channel << 8) | offset);
}

/* Subtract timespec t2 from t1 */
static void compute_delta(struct timespec *t1, const struct timespec *t2)
{

  t1->tv_sec -= t2->tv_sec;
  t1->tv_nsec -= t2->tv_nsec;

  if (t1->tv_nsec >= 1000000000l) {
    t1->tv_sec++;
    t1->tv_nsec -= 1000000000l;
  } else if (t1->tv_nsec < 0l) {
    t1->tv_sec--;
    t1->tv_nsec += 1000000000l;
  }

}

int interrupt_example(int slot_id, int interrupt_number){
    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
    pci_bar_handle_t dma_bar_handle = PCI_BAR_HANDLE_INIT;
    pci_bar_handle_t ddr_bar_handle = PCI_BAR_HANDLE_INIT;
    uint32_t read_data;
    uint32_t last_read_data;
    int rc = 0;
    int pf_id = 0;
    int bar_id = 0;
    int fpga_attach_flags = 0;
    uint32_t interrupt_reg_offset = 0xd00;
    int i;
    struct timespec ts_start, ts_end;
    
    printf("Starting MSI-X Interrupt test \n");
    rc = fpga_pci_attach(slot_id, pf_id, bar_id, fpga_attach_flags, &pci_bar_handle);
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);

    rc = fpga_pci_attach(slot_id, pf_id, 2, fpga_attach_flags, &dma_bar_handle);
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);
    
    rc = fpga_pci_attach(slot_id, pf_id, 4, fpga_attach_flags, &ddr_bar_handle);
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);
    
    
    rc = fpga_pci_peek(ddr_bar_handle, 0, &last_read_data);
    printf("Peek last_read_data: %0x\n", last_read_data);
    
    rc = fpga_pci_peek(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x000), &read_data);
    printf("IRQ Block Indentifier read_data: %0x\n", read_data);
    
    rc = fpga_pci_peek(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x004), &read_data);
    printf("IRQ Block User Interrupt Enable Mask read_data: %0x, Addr: %0x\n", read_data, dma_reg_addr(IRQ_TGT, 0, 0x004));
    
    rc = fpga_pci_peek(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x004), &read_data);
    printf("IRQ Block User Interrupt Enable Mask read_data: %0x\n", read_data);
    
    // pick different vector for each interrupt
    rc = fpga_pci_peek(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x080), &read_data);
    printf("IRQ Block User Vector Number read_data: %0x, Addr: %0x\n", read_data, dma_reg_addr(IRQ_TGT, 0, 0x080));
    
    // point each user interrupt to a different vector
    rc  = fpga_pci_poke(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x080), 0x03020100);
    rc |= fpga_pci_poke(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x084), 0x07060504);
    rc |= fpga_pci_poke(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x088), 0x0b0a0908);
    rc |= fpga_pci_poke(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x08c), 0x0f0e0d0c);
    fail_on(rc, out, "Unable to write to the fpga !");      
    
    // read the first one back for grins
    rc = fpga_pci_peek(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x080), &read_data);
    printf("IRQ Block User Vector Number read_data: %0x\n", read_data);
    
    // Enable Interrupt Mask (This step seems a little backwards.)    
    rc = fpga_pci_poke(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x004), 0xffff);
    printf("IRQ Block User Interrupt Enable Mask read_data: %0x\n", read_data);
        
    printf("Triggering each of the MSI-X interrupts...\n");
    for(i=0; i<NUM_OF_USER_INTS; i++) {
      rc = fpga_pci_poke(pci_bar_handle, interrupt_reg_offset , 1 << i);
      fail_on(rc, out, "Unable to write to the fpga !");      
    }
    
    /* grab start time */
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
    
        rc = fpga_pci_peek(pci_bar_handle, interrupt_reg_offset, &read_data);
	printf("CL interrupt status read_data: %08x\n", read_data);
	

	i = 0;
	do {
	  rc = fpga_pci_peek(ddr_bar_handle, 0, &read_data);
	  if (verbose)
	    printf("Peek read_data: %0x\n", read_data);
	} while ((i<100) & (read_data == last_read_data));
	    
	rc = fpga_pci_peek(ddr_bar_handle, 0, &read_data);
	printf("Peek read_data: %0x\n", read_data);

    /* grab end time */
    rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);


    compute_delta(&ts_end, &ts_start);
    printf("time %ld.%09ld seconds for ISR\n",
    ts_end.tv_sec, ts_end.tv_nsec);
    
    // In this CL, a successful interrupt is indicated by the CL setting bit <interrupt_number + 16>
    // of the interrupt register. Here we check that bit is set and write 1 to it to clear.
    rc = fpga_pci_peek(pci_bar_handle, interrupt_reg_offset, &read_data);
    printf("CL read_data: %08x\n", read_data);
    fail_on(rc, out, "Unable to read from the fpga !");

    // clear acknowledge bits
    rc = fpga_pci_poke(pci_bar_handle, interrupt_reg_offset , read_data );
    fail_on(rc, out, "Unable to write to the fpga !");
    
    // make sure bits were cleared
    rc = fpga_pci_peek(pci_bar_handle, interrupt_reg_offset, &read_data);
    printf("CL read_data: %08x\n", read_data);
    fail_on(rc, out, "Unable to read from the fpga !");

out:
    printf("leaving\n");
    return rc;
}

int main(int argc, char **argv) {
    int rc;
    int slot_id;
    int interrupt_number;

    slot_id = 0;
    interrupt_number = 0;
    
    rc = interrupt_example(slot_id, interrupt_number);
    fail_on(rc, out, "Interrupt example failed");
    
out:
    return rc;
}


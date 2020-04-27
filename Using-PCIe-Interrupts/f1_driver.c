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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>

#include <linux/slab.h>
#include <linux/interrupt.h>


MODULE_AUTHOR("Winefred Washington <winefred@amazon.com>");
MODULE_DESCRIPTION("AWS F1 CL_DRAM_DMA Interrupt Driver Example");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

static int slot = 0x0f;
module_param(slot, int, 0);
MODULE_PARM_DESC(slot, "The Slot Index of the F1 Card");

#define DOMAIN 0
#define BUS 0
#define FUNCTION 0
#define DDR_BAR  4
#define XDMA_BAR 2
#define OCL_BAR  0

#define NUM_OF_USER_INTS 16

struct msix_entry f1_ints[] = {
  {.vector = 0, .entry = 0},
  {.vector = 0, .entry = 1},
  {.vector = 0, .entry = 2},
  {.vector = 0, .entry = 3},
  {.vector = 0, .entry = 4},
  {.vector = 0, .entry = 5},
  {.vector = 0, .entry = 6},
  {.vector = 0, .entry = 7},
  {.vector = 0, .entry = 8},
  {.vector = 0, .entry = 9},
  {.vector = 0, .entry = 10},
  {.vector = 0, .entry = 11},
  {.vector = 0, .entry = 12},
  {.vector = 0, .entry = 13},
  {.vector = 0, .entry = 14},
  {.vector = 0, .entry = 15}
};
  
int f1_major = 0;

struct pci_dev *f1_dev;
int *f1_dev_id[NUM_OF_USER_INTS];
int *f1_dev_id_1;

void __iomem *ocl_base;
void __iomem *ddr_base;
void __iomem *xdma_base;

static DEFINE_SPINLOCK(f1_isr_lock);

static irqreturn_t f1_isr(int a, void *dev_id) {
  unsigned long flags;

  // make sure we do not interrupt another ISR
  spin_lock_irqsave(&f1_isr_lock, flags);
  
  // write a message to the kernel log file and increment memory
  printk(KERN_NOTICE "f1_isr: %d\n", *(int *)dev_id);
  *(unsigned int *)ddr_base += 1;

  spin_unlock_irqrestore(&f1_isr_lock, flags);
  
  // notify the kernel that the interrupt was handled.
  return IRQ_HANDLED;
}
  

static int __init f1_init(void) {
  int result;
  int i;
  
  printk(KERN_NOTICE "Installing f1 module\n");

  f1_dev = pci_get_domain_bus_and_slot(DOMAIN, BUS, PCI_DEVFN(slot,FUNCTION));
  if (f1_dev == NULL) {
    printk(KERN_ALERT "f1_driver: Unable to locate PCI card.\n");
    return -1;
  }

  printk(KERN_INFO "vendor: %x, device: %x\n", f1_dev->vendor, f1_dev->device);

  result = pci_enable_device(f1_dev);
  printk(KERN_INFO "Enable result: %x\n", result);

  result = pci_request_region(f1_dev, DDR_BAR, "DDR Region");
  if (result <0) {
    printk(KERN_ALERT "f1_driver: cannot obtain the DDR region.\n");
    return result;
  }

  ddr_base = (void __iomem *)pci_iomap(f1_dev, DDR_BAR, 0);

  result = pci_request_region(f1_dev, OCL_BAR, "OCL Region");
  if (result <0) {
    printk(KERN_ALERT "f1_driver: cannot obtain the OCL region.\n");
    return result;
  }

  ocl_base = (void __iomem *)pci_iomap(f1_dev, OCL_BAR, 0);

  result = pci_request_region(f1_dev, XDMA_BAR, "XDMA Region");
  if (result <0) {
    printk(KERN_ALERT "f1_driver: cannot obtain the XDMA region.\n");
    return result;
  }
  
  xdma_base = (void __iomem *)pci_iomap(f1_dev, XDMA_BAR, 0);


  *(unsigned int *)ddr_base = 0x0;
  
  // allocate MSIX resources

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,8,0)
  result = pci_enable_msix_exact(f1_dev, f1_ints, NUM_OF_USER_INTS);
#else
  result = pci_enable_msix(f1_dev, f1_ints, NUM_OF_USER_INTS);
#endif

  // initialize user interrupts with interrupt specific data
  for(i=0; i<NUM_OF_USER_INTS; i++) {
    f1_dev_id[i] = kmalloc(sizeof(int), GFP_DMA | GFP_USER);
    *f1_dev_id[i] = i;
    request_irq(f1_ints[i].vector, f1_isr, 0, "f1_driver", f1_dev_id[i]);
  }
  
  return 0;

}

static void __exit f1_exit(void) {
  int i;
  
  // free up interrupt vectors and resources
  for(i=0; i<NUM_OF_USER_INTS; i++) {
    free_irq(f1_ints[i].vector, f1_dev_id[i]);
    kfree(f1_dev_id[i]);
  }
  
  // free up MSIX resources
  pci_disable_msix(f1_dev);
  

  if (f1_dev != NULL) {
    pci_iounmap(f1_dev, ddr_base);
    pci_iounmap(f1_dev, ocl_base);
    pci_iounmap(f1_dev, xdma_base);

    pci_release_region(f1_dev, DDR_BAR);    // release DDR & OCL regions
    pci_release_region(f1_dev, OCL_BAR);
    pci_release_region(f1_dev, XDMA_BAR);

    pci_disable_device(f1_dev);
    pci_dev_put(f1_dev);                    // free device memory
  }

  printk(KERN_NOTICE "Removing f1 module\n");
}

module_init(f1_init);

module_exit(f1_exit);


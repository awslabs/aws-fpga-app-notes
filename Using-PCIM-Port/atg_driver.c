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
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/pci.h>

#include <linux/slab.h>



MODULE_AUTHOR("Winefred Washington <winefred@amazon.com>");
MODULE_DESCRIPTION("AWS F1 CL_DRAM_DMA ATG Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

static int slot = 0x0f;
module_param(slot, int, 0);
MODULE_PARM_DESC(slot, "The Slot Index of the F1 Card");

static struct cdev *kernel_cdev;
static dev_t dev_no;

#define DOMAIN 0
#define BUS 0
#define FUNCTION 0
#define DDR_BAR 3
#define OCL_BAR 0

int atg_open(struct inode *inode, struct file *flip);
int atg_release(struct inode *inode, struct file *flip);
long atg_ioctl(struct file *filp, unsigned int ioctl_num, unsigned long ioctl_param);
ssize_t atg_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t atg_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

struct file_operations atg_fops = {
 .read =           atg_read,
 .write =          atg_write,
 // .unlocked_ioctl = atg_ioctl,
 .open =           atg_open,
 .release =        atg_release
};

int atg_major = 0;
#define ATG_BUFFER_SIZE 4096
unsigned char *atg_buffer;
unsigned char *phys_atg_buffer;

struct pci_dev *atg_dev;

void __iomem *ocl_base;

#define CFG_REG           0x00
#define CNTL_REG          0x08
#define NUM_INST          0x10
#define MAX_RD_REQ        0x14

#define WR_INSTR_INDEX    0x1c
#define WR_ADDR_LOW       0x20
#define WR_ADDR_HIGH      0x24
#define WR_DATA           0x28
#define WR_LEN            0x2c

#define RD_INSTR_INDEX    0x3c
#define RD_ADDR_LOW       0x40
#define RD_ADDR_HIGH      0x44
#define RD_DATA           0x48
#define RD_LEN            0x4c

#define RD_ERR            0xb0
#define RD_ERR_ADDR_LOW   0xb4
#define RD_ERR_ADDR_HIGH  0xb8
#define RD_ERR_INDEX      0xbc

#define WR_CYCLE_CNT_LOW  0xf0
#define WR_CYCLE_CNT_HIGH 0xf4
#define RD_CYCLE_CNT_LOW  0xf8
#define RD_CYCLE_CNT_HIGH 0xfc

#define WR_START_BIT   0x00000001
#define RD_START_BIT   0x00000002


static void poke_ocl(unsigned int offset, unsigned int data) {
  unsigned int *phy_addr = (unsigned int *)(ocl_base + offset);
  *phy_addr = data;
}

static unsigned int peek_ocl(unsigned int offset) {
  unsigned int *phy_addr = (unsigned int *)(ocl_base + offset);
  return *phy_addr;
}

static unsigned int test_pattern;

static void run_atg (void) {
    unsigned int data;
    unsigned int status;

    printk(KERN_INFO "ocl_base: %lx\n", (unsigned long)ocl_base);
    printk(KERN_INFO "peek 0: %x\n", *(unsigned int *)ocl_base);
    printk(KERN_INFO "atg_buffer: %lx\n", (unsigned long)atg_buffer);
    printk(KERN_INFO "phys_atg_buffer: %lx\n", (unsigned long)phys_atg_buffer);
    
    // Enable Incr ID mode, Sync mode, and Read Compare
    poke_ocl(CFG_REG, 0x01000018);

    // Set the max number of read requests
    poke_ocl(MAX_RD_REQ, 0x0000000f);

    poke_ocl(WR_INSTR_INDEX, 0x00000000);                                       // write index
    poke_ocl(WR_ADDR_LOW,    ((unsigned int)(unsigned long)phys_atg_buffer & 0xffffffffl));          // write address low
    poke_ocl(WR_ADDR_HIGH,   (unsigned int)((unsigned long)phys_atg_buffer >> 32l)); // write address high
    poke_ocl(WR_DATA,        test_pattern);                                          // write data
    poke_ocl(WR_LEN,         0x00000001);                                            // write 128 bytes

    printk(KERN_INFO "wr low: %x\n", (unsigned int)peek_ocl(WR_ADDR_LOW));
    printk(KERN_INFO "  high: %x\n", (unsigned int)peek_ocl(WR_ADDR_HIGH));

    poke_ocl(RD_INSTR_INDEX, 0x00000000);                                       // read index
    poke_ocl(RD_ADDR_LOW,    ((unsigned int)(unsigned long)phys_atg_buffer & 0xffffffffl));          // read address low
    poke_ocl(RD_ADDR_HIGH,   (unsigned int)(((unsigned long)phys_atg_buffer) >> 32l));  // read address high
    poke_ocl(RD_DATA,        test_pattern);                                             // read data
    poke_ocl(RD_LEN,         0x00000001);                                               // read 128 bytes

    printk(KERN_INFO "rd low: %x\n", (unsigned int)peek_ocl(RD_ADDR_LOW));
    printk(KERN_INFO "  high: %x\n", (unsigned int)peek_ocl(RD_ADDR_HIGH));

    // Number of instructions, zero based ([31:16] for read, [15:0] for write)
    poke_ocl(NUM_INST, 0x00000000);

    // Start writes and reads
    poke_ocl(CNTL_REG, WR_START_BIT | RD_START_BIT);

    // for fun, read status
    status = peek_ocl(CNTL_REG);
    printk(KERN_INFO "status: %d\n", status);

    // Stop ATG
    poke_ocl(CNTL_REG, 0x00000000);

    // for fun, read count registers
    data = peek_ocl(WR_CYCLE_CNT_LOW);
    printk(KERN_INFO "Write Cycle Count Low: %d\n", data);

    data = peek_ocl(RD_CYCLE_CNT_LOW);
    printk(KERN_INFO "Read Cycle Count Low: %d\n", data);

}

static int __init atg_init(void) {
  int result;

  printk(KERN_NOTICE "Installing atg module\n");

  atg_dev = pci_get_domain_bus_and_slot(DOMAIN, BUS, PCI_DEVFN(slot,FUNCTION));
  if (atg_dev == NULL) {
    printk(KERN_ALERT "atg_driver: Unable to locate PCI card.\n");
    return -1;
  }

  printk(KERN_INFO "vendor: %x, device: %x\n", atg_dev->vendor, atg_dev->device);

  result = pci_enable_device(atg_dev);
  printk(KERN_INFO "Enable result: %x\n", result);

  result = pci_request_region(atg_dev, DDR_BAR, "DDR Region");
  if (result <0) {
    printk(KERN_ALERT "atg_driver: cannot obtain the DDR region.\n");
    return result;
  }


  result = pci_request_region(atg_dev, OCL_BAR, "OCL Region");
  if (result <0) {
    printk(KERN_ALERT "atg_driver: cannot obtain the OCL region.\n");
    return result;
  }

  ocl_base = (void __iomem *)pci_iomap(atg_dev, OCL_BAR, 0);   // BAR=0 (OCL), maxlen = 0 (map entire bar)


  result = alloc_chrdev_region(&dev_no, 0, 1, "atg_driver");   // get an assigned major device number

  if (result <0) {
    printk(KERN_ALERT "atg_driver: cannot obtain major number.\n");
    return result;
  }

  atg_major = MAJOR(dev_no);
  printk(KERN_INFO "The atg_driver major number is: %d\n", atg_major);

  kernel_cdev = cdev_alloc();
  kernel_cdev->ops = &atg_fops;
  kernel_cdev->owner = THIS_MODULE;

  result = cdev_add(kernel_cdev, dev_no, 1);

  if (result <0) {
    printk(KERN_ALERT "atg_driver: Unable to add cdev.\n");
    return result;
  }

  atg_buffer = kmalloc(ATG_BUFFER_SIZE, GFP_DMA | GFP_USER);    // DMA buffer, do not swap memory
  phys_atg_buffer = (unsigned char *)virt_to_phys(atg_buffer);  // get the physical address for later

  test_pattern = 0x44434241;  // initialize test_pattern

  return 0;

}

static void __exit atg_exit(void) {

  cdev_del(kernel_cdev);

  unregister_chrdev_region(dev_no, 1);

  if (atg_buffer != NULL)
    kfree(atg_buffer);
  

  if (atg_dev != NULL) {
    pci_iounmap(atg_dev, ocl_base);
    pci_disable_device(atg_dev);

    pci_release_region(atg_dev, DDR_BAR);    // release DDR & OCL regions
    pci_release_region(atg_dev, OCL_BAR);

    pci_dev_put(atg_dev);                    // free device memory
  }

  printk(KERN_NOTICE "Removing atg module\n");
}

module_init(atg_init);

module_exit(atg_exit);

int atg_open(struct inode *inode, struct file *filp) {
  printk(KERN_NOTICE "atg_driver opened\n");
  return 0;
}

int atg_release(struct inode *inode, struct file *filp) {
  printk(KERN_NOTICE "atg_driver closed\n");
  return 0;
}

ssize_t atg_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
  unsigned long result;
  size_t n;

  printk(KERN_INFO "user read size: %zd\n", count);

  n = (count > ATG_BUFFER_SIZE) ? ATG_BUFFER_SIZE : count;
  result = copy_to_user(buf, atg_buffer, count);

  if (result != 0)
    printk(KERN_INFO "Could not copy %ld bytes\n", result);


  *f_pos += n - result;

  return (n-result);
}

ssize_t atg_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
  unsigned long result;
  size_t n;

  printk(KERN_INFO "user write buffer: %s\n", buf);
  printk(KERN_INFO "user write size: %zd\n", count);

  n = (count > ATG_BUFFER_SIZE) ? ATG_BUFFER_SIZE : count;

  // put data in driver buffer
  result = copy_from_user(atg_buffer, buf, n);

  if (*buf != '0')
    run_atg();

  if (result != 0)
    printk(KERN_INFO "Could not copy %ld bytes\n", result);

  *f_pos += n - result;

  return (n - result);
}


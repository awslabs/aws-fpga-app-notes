
## F1 FPGA Application Note
# How to Use PCIe Interrupts
## Version 1.0

## Introduction

When using interrupts a developer can use the device drivers supplied with the F1 Developer's Kit or write their own. This application note describes the basic kernel calls needed for a developer to write a custom interrupt service routine (ISR) and provides an example that demonstrates those calls.

## Concepts

To interrupt the host CPU, the F1 Shell (SH) uses a method called Message Signaled Interrupts, MSI. MSI interrupts are not sent using dedicated wires between the device and the CPU's interrupt controller. MSI interrupts use the PCIe bus to transmit a message from the device to grab the attention of the CPU.

MSI first appeared in PCI 2.2 and enabled a device to generate up to 32 interrupts. In PCI 3.0, an extended version of MSI was created called MSI-X, and MSI-X increases the number of possible interrupts from 32 to 2048. A F1 custom logic (CL) accelerator uses this latest MSI-X protocol and can generate up to 16 user-defined interrupts.

Each of these interrupts may call a different ISR, or one or more of the interrupts can call the same ISR with a parameter to differentiate them. The ISR is executed in kernel space, not user space; therefore, care must be taken when writing a custom ISR to prevent noticeable delays in other ISRs or user space applications. In additional goal of this application note is to illustrate how to place the majority of interrupt processing in a user space process instead of inside the kernel module.

When the device wants to send an interrupt the SH PCIe block is notified by asserting one of 16 user-interrupt signals. The PCIe will acknowledge the interrupt by asserting the acknowledge signal. The PCIe block will issue a MSI-X message to the PCIe bridge located in the server, and the bridge notifies the CPU.

Before using interrupts, they must be [enabled in PCIe configuration space](#enabling-interrupts-in-pcie-configuration-space), [registered with the kernel](#registering-interrupts-with-the-kernel), and [configured in the PCIe block](#configuring-interrupts-in-the-pcie-dma-subsystem).

### Enabling Interrupts in PCIe Configuration Space

MSI-X functionality is enabled using the ```pci_enable_msix``` function. It requires a pointer to the device's PCI data structure, a table to map kernel vector numbers to device interrupts, and number of interrupts are being allocated in the kernel.

```
#define NUM_OF_USER_INTS 16

struct msix_entry f1_ints[] = {
  {.vector = 0, .entry = 0},
  {.vector = 0, .entry = 1},
...
  {.vector = 0, .entry = 15}
};

  // allocate MSIX resources
  result = pci_enable_msix(f1_dev, f1_ints, NUM_OF_USER_INTS);

```

### Registering Interrupts with the Kernel

Once interrupts are allocated in the kernel, ```request_irq``` called is needed to connect a specific interrupt to a specific ISR. In this example all 16 interrupt sources point to the same ISR, ```f1_isr```. To differentiate between the interrupt sources, an unique structure (a pointer to an integer) is registered with the vector. It ISR can retrieve this structure when it is called.

```
  for(i=0; i<NUM_OF_USER_INTS; i++) {
    f1_dev_id[i] = kmalloc(sizeof(int), GFP_DMA | GFP_USER);
    *f1_dev_id[i] = i;                                          // used to differentiate between the user interrupts
    request_irq(f1_ints[i].vector, f1_isr, 0, "f1_driver", f1_dev_id[i]);
  }
  
```

### Configuring Interrupts in the PCIe DMA Subsystem
The snippet of code shown below runs from user space and programs the PCIe block to handle interrupts. To set which user interrupt line cooresponds to which msix_entry/vector, the IRQ Block User Vector Number must be programmed. At reset, the interrupt vector registers all point to index 0. For this example, each user interrupt is programmed to point to a different interrupt vector to illustrate using all 16 entries. The vector numbers are aligned on byte boundaries, and four 32-bit addresses (0x80-0x8c) are used to assign interrupt vectors.

```
    // point each user interrupt to a different vector
    rc = fpga_pci_poke(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x080), 0x03020100);    
    rc |= fpga_pci_poke(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x084), 0x07060504);    
    rc |= fpga_pci_poke(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x088), 0x0b0a0908);
    rc |= fpga_pci_poke(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x08c), 0x0f0e0d0c);
```

The final step is to enable interrupts by writing the interrupt enable mask bits. In the example, all the interrupts are enabled at once with a single write operation; however, you can also use two other registers, IRQ 0x4 and 0x8, to set and clear individual bits without the risk associated with a RMW operation.

```
    // Enable Interrupt Mask (This step seems a little backwards.)    
    rc = fpga_pci_poke(dma_bar_handle, dma_reg_addr(IRQ_TGT, 0, 0x004), 0xffff);
    printf("IRQ Block User Interrupt Enable Mask read_data: %0x\n", read_data);
    
```

## A Barebones ISR
Below is the ISR used in the driver. It uses a spinlock call to make sure multiple interrupts are not interrupting each other. It leaves a short message displaying the user interrupt number. Finally, it increments the DDR location to let the test program know it was called. 

*(Note: Printing from the ISR should only be used for debug purposes, because it can increase interrupt latency.)*

```
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
  
```

## Compiling and Loading the F1 Interrupt Driver

To run this example, launch an F1 instance, clone the aws-fpga Github repository, and download the latest [app note files](./f3fbb176cfa44bf73b4c201260f52f25).

Use the ```fpga-load-local-image``` command to load the FPGA with the CL_DRAM_DMA AFI. *(If you are running on a 16xL, load the AFI into slot 0.)*

Based on your instance size, type one of the following commands:
```
$ sudo lspci -vv -s 0000:00:0f.0  # 16xL
```
Or
```
$ sudo lspci -vv -s 0000:00:1d.0  # 2xL
```
The command will produce output similar to the following:
```

00:0f.0 Memory controller: Device 1d0f:f001
        Subsystem: Device fedc:1d51
        Physical Slot: 15
        Control: I/O- Mem+ BusMaster- SpecCycle- MemWINV- VGASnoop- ParErr- Stepping- SERR- FastB2B- DisINTx-
        Status: Cap+ 66MHz- UDF- FastB2B- ParErr- DEVSEL=fast >TAbort- <TAbort- <MAbort- >SERR- <PERR- INTx-
        Region 0: Memory at c4000000 (32-bit, non-prefetchable) [size=32M]
        Region 1: Memory at c6000000 (32-bit, non-prefetchable) [size=2M]
        Region 2: Memory at 5e000410000 (64-bit, prefetchable) [size=64K]
        Region 4: Memory at 5c000000000 (64-bit, prefetchable) [size=128G]
        Capabilities: [40] Power Management version 3
                Flags: PMEClk- DSI- D1- D2- AuxCurrent=0mA PME(D0-,D1-,D2-,D3hot-,D3cold-)
                Status: D0 NoSoftRst+ PME-Enable- DSel=0 DScale=0 PME-
        Capabilities: [60] MSI-X: Enable- Count=33 Masked-
                Vector table: BAR=2 offset=00008000
                PBA: BAR=2 offset=00008fe0
        Capabilities: [70] Express (v2) Endpoint, MSI 00
                DevCap: MaxPayload 1024 bytes, PhantFunc 0, Latency L0s <64ns, L1 <1us
                        ExtTag+ AttnBtn- AttnInd- PwrInd- RBE+ FLReset-
                DevCtl: Report errors: Correctable- Non-Fatal- Fatal- Unsupported-
                        RlxdOrd+ ExtTag- PhantFunc- AuxPwr- NoSnoop+
                        MaxPayload 128 bytes, MaxReadReq 512 bytes
                DevSta: CorrErr- UncorrErr- FatalErr- UnsuppReq- AuxPwr- TransPend-
                LnkCap: Port #0, Speed 8GT/s, Width x16, ASPM not supported, Exit Latency L0s unlimited, L1 unlimited
                        ClockPM- Surprise- LLActRep- BwNot- ASPMOptComp+
                LnkCtl: ASPM Disabled; RCB 64 bytes Disabled- CommClk-
                        ExtSynch- ClockPM- AutWidDis- BWInt- AutBWInt-
                LnkSta: Speed 8GT/s, Width x16, TrErr- Train- SlotClk+ DLActive- BWMgmt- ABWMgmt-
                DevCap2: Completion Timeout: Range BC, TimeoutDis+, LTR-, OBFF Not Supported
                DevCtl2: Completion Timeout: 50us to 50ms, TimeoutDis-, LTR-, OBFF Disabled
                LnkSta2: Current De-emphasis Level: -6dB, EqualizationComplete+, EqualizationPhase1+
                         EqualizationPhase2+, EqualizationPhase3+, LinkEqualizationRequest-
```
Notice the Capabilities Register [60]. It shows that MSI-X functionality is disabled.

Next, compile the F1 interrupt driver and test program.
```
$ make                     # compiles the kernel module
$ make test                # compiles the test program
```
Now we are ready to install the device driver. Type the following command:
```
$ sudo insmod f1_driver.ko slot=0x0f           # 16xl
```
Or
```
$ sudo insmod f1_driver.ko slot=0x1d           # 2xl
```
You should not see any errors and it should silently return to the command prompt. To check to see if the driver loaded, type:
```
$ dmesg
```

This command will print the message buffer from the kernel. Since the device driver is a kernel module, special prints are used to place messages in this buffer.

Rerun the lspci command and you should see that MSI-X was enabled by the driver.

```
        Capabilities: [60] MSI-X: Enable+ Count=33 Masked-
                Vector table: BAR=2 offset=00008000
                PBA: BAR=2 offset=00008fe0

```


## Generating Interrupts

The cl_dram_dma example design does not generate interrupts independently; however, it does contain a register which when written will produce one or more interrupts. The test program writes this register to generate interrupts. When the interrupt is received by the ISR, it will read a DDR location in cl_dram_dma and increment it by one. The test program monitors the interrupt status register in the CL and looks for the acknowledge bit to assert. For fun, the test program also monitors the DDR location and looks for the 32 bit value to change.

To run the test, type:
```
$ sudo ./f1_test
```

### Interrupt Performance

The test program measures the time it takes from the acknowledging the interrupt to the increment of the DDR location. It gives a rough estimate of the interrupt latency, which is ~120 mS. If your application contains hardware which must be serviced using an ISR, make sure it contains enough buffering to handle interrupt latencies for your application.

### Things to Check

During driver development, you may observe that interrupts stop working. The likely cause is an errata described in 
[MSI-X Interrupts on F1 FPGA x1.3.X Shell v071417d3](https://forums.aws.amazon.com/ann.jspa?annID=4917). This can occur if interrupts are issued by the CL without MSI-X support enabled by the kernel space software.

The Makefile contains a target, ```reload``` that unsticks the interrupts and restores functionality.

```
$ make reload                # reloads the CL design and unsticks the interrupts
```

## For Further Reading

### [AWS F1 FPGA Developer's Kit](https://github.com/aws/aws-fpga)
### [AWS F1 Shell Interface Specification](https://github.com/aws/aws-fpga/blob/master/hdk/docs/AWS_Shell_Interface_Specification.md)
### [The MSI Driver Guide HOWTO](https://www.kernel.org/doc/Documentation/PCI/MSI-HOWTO.txt)
### [Information on Using Spinlocks to Provide Exclusive Access in Kernel.](https://www.kernel.org/doc/Documentation/locking/spinlocks.txt)
### [How To Write Linux PCI Drivers](https://www.kernel.org/doc/Documentation/PCI/pci.txt)

## Revision History

|      Date      | Version |           Revision          |   Shell    |   Developer   |
| -------------- |  :---:  | --------------------------- |   :---:    |     :---:     |
|  Nov. 22, 2017 |   1.0   | Initial Release             | 0x071417d3 | W. Washington |


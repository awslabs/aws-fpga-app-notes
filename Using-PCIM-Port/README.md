
## F1 FPGA Application Note
# How to Use the PCIM AXI Port
## Version 1.0

## Introduction
The purpose of this application note is to provide the F1 developer with additional information when implementing a Custom Logic (CL) that uses the PCIM AXI port to transfer data between the card and host memory. A small device driver is presented that illustrates the basic requirements to control a hardware module connected to the PCIM port.

## Concepts
To perform large data moves (>1KB) between the CL and host, the developer can use the DMA hardware located in the Shell (SH) or implement logic in the CL. The SH DMA is well-suited for linear data transfer, and the F1 Developer’s Kit comes with a compatible device driver. If your application does not perform large linear data transfers or contains DMA logic already, then using the PCIM AXI port is an alternative solution.
To use the PCIM AXI port effectively, the following concepts are important:
-	Virtual to Physical Address Translation
-	PCIM AXI Restrictions
-	Accessing CL Registers from Software

This application note uses the CL_DRAM_DMA example to demonstrate these concepts. The example contains an Automatic Test Pattern Generator (ATG) that is connected to the PCIM port. The ATG is able to write, read, and compare data located in host memory.
Before accessing host memory, software must obtain its physical address and program this address into the ATG. 

### Virtual to Physical Address Translation

Every memory location in an operating system (OS) has at least two addresses: a virtual address and a physical address. Applications running in user space reference memory using virtual addresses, which enables to the OS to host multiple applications. 

The operating system typically handles memory allocation and virtualization in 4KB chunks called pages. Of course an application can allocate memory buffers that are many times larger than 4KB, and from an application’s perspective, the addresses used by the application to access the memory are contiguous. In reality, the physical locations of the pages may be scattered throughout physical memory.

The F1 SH contains hardware to enforce isolation between guess OSes, so that a CL cannot read or write data from another OS. In order for data to move between the application’s user space and the F1 card, software is required to request the physical address of server (host) memory. The physical address may be used by logic in the CL to read/write memory via the PCIM port.

One of the simplest methods is to call ```virt_to_phys()``` to obtain the physical address of a memory buffer. This kernel function takes the address of a 4KB page and uses the MMU page table entries to locate the physical address of the page. The number of calls to ```virt_to_phys()``` should be minimized to improvement driver performance.

When the ATG device driver is opened a 4KB region is allocated. Reading and writing the device file will read and write this memory buffer.
 
For simplicity, the device driver is contained in a single file and assumes the CL_DRAM_DMA is loaded in the FPGA. The code provided is for demonstration purposes only. Take a moment to study the [code](./f3fbb176cfa44bf73b4c201260f52f25#file-atg_driver-c). A production device driver will require additional error checking and device management code.

### PCIM AXI Restrictions

Three reasons exist the PCIM AXI restrictions. First, multiple operating systems are present on a single host. Second, communication between the host and card is over a PCIe interface. And third, the AMBA protocol is used. The following transaction restrictions are placed on the PCIM AXI port:
-	All transactions must use a size of 64 bytes per beat (AxSIZE = 6).
-	All transactions larger than 64 bits must have contiguous byte enables.
-	A transaction must not cross a 4KB address boundary.
-	A transaction must remain within the OS memory space.
-	A transaction must complete within 8 us.
-	A transaction must remain within a set of predetermined address ranges.

If any of these restrictions are violated, monitoring logic located in the SH will terminate the transaction, and error counters are incremented to log the violation.
Examining each of the restrictions in detail is beyond the scope of this application note; therefore, only the timeout and address restrictions are described.

A timeout error is logged when a transaction fails (or takes too long) to complete. The timeout threshold is set at 8us. A PCIM transaction must complete before the timer expires. If it does not, the PCIe transaction will be forcibly completed by SH logic. The values read or written to host memory must be considered undefined, and depending of the CL, the developer may need to reset/re-initialize their CL after a timeout error. 

An address error is logged if a PCIM transaction points to an address which is not contained within the OS memory space, or the Bus Mater Enable bit is disabled in the device's configuration space.

### Accessing CL Registers from Software

The intended purpose of the OCL port is to connect a CL's control/status registers to the PCIe bus. When the F1 card is enumerated the registers are placeed into BAR 0. In order to access these registers, they must be mapped into the device driver's address space. To do this requires four function calls.

```
  // Retrieve the device specific information about the card
  atg_dev = pci_get_domain_bus_and_slot(DOMAIN, BUS, PCI_DEVFN(slot,FUNCTION));

  ...
  // Initialize the card
  result = pci_enable_device(atg_dev);

  ...
  // Mark the region as owned
  result = pci_request_region(atg_dev, OCL_BAR, "OCL Region");

  ...
  // Map the entire BAR 0 region into the driver's address space
  ocl_base = (void __iomem *)pci_iomap(atg_dev, OCL_BAR, 0);   // BAR=0 (OCL), maxlen = 0 (map entire bar)

```
All OCL addresses are relative to the starting address of the BAR.

### Compiling and Running the ATG Device Driver
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
        Control: I/O- Mem+ BusMaster+ SpecCycle- MemWINV- VGASnoop- ParErr- Stepping- SERR- FastB2B- DisINTx-
        Status: Cap+ 66MHz- UDF- FastB2B- ParErr- DEVSEL=fast >TAbort- <TAbort- <MAbort- >SERR- <PERR- INTx-
        Latency: 0
        Region 0: Memory at c4000000 (32-bit, non-prefetchable) [size=32M]
        Region 1: Memory at c6000000 (32-bit, non-prefetchable) [size=2M]
        Region 2: Memory at 5e000410000 (64-bit, prefetchable) [size=64K]
        Region 4: Memory at 5c000000000 (64-bit, prefetchable) [size=128G]
```

Check to make sure the output displays ```BusMaster+```. This indicates that the device is allowed to perform bus mastering operations. It is not unusual to have the Bus Master Enable turned off, ```BusMaster-```, by the OS when loading or unload a device driver or after an error. If the Bus Master Enable is disabled, it can be enabled again by typing:
```
$ sudo setpci -v -s 0000:00:0f.0 COMMAND=06
```
The OCL interface is mapped to Region 0. Accesses to this region will produce AXI transactions at the OCL port of the CL. The ATG registers are located in this region.

Next, compile the ATG device driver and test program.
```
$ make            # compiles the device driver
$ make test       # compiles the test program
```
Now we are ready to install the device driver. Type the following command:
```
$ sudo insmod atg_driver.ko slot=0x0f           # 16xl
```
Or
```
$ sudo insmod atg_driver.ko slot=0x1d           # 2xl
```
You should not see any errors and it should silently return to the command prompt. To check to see if the driver loaded, type:
```
$ dmesg
```
This command will print the message buffer from the kernel. Since the device driver is a kernel module, special prints are used to place messages in this buffer. You should see something similar to the following:
```
[ 6727.147510] Installing atg module
[ 6727.153025] vendor: 1d0f, device: f001
[ 6727.156472] Enable result: 0
[ 6727.165227] The atg_driver major number is: 247
```
The atg_driver will load and an unused major number will be assigned by the OS. Please use the major number (247 is this example) when creating the device special file:
```
$ sudo mknod /dev/atg_driver c 247 0
```
You will not need to create this device file again unless you reboot your instance.
You can now run the test:
```
$ sudo ./atg_test
msg_result: This is a test
msg_result: DCBAECBAFCBAGCB      # expected result
```
The two prints are output by the test program snippet shown in Figure 1. The test program copies a string to the device driver buffer using the ```pwrite()```. The first line is simply a read of the buffer and a print of its contents using ```pread()```. The CL was not accessed.
The second ```pwrite()``` uses a non-zero offset. This is detected by device driver and is used to run ATG logic. The logic overwrites the buffer with a test pattern. This time when the buffer is read, the test pattern is returned. 
```
  // write msg == read msg
  pwrite(fd, test_msg, sizeof(test_msg), 0);
  pread(fd, msg_result, sizeof(test_msg), 0);
  printf("msg_result: %s\n", msg_result);

  // write msg != read msg
  pwrite(fd, test_msg, sizeof(test_msg), 0x100);
  pread(fd, msg_result, sizeof(test_msg), 0);
  printf("msg_result: %s\n", msg_result);
```
*Figure 1. ATG Test Program Body*

With normal file I/O, the ```pwrite/pread``` offset argument is used to move the file pointer to various locations within the file. In this example the offset argument is used by the device driver to enable a different behavior. For your application, you may use the offset to program different addresses within the CL.

During development of your device driver and CL, it is a good idea to periodically check the FPGA metrics to look for errors. Simply, type:
```
$ sudo fpga-describe-local-image -S 0 –M
```
Figure 2 shows an example where the PCIM generated a Bus Master Enable error caused when the CL accessed an invalid address. The ```pcim-axi-protocol-bus-master-enable-error``` field is set along with the error address and count.

To clear the counters, type:
```
$ sudo fpga-describe-local-image -S 0 –C
```

```
AFI          0       agfi-02948a33d1a0e9665  loaded            0        ok               0       0x071417d3
AFIDEVICE    0       0x1d0f      0xf001      0000:00:0f.0
sdacl-slave-timeout=0
virtual-jtag-slave-timeout=0
ocl-slave-timeout=0
bar1-slave-timeout=0
dma-pcis-timeout=0
pcim-range-error=0
pcim-axi-protocol-error=1
pcim-axi-protocol-4K-cross-error=0
pcim-axi-protocol-bus-master-enable-error=1
pcim-axi-protocol-request-size-error=0
pcim-axi-protocol-write-incomplete-error=0
pcim-axi-protocol-first-byte-enable-error=0
pcim-axi-protocol-last-byte-enable-error=0
pcim-axi-protocol-bready-error=0
pcim-axi-protocol-rready-error=0
pcim-axi-protocol-wchannel-error=0
sdacl-slave-timeout-addr=0x0
sdacl-slave-timeout-count=0
virtual-jtag-slave-timeout-addr=0x0
virtual-jtag-slave-timeout-count=0
ocl-slave-timeout-addr=0x0
ocl-slave-timeout-count=0
bar1-slave-timeout-addr=0x0
bar1-slave-timeout-count=0
dma-pcis-timeout-addr=0x0
dma-pcis-timeout-count=0
pcim-range-error-addr=0x0
pcim-range-error-count=0
pcim-axi-protocol-error-addr=0x85000
pcim-axi-protocol-error-count=4
pcim-write-count=2
pcim-read-count=0
DDR0
   write-count=0
   read-count=0
DDR1
   write-count=0
   read-count=0
DDR2
   write-count=0
   read-count=0
DDR3
   write-count=0
   read-count=0
```
*Figure 2. fpga-describe-local-image Metrics Dump*

To understand how to access CL registers mapped on the OCL interface, take a look at the poke_ocl and peek_ocl functions in the [atg_driver.c](./f3fbb176cfa44bf73b4c201260f52f25#file-atg_driver-c) file.
```
static void poke_ocl(unsigned int offset, unsigned int data) {
  unsigned int *phy_addr = (unsigned int *)(ocl_base + offset);
  *phy_addr = data;
}

static unsigned int peek_ocl(unsigned int offset) {
  unsigned int *phy_addr = (unsigned int *)(ocl_base + offset);
  return *phy_addr;
}
```
The ocl_base variable holds the starting address of the OCL BAR and is found by using ```pci_iomap()```.

## For Further Reading

### [AWS F1 FPGA Developer's Kit](https://github.com/aws/aws-fpga)
### [AWS F1 Shell Interface Specification](https://github.com/aws/aws-fpga/blob/master/hdk/docs/AWS_Shell_Interface_Specification.md)

### [Using the PCIM Interface Application Note](https://github.com/awslabs/aws-fpga-app-notes/tree/master/Using-PCIM-Port)

### [How To Write Linux PCI Drivers](https://www.kernel.org/doc/Documentation/PCI/pci.txt)

## Revision History

|     Date      | Version |     Revision    |   Shell    |   Developer   |
| ------------- |  :---:  | --------------- |   :---:    |     :---:     |
| Aug. 21, 2017 |   1.0   | Initial Release | 0x071417d3 | W. Washington |



## F1 FPGA Application Note
# How to Use Write Combining to Improve PCIe Bus Performance
## Version 1.0

## Introduction
A developer has multiple ways to transfer data to a F1 accelerator. For large transfers (>1KByte), either the Shell DMA or custom PCIM logic are good choices for transferring data from host memory to accelerator. For small transfers, the overhead of setting up a hardware-based data move is significant and may consume more time than simply writing the data directly to the accelerator. 
Write combining (WC) is a technique used to increase host write performance to non-cacheable PCIe devices. This application note describes when to use WC and how to take advantage of WC in software for a F1 accelerator. Write bandwidth benchmarks are included to show the performance improvements possible with WC.

## Concepts
The host’s 64-bit address map is subdivided into regions. These regions have various attributes assigned to them by the hypervisor and operating system to control how a user program interacts with system memory and devices.

This app note is focused on two attributes: Non-Cacheable and Write-Combine and how to improve host-to-card write performance. Data written to non-cacheable regions are not stored in a CPU’s cache to be written later (called WriteBack), but are written directly to the memory or device.

For example, if a program writes a 32-bit value to a device mapped in a non-cacheable region, then the device will receive immediately four data bytes. The hardware will generate all the appropriate strobes, masks, and shifts to ensure the bytes are placed on the correct byte lanes with the correct strobes. Depending on the bus hierarchies and protocols, single data accesses can be very slow (<< 1 GB/s), because they use only a portion of the available data bus capacity.

Using a region marked with the WC attribute can improve performance. Writes to a WC region will accumulate in a 64 byte buffer. Once the buffer is full or a flush event occurs (such as a write outside the 64 byte buffer range), a “combined” write to the device is performed. WC increases bus utilization, which results in higher performance.

## Accessing the AppPF Bar 4 Region
The F1 Developer’s Kit includes a FPGA library that can be used to access a F1 card. The library contains functions that simplify accessing a F1 card. This app note uses a small subset. 

To run this example, launch an F1 instance, clone the aws-fpga Github repository, and download the latest [app note files].

After sourcing the ./sdk_setup.sh file, use the fpga-load-local-image command to program the FPGA with the CL_DRAM_DMA AFI. (If you are running on a 16xL, program the FPGA in slot 0.)

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

Region 4 is where the card’s 64 GB of DDR memory is located. To gain access to this region, the memory must be mapped into the user address space of the application. The FPGA library's function, fpga_pci_attach, performs this operation and stores the information in a structure.
```
rc = fpga_pci_attach(slot_id, pf_id, bar_id, write_combine, &pci_bar_handle);
```
Four input arguments are necessary: (1) the slot number, (2) the physical function, (3) the bar/region, and (4) the write combining flag. The function uses these arguments to open the appropriate sysfs file. The ```write_combine``` determines whether to select a region with or without the WC attribute. A F1 card has two regions capable of supporting WC: region 2 and 4. For example, calling the function with the following arguments, 
```
rc = fpga_pci_attach(0, 0, 4, BURST_CAPABLE, &pci_bar_handle);
```
opens the sysfs file: ```/sys/bus/pci/devices/0000:00:0f.0/resource4_wc``` and uses ```mmap``` to create a user space pointer to Region 4 with a WC attribute. The ```BURST_CAPABLE``` enum is part of the F1 FPGA library. To omit the WC attribute, set the ```write_combine``` argument to ```0```. The returned pci_bar_handle structure is used by other FPGA library calls to read and write the F1 card.

The FPGA library call used to write a buffer of UINT32_t data is ```fpga_pci_write_burst```, but writing a custom function is possible.

The write bandwidth is calculated by dividing the total number of bytes transferred by the time it takes for ```fpga_pci_write_burst``` to complete.

```
      /* grab start time */
      rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);

      // perform multiple passes to minimize the affects introduced by clock_gettime
      for(int pass=0; pass < num_of_passes; pass++) {
	if (use_custom)
	  custom_move(pci_bar_handle, 0, buffer, j);
	else
	  fpga_pci_write_burst(pci_bar_handle, 0, buffer, j);
      }

      /* grab end time */
      rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);
```    

As mentioned earlier, developers are not required to use the FPGA library calls but may write their own. ```custom_move``` is an example.

```
int custom_move(pci_bar_handle_t handle, uint64_t offset, uint32_t* datap, uint64_t dword_len) {

  /** get the pointer to the beginning of the range */
  uint32_t *reg_ptr;
  
  int rc  = fpga_pci_get_address(handle, offset, sizeof(uint32_t)*dword_len, (void **)&reg_ptr);
  ...
  
  memcpy((void *)reg_ptr, (void *)datap, sizeof(uint32_t)*dword_len);
  
  ...
}
```
At the heart of the function, is a simple ```memcpy```. The destination address is obtained by calling ```fpga_pci_get_address``` with the pci_handle obtained from ```fpga_pci_attach``` along with the offset into the region.

## Write Performance
This app note includes a program called wc_perf. Run ```make``` in the app note directory to build the program. This program will perform various write operations with and without WC enabled based on the options used. To see a list of the available options, type ```wc_perf -h```.
```
$ sudo ./wc_perf -h
SYNOPSIS
        wc_perf [options] [-h]
Example: wc_perf -i 1024 -w
DESCRIPTION
        Writes bytes to AppPF BAR4 and reports the bandwidth in GB/s
OPTIONS
        -i num - Maximum number of integers to move.
        -p num - Maximum number of passes for measurement.
        -w     - Use WriteCombine region.
        -c     - Use custom memory move.
        -v     - Enable verbose mode.
```

The graph and table show the write bandwidth with different sizes and function with and without WC. _Please note the Y axis is the log of the bandwidth._

![WC Performance Graph](./Write-Combine-Performance.png)

| size (bytes) | burst | burst-wc | custom | custom-wc (GB/s) |
| ---- | :---: | :---: | :---: | :---: |
| 64 | 0.026475 | 0.434842 | 0.052006 | 0.257794 |
| 128 |	0.029538 | 0.451738 | 0.055200 | 0.479473 |
| 256 |	0.028037 | 1.372066 | 0.139926 | 1.870115 |
| 512 |	0.028219 | 1.526626 | 0.174819 | 3.017622 |
| 1024 | 0.027883 | 1.622178 | 0.188182 | 2.060155 |
| 2048 | 0.027718 | 1.632887 | 0.006942 | 1.971544 |
| 4096 | 0.027786 | 1.666118 | 0.006993 | 1.949910 |
| 8192 | 0.027817 | 1.690142 | 0.006990 | 1.915634 |
| 16384 | 0.027669 | 1.699400 | 0.006937 | 1.889101 |
| 32768 | 0.027668 | 1.629239 |	0.006927 | 1.893771 |
| 65536 | 0.027646 | 1.665334 |	0.006942 | 1.894148 |
| 131072 | 0.027732 | 1.637982 | 0.014733 | 1.899972 |
| 262144 | 0.027796 | 1.626075 | 0.033810 | 1.893547 |
| 524288 | 0.059399 | 1.634848 | 0.103400 | 1.891873 |
| 1048576 | 0.137412 | 1.653896 | 0.110928 | 1.891094 |
| 2097152 | 0.387684 | 1.643016 | 0.314870 | 1.879379 |
| 4194304 | 0.432190 | 1.645398 | 0.638929 | 1.877379 |

- burst     = fpga_pci_write_burst
- burst-wc  = fpga_pci_write_burst to a WC region
- custom    = custom move function
- custom-wc = custom move function to a WC region

Based on the data, writing to a WC region achieves almost 5GB/s for 1KB. Above 32KB, performance levels out at just under 2GB/s.

## A Few Words about Side Effects

*If WC is so great why don't we use it all the time?*

To understand why that is not a good idea, a closer look is needed. The ```fpga-describe-local-image``` command is used to dump FPGA metrics such as the number of data beats written to and read from DDR memory.

```
$ sudo fpga-describe-local-image -S 0 -C

AFI          0       agfi-02948a33d1a0e9665  loaded            0        ok               0       0x071417d3
AFIDEVICE    0       0x1d0f      0xf001      0000:00:0f.0

...

DDR0
   write-count=16
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
The ```-C``` option tells the command to retrieve all the available metrics, display them, and reset them to zero. After dumping the metrics, run the following command:

```
$ sudo ./wc_perf
```
By default, ```wc_perf``` will write 16 UINT32 integers to DDR0 without using WC. Each write operation produces one data beat to the DDR 0 memory controller inside the CL. The data bus to the memory controller is 512 bits (64 bytes) wide, but only 32 bits are used for each write, and this is why the DDR 0 write-count shows a value of 16.

Next, run:
```
$ sudo ./wc_perf -w
```
Followed by:
```
$ sudo fpga-describe-local-image -S 0 -C
...
DDR0
   write-count=1
   read-count=0
...
```
The ```-w``` option tells ```wc_perf``` to use WC, and the number of write data beats was reduced from 16 to 1. This is the reason why writing a WC region with small operations is faster, because they are accumulated into larger chunks using a 64 byte buffer located in the CPU core bus interface (BIU). This is also the reason why it cannot be used for all accesses.

Suppose instead of DDR memory there was a piece of hardware located at AppPF BAR4 that required individual writes to control particular functions. The hardware would only see a single access with WC enabled instead of 16 individual writes. Care must be taken when placing logic other than memory such as FIFOs in a WC region, because the order or number of writes does not match the application writes.

A simular issue exists if a FIFO is mapped into a WC region. The FIFO should decode across a 64B aligned address to prevent data written to the same address from being "eaten" by the BIU buffer, and the software must "spray" the data across the 64B buffer. For example, six byte writes to address 0 of BAR4 will result in a single byte write to address 0 when the buffer is stored. To ensure all six bytes are written to the FIFO, the software must write addresses 0 to 5. If the BIU buffer is stored with partial data, the FIFO will receive multiple writes instead of a single 64B beat.

Finally, data being held in the WC buffer prior to being written is not guaranteed to be coherent. If a read is performed before the WC buffer is flushed, it may contain stale data.

When BIU data are stored is not deterministic. Depending on the CPU version, it only contains a small number of BIU buffers (<=6). If another process accesses a different WC region, then a partially filled buffer may be flushed to make room. If an application must make sure that all writes are stored from the BIU, then it is the software's responsiblity to use a x86 SFENCE instruction or similar mechanism to force the writes.

## For Further Reading:
The sysfs Filesystem

* https://www.kernel.org/pub/linux/kernel/people/mochel/doc/papers/ols-2005/mochel.pdf
* https://www.kernel.org/doc/Documentation/filesystems/sysfs.txt

PCI Device Drivers
* https://www.kernel.org/doc/Documentation/PCI/pci.txt

Intel Write Combining Info
* https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/pcie-burst-transfer-paper.pdf
* http://download.intel.com/design/PentiumII/applnots/24442201.pdf

## Revision History

|      Date      | Version |     Revision    |   Shell    |   Developer   |
| -------------- |  :---:  | --------------- |   :---:    |     :---:     |
| Oct.  5, 2017  |   1.0   | Initial Release | 0x071417d3 | W. Washington |
| Nov. 28, 2017  |   1.1   | new '-p' option and performance data | 0x071417d3 | W. Washington |

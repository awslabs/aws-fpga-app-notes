# aws-fpga-app-notes
This repository holds application notes and tutorials for the F1 EC2 Instance. Please refer to the README.md file located in each sub-directory for specific information.

## Application Notes

### [How to Use the PCIM AXI Port](/Using-PCIM-Port)
The purpose of this application note is to provide the F1 developer with additional information when implementing a Custom Logic (CL) that uses the PCIM AXI port to transfer data between the card and host memory. A small device driver is presented that illustrates the basic requirements to control a hardware module connected to the PCIM port.

### [How to Use Write Combining to Improve PCIe Bus Performance](/Using-PCIe-Write-Combining)
Write combining (WC) is a technique used to increase host write performance to non-cacheable PCIe devices. This application note describes when to use WC and how to take advantage of WC in software for a F1 accelerator. Write bandwidth benchmarks are included to show the performance improvements possible with WC.

### [How to Use User PCIe Interrupts](/Using-PCIe-Interrupts)
This application note describes the basic kernel calls needed for a developer to write a custom interrupt service routine (ISR) and provides an example that demonstrates those calls.

## Tutorials

[re:Invent 2017 Developer Workshop](/reInvent17_Developer_Workshop)

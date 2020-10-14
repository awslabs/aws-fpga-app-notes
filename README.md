# aws-fpga-app-notes
This repository holds application notes and tutorials for the F1 EC2 Instance. Please refer to the **README.md** file located in each sub-directory for specific information.

### Application Notes

#### [How to Use the PCIM AXI Port](/Using-PCIM-Port)
The purpose of this application note is to provide the F1 developer with additional information when implementing a Custom Logic (CL) that uses the PCIM AXI port to transfer data between the card and host memory. A small device driver is presented that illustrates the basic requirements to control a hardware module connected to the PCIM port.

#### [How to Use Write Combining to Improve PCIe Bus Performance](/Using-PCIe-Write-Combining)
Write combining (WC) is a technique used to increase host write performance to non-cacheable PCIe devices. This application note describes when to use WC and how to take advantage of WC in software for a F1 accelerator. Write bandwidth benchmarks are included to show the performance improvements possible with WC.

#### [How to Use PCIe User Interrupts](/Using-PCIe-Interrupts)
This application note describes the basic kernel calls needed for a developer to write a custom interrupt service routine (ISR) and provides an example that demonstrates those calls.

### Tutorials

#### [AWS Summit NYC 2018 Developer Workshop](/NYC_Summit18_Developer_Workshop)

This developer workshop is divided in 4 modules.

1. **Connecting to your F1 instance**
1. **Experiencing F1 acceleration**
1. **Developing and optimizing F1 applications with SDAccel**
1. **Wrap-up and next steps**

#### [re:Invent 2018 Developer Workshop](/reInvent18_Developer_Workshop)

This developer workshop is divided in 5 modules.

1. **Connecting to your F1 instance**
1. **Developing an Application using SDAccel GUI**
1. **Developing and optimizing F1 applications with SDAccel**
1. **Optimizing host code**
1. **Wrap-up and next steps**

#### [FPGA Developer AMI Build Recipe (AL2)](/FPGA_Developer_AMI_Build_Recipe_AL2)
Instructions to build a FPGA Developer AMI using Ansible and Packer

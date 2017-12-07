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

This developer workshop is divided in 4 modules.

1. **Connecting to your F1 instance** \
You will start an EC2 F1 instance based on the FPGA developer AMI and connect to it using a remote desktop client. Once connected, you will confirm you can execute a simple application on F1.
1. **Experiencing F1 acceleration** \
AWS F1 instances are ideal to accelerate complex workloads. In this module you will experience the potential of F1 by using FFmpeg to run both a software implementation and an F1-optimized implementation of an H.265/HEVC encoder. 
1. **Developing and optimizing F1 applications with SDAccel** \
You will use the SDAccel development environment to create, profile and optimize an F1 accelerator. The workshop focuses on the Inverse Discrete Cosine Transform (IDCT), a compute intensive function used at the heart of all video codecs.
1. **Wrap-up and next steps** \
Explore next steps to continue your F1 experience after the re:Invent 2017 Developer Workshop.

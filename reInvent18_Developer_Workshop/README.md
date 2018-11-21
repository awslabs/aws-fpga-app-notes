<table style="width:100%">
  <tr>
    <th width="100%" colspan="6"><h2>re:Invent 2018 Developer Workshop</h2></th>
  </tr>
  <tr>
    <td width="20%" align="center"><b>Introduction</b></td>
    <td width="20%" align="center"><a href="SETUP.md">1. Connecting to your F1 instance</a></td> 
    <td width="20%" align="center"><a href="FILTER2D_Lab.md">2. Developing F1 applications</a></td>
    <td width="20%" align="center"><a href="SDAccelGUI_INTRO.md">3. Introduction to SDAccel GUI</a></td>
    <td width="20%" align="center"><a href="HOSTCODE_OPT.md">4. Host Code Optimization</a></td>
    <td width="20%" align="center"><a href="WRAP_UP.md">5. Wrapping-up</td>
  </tr>
</table>

---------------------------------------
### Introduction

Welcome to the re:Invent 2018 Developer Workshop. During this session you will gain hands-on experience with AWS F1 and learn how to develop accelerated applications using the AWS F1 Software Defined Accelerator Development- SDAccel flow.    

#### Overview of the AWS F1 platform and SDAccel flow

The architecture of the AWS F1 platform and the SDAccel development flow are pictured below:

![](./images/introduction/f1_platform.png)

1. Amazon EC2 F1 is a compute instance combining Intel CPUs with Xilinx FPGAs. The FPGAs are programmed with custom accelerators which can accelerate complex workloads up to 100x when compared with servers that use CPUs alone. 
2. An F1 application consists of an x86 executable for the host application and an FPGA binary (also referred to as Amazon FPGA Image or AFI) for the custom hardware accelerators. Communication and data movement between the host application and the accelerators are automatically managed by the [XRT runtime](https://github.com/Xilinx/XRT).
3. Software Defined Accelerator Development- SDAccel is the development environment used to create applications accelerated on the F1 instance. It comes with a fully fledged IDE, x86 and FPGA compilers, profiling and debugging tools.
4. The host application is written in C or C++ and uses the OpenCL APIs to interact with the accelerated functions on the FGPA. The accelerated functions (also referred to as kernels) can be written in C, C++, OpenCL or even RTL.


#### Overview of the re:Invent 2018 Developer Workshop modules

This developer workshop is divided in 4 modules. It is recommended to complete each module before proceeding to the next.

1. **Connecting to an F1 instance** \
You will start an EC2 F1 instance based on the FPGA developer AMI and connect to it using a remote desktop client. Once connected, you will confirm you can execute a simple application on F1.
1. **Developing and optimizing F1 applications with SDAccel** \
You will use the SDAccel development environment to create and optimize an F1 accelerator. The workshop focuses on the 2D video frame Filter , a compute intensive function which is widely used in image processing.
1. **Introduction to SDAccel GUI** \
You will learn about creating a project workspace using SDAccel GUI, its layout and how to debug an example application.
1. **Optimizing host code** \
You will experiment with profiling & debug tools to optimize host code to enhance your application.
1. **Wrap-up and next steps** \
Explore next steps to continue your F1 experience after the re:Invent 2018 Developer Workshop.

Since building FPGA binaries is not instantaneous, all the modules of this Developer Workshop will use precompiled FPGA binaries.

---------------------------------------

<p align="center"><b>
Start the next module: <a href="SETUP.md">1. Connecting to your F1 instance</a>
</b></p>


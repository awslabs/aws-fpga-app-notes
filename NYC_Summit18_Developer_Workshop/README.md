<table style="width:100%">
  <tr>
    <th width="100%" colspan="5"><h2>AWS Summit NYC 2018 Developer Workshop</h2></th>
  </tr>
  <tr>
    <td width="20%" align="center"><b>Introduction</b></td>
    <td width="20%" align="center"><a href="SETUP.md">1. Connecting to your F1 instance</a></td> 
    <td width="20%" align="center"><a href="BLACK_SCHOLES_Lab.md">2. Experiencing F1 acceleration</a></td>
    <td width="20%" align="center"><a href="IDCT_Lab.md">3. Developing F1 applications</a></td>
    <td width="20%" align="center"><a href="WRAP_UP.md">4. Wrapping-up</td>
  </tr>
</table>

---------------------------------------
### Introduction

Welcome to the AWS Summit NYC 2018 Developer Workshop. During this session you will gain hands-on experience with AWS F1 and learn how to develop accelerated applications using the AWS F1 C/C++/OpenCL flow and the Xilinx SDAccel development environment.  Prior to starting the workshop, please review the presentation on [accelerating your C/C++ applications with Amazon EC2 F1 Instances](https://www.slideshare.net/AmazonWebServices/accelerate-your-cc-applications-with-amazon-ec2-f1-instances-cmp402-reinvent-2017).    

#### Overview of the AWS F1 platform and SDAccel flow

The architecture of the AWS F1 platform and the SDAccel development flow are pictured below:

![](./images/introduction/f1_platform.png)

1. Amazon EC2 F1 is a compute instance combining x86 CPUs with Xilinx FPGAs. The FPGAs are programmed with custom hardware accelerators which can accelerate complex workloads up to 30x when compared with servers that use CPUs alone. 
2. An F1 application consists of an x86 executable for the host application and an FPGA binary (also referred to as Amazon FPGA Image or AFI) for the custom hardware accelerators. Communication between the host application and the accelerators are automatically managed by the OpenCL runtime.
3. SDAccel is the development environment used to create F1 applications. It comes with a full fledged IDE, x86 and FPGA compilers, profiling and debugging tools.
4. The host application is written in C or C++ and uses the OpenCL API to interact with the accelerated functions. The accelerated functions (also referred to as kernels) can be written in C, C++, OpenCL or even RTL.


#### Overview of the AWS Summit NYC 2018 Developer Workshop modules

This developer workshop is divided in 4 modules. It is recommended to complete each module before proceeding to the next.

1. **Connecting to your F1 instance** \
You will start an EC2 F1 instance based on the FPGA developer AMI and connect to it using a remote desktop client. Once connected, you will confirm you can execute a simple application on F1.
1. **Experiencing F1 acceleration** \
AWS F1 instances are ideal to accelerate complex workloads. In this module you will experience the potential of F1 by using a financial risk simulator model to predict call/put option pricing. 
1. **Developing and optimizing F1 applications with SDAccel** \
You will use the SDAccel development environment to create, profile and optimize an F1 accelerator. The workshop focuses on the Inverse Discrete Cosine Transform (IDCT), a compute intensive function used at the heart of all video codecs.
1. **Wrap-up and next steps** \
Explore next steps to continue your F1 experience after the re:Invent 2017 Developer Workshop.

Since building FPGA binaries is not instantaneous, all the modules of this Developer Workshop will use precompiled FPGA binaries.

---------------------------------------

<p align="center"><b>
Start the next module: <a href="SETUP.md">1. Connecting to your F1 instance</a>
</b></p>


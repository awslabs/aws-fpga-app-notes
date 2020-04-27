
<table>
<tr>
 <td align="center"><h1>Methodology for Optimizing Accelerated FPGA Applications
 </td>
 </tr>
</table>

# Introduction

The methodology for developing optimized accelerated applications is comprised of two major phases: architecting the application, and developing the accelerator to meet your desired performance goals.

* In the first phase, you make key decisions about the application architecture by determining which software functions should be accelerated onto FPGA kernels, how much parallelism can be achieved, and how to deliver it in code.
* In the second phase, you implement kernels by structuring the source code, and applying the necessary compiler options and pragmas to create the kernel architecture needed to achieve the performance target.

You begin this tutorial with a baseline application, and profile it to examine the potential for hardware acceleration. The tutorial application performs a 2D convolution of an RGBA video and a set of filter coefficients using ffmpeg, a popular multimedia framework that can play, transcode, mux, demux, and filter many audio/video formats. Then, you perform various optimizations on both the host program and kernel side. In this tutorial, you will work with the following optimization techniques:

* Memory transfer optimizations
* Fixed point data type adoption
* Dataflow and streams
* Optimization of Loops

This tutorial follows the *SDAccel Methodology Guide* ([UG1346](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2019_1/ug1346-sdaccel-methodology-guide.pdf)) about how to migrate a CPU-based application to an optimized FPGA-accelerated design. For a deeper understanding, you should review that material as you are working through this tutorial.

# Before You Begin

## AWS F1 environment setup

Before starting this lab, perform a fresh reinstall of the AWS EC2 FPGA Development Kit and download the lab contents on your instance. Open a new terminal by right-clicking anywhere in the Desktop area and selecting **Open Terminal**, then run the following commands:

```bash  
# Install the AWS EC2 FPGA Development Kit
cd ~
git clone https://github.com/aws/aws-fpga.git
cd aws-fpga                                   
source sdaccel_setup.sh

# Download the SDAccel F1 Developer Labs
cd ~
rm -rf SDAccel-AWS-F1-Developer-Labs
git clone https://github.com/Xilinx/SDAccel-AWS-F1-Developer-Labs
```



## ffmpeg plugin setup

This tutorial requires that the ffmpeg framework is installed on the machine where the steps will be executed. These dependencies can be downloaded by running the following commands.

* For CentOS:

   ```
   sudo yum localinstall --nogpgcheck https://download1.rpmfusion.org/free/el/rpmfusion-free-release-7.noarch.rpm
   sudo yum install ffmpeg
   ```

The labs in this tutorial use:

* BASH Linux shell commands.
* 2019.1 SDx release and the *aws-vu9p-f1-04261818_dynamic_5_0* platform.
* A `Makefile` that is detailed and contains many steps and variables. For a discussion of the `Makefile` structure and contents, refer to [Understanding the Makefile](./HowToRunTutorial.md).


# Tutorial Overview

The following labs walk through the best practices for taking an existing application and optimizing it as an FPGA-accelerated application. This tutorial is divided into several different labs that show the methodology. It is recommended to complete each lab before proceeding to the next.

1. [Evaluating the Original Application](RunOriginalCode.md): In this lab, the original C-based application processes an input video to generate the convolution output video. This lab also discusses setting realistic performance goals for an accelerated application.
2. [Creating an SDAccel Application from the C Application](baseline.md): Convert the original C code into a host program and hardware kernel that is called by the host using the OpenCL™ API.
3. [Optimizing Memory Transfers](localbuf.md): Learn methods for optimizing the hardware kernel for improved memory access. You will learn how to use local cache to make efficient use of the FPGA bandwidth.
4. [Optimizing Using Fixed Point Data Types](fixedtype.md): Discusses how data types affect design performance.
5. [Optimizing with Dataflow](dataflow.md): Improve the compute efficiency of your kernel, applying dataflow and streaming to improve the data-path in your kernel.
6. [Using Out-of-Order Queues and Multiple Compute Units](multi-CU.md): Modify the OpenCL API calls in the host program to allow for out-of-order task execution, and increase parallelism on the accelerator by synthesizing multiple kernels to perform the work.
7. [Running the Accelerator in Hardware](RunOnHardware.md): All the previous steps have been run in Hardware Emulation mode. Here you run the application on the acceleration hardware.

</br>
<hr/>
<p align= center><b><a href="/README.md">Return to Main Page</a></b></p>
</br>
<p align="center"><sup>Copyright&copy; 2019 Xilinx</sup></p>

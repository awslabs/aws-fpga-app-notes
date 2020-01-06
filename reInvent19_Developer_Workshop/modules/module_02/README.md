## Module 2: Optimize interactions between the host CPU and the FPGA

### Preparing to run the labs

Skip this step if the SDAccel environment is already setup.

Before starting this module, perform a fresh reinstall of the AWS EC2 FPGA Development Kit and download the lab contents on your instance. Open a new terminal by right-clicking anywhere in the Desktop area and selecting **Open Terminal**, then run the following commands:

```
# Install the AWS EC2 FPGA Development Kit
cd ~
git clone https://github.com/aws/aws-fpga.git
cd aws-fpga                                   
source sdaccel_setup.sh

# Download the SDAccel F1 Developer Labs
cd ~
rm -rf SDAccel-AWS-F1-Developer-Labs
git clone https://github.com/Xilinx/SDAccel-AWS-F1-Developer-Labs.git SDAccel-AWS-F1-Developer-Labs
```



### Setup for running application on FPGA
```
sudo sh
# Source the SDAccel runtime environment
source /opt/xilinx/xrt/setup.sh
```

### Module overview

This module is divided in two labs focusing on interactions between the CPU and the FPGA (data transfers, task invocations) and their impact on overall performance. The application used in this module is a Bloom filter, a space-efficient probabilistic data structure used to test whether an element is a member of a set. Since building FPGA binaries is not instantaneous, a precompiled FPGA binary is provided for both labs.

1. **Experiencing acceleration** \
You will profile the Bloom filter application and evaluate which sections are best suited for FPGA acceleration. You will also experience the acceleration potential of AWS F1 instances by running the application first as a software-only version and then as an optimized FPGA-accelerated version.

1. **Optimizing CPU and FPGA interactions for improved performance** \
You will learn the coding techniques used to create the optimized version run in the first lab. Working with a predefined FPGA accelerator, you will experience how to optimize data movements between host and FPGA, how to efficiently invoke the FPGA kernel and how to overlap computation on the CPU and the FPGA to maximize application performance.

After you complete the last lab, you will be guided to close your RDP session, stop your F1 instance and explore next steps to continue your experience with SDAccel on AWS.

---------------------------------------

<p align="center"><b>
Start the first lab: <a href="host_eval.md">Experience FPGA Acceleration</a>
</b></p>

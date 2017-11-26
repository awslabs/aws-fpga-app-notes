<table style="width:100%">
  <tr>
    <th width="100%" colspan="5"><h2>reInvent17 Developer Workshop</h2></th>
  </tr>
  <tr>
    <td width="20%" align="center"><a href="README.md">Introduction</a></td>
    <td width="20%" align="center"><a href="SETUP.md">1. Connecting to your F1 instance</a></td> 
    <td width="20%" align="center"><a href="FFMPEG_Lab.md">2. Experiencing F1 acceleration</a></td>
    <td width="20%" align="center"><a href="IDCT_Lab.md">3. Developing F1 applications</a></td>
    <td width="20%" align="center"><b>4. Wrapping-up</b></td>
  </tr>
</table>

---------------------------------------

## Wrap-Up and Next Steps

It is now time to wrap-up this lab. Please follow these steps to close your RDP session and stop your instance. It is important to always stop or terminate AWS EC2 instances when you are done using them. This is a recommended best practice to avoid unwanted charges.


### Stopping your instance

* Click the 'X' icon to close your RDP client.
* On your local machine, return to your browser and to the tab showing the **EC2 Console** and the details of your running instance.
   * If necessary, use the link which was emailed to you to return to the proper web page.
* In the **EC2 Console**, make sure you instance is selected
* Click the **Actions** button, select **Instance State** and then click **Stop**.

Note: permission to **Terminate** instances was disabled for all user accounts of this Developer Lab to prevent accidental terminations.

### Congratulations!
You have successfully completed all the modules of reInvent17 Developer Workshop. 
1. You started a pre-configured EC2 F1 instance and connected to it using a remote desktop client. 
1. You experienced the acceleration potential of F1 by using ffmpeg and an F1-optimized HEVC encoder. 
1. You used SDAccel to create, profile and optimize an F1 accelerator. 
1. You closed your RDP session and properly stopped your F1 instance.

### Next steps
To take your AWS F1 and SDAccel experience further, we recommend the following resources:

| Resource | Title                       | Description  |
| -------- |---------------------------- | ----- |
| F1 App | **Test-drive Xilinx GoogLeNet / ResNet on AWS F1** | See for yourself how Xilinx can accelerate machine learning image classification on AWS F1. [**Click here**](https://www.xilinx.com/applications/megatrends/machine-learning/aws-f1-test-drive.html). |
| Video  | **Learn more about the SDAccel OpenCL application structure** | This video introduces the host code and kernel elements of an OpenCL application and explains how they map to Xilinx FPGAs. [**Click here**](https://www.xilinx.com/video/hardware/opencl-application-structure.html). |
| Tutorial | **Create, configure and test an AWS F1 instance** | Learn how to launch and configure an F1 instance to work with the SDAccel development environment. [**Click here**](https://github.com/Xilinx/SDAccel_Examples/wiki/Create,-configure-and-test-an-AWS-F1-instance). |
| Tutorial | **Get started on F1 with the SDAccel C/OpenCL flow** | This guide takes new users through all the steps required to build a first working application on F1. [**Click here**](https://github.com/Xilinx/SDAccel_Examples/wiki/Getting-Started-on-AWS-F1-with-SDAccel-and-C-OpenCL-Kernels). |
| Tutorial | **Get started on F1 with the SDAccel RTL flow** | This guide is targeted to developers with prior hardware design experience and legacy RTL designs. [**Click here**](https://github.com/Xilinx/SDAccel_Examples/wiki/Getting-Started-on-AWS-F1-with-SDAccel-and-RTL-Kernels). |

In addition, the examples below are also very good resources to get familiarized with more complex applications and learn more about optimization techniques and 

| Resource | Title                       | Description  |
| -------- |---------------------------- | ----- |
| Examples | **Github Examples** | Explore a repository of more than 80 examples to learn more about the SDAccel OpenCL programming model. [**Click here**](https://github.com/Xilinx/SDAccel_Examples). |
| Examples | **Matrix Multiply** | This GEMM-compatible library provide 2TOPs on 16-bit date types. [**Click here**](https://github.com/Xilinx/gemx). |
| Examples | **Data Analytics** | The SQL search example running on F1 is 6 to 10x faster than CPU. [**Click here**](https://github.com/Xilinx/data-analytics). |
| F1 App | **Automata Processing** | REAPR is flexible end-to-end framework for automata processing developed by University of Virginia. REAPR is 44x faster than Hyperscan on F1, and 6x faster than GPU. An F1 AMI will be available soon. Check [**http://cap.virginia.edu/**](http://cap.virginia.edu) for updates. |

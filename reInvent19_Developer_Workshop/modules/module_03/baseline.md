<table>
 <tr>
 <td align="center"><h1>Methodology for Optimizing Accelerated FPGA Applications
 </td>
 </tr>
</table>

# 2. Creating an SDAccel Application from the C Application

In the last lab, you determined that the `convolve_cpu` function is where the bulk of time is spent in the application. To accelerate performance, you are going to move that function into a kernel to run on the FPGA accelerator card.

This lab discusses the process for converting a C function into a hardware kernel and evaluating its performance. You will also establish the baseline performance numbers for the accelerated application that you will use to compare to each additional optimization in the subsequent labs. You are going to take your first FPGA kernel and run it on a fixed image size. Because you will be running your kernel in hardware emulation, you will use a smaller image size. For now, set the image size to 512x10.

To measure the performance, you will use the Profile Summary and Timeline Trace reports. For details about these reports, refer to the [Generating Profile and Trace Reports](../Pathway3/ProfileAndTraceReports.md) lab in the Essential Concepts for Building and Running the Accelerated Application tutorial.

## Kernel Code Modifications

Use the following instructions to convert the C code into kernel code, and convert the `convolve_cpu` function into a hardware accelerated kernel to run on the FPGA.

>**TIP:** The completed kernel source file is provided in the `reference-files/baseline` folder. You can use it as a reference if needed.

1. Select the `convolve_kernel.cpp` file from `cpu_src` folder, and make a copy called `convolve_fpga.cpp`.

2. Open the `convolve_fpga.cpp` file, and rename the `convolve_cpu` function to `convolve_fpga`.

3. Add the following HLS INTERFACE pragmas at the beginning of the function, within the braces.

        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE s_axilite port=inFrame bundle=control
        #pragma HLS INTERFACE s_axilite port=outFrame bundle=control
        #pragma HLS INTERFACE s_axilite port=coefficient bundle=control
        #pragma HLS INTERFACE s_axilite port=coefficient_size bundle=control
        #pragma HLS INTERFACE s_axilite port=img_width bundle=control
        #pragma HLS INTERFACE s_axilite port=img_height bundle=control

    The FPGA kernel must have a single AXI4-Lite slave control interface (`s_axilite`) which allows the host application to configure and communicate with the kernel. All function arguments including the _return_ must be mapped to this `control` interface using dedicated pragmas.

4. Each pointer argument must also be mapped to an AXI memory mapped master port (`m_axi`) using a dedicated pragma. Add the following pragmas to the function.

        #pragma HLS INTERFACE m_axi port=inFrame offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=outFrame offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=coefficient offset=slave bundle=gmem2

    The AXI Master ports are used by the kernel to access data in the global memory. The base address of the data (`offset`) is provided to the kernel through the previously defined control interface (`slave`). The _bundle_ property of the pragma allows you to name the interface ports. In this example, the function has three pointer arguments, but the kernel will have two ports, `gmem1` and `gmem2`. The `gmem1` port will be used to read `inFrame` and write `outFrame`, and the `gmem2` port will be used to read `coefficient`.

5. Finally, add the `data_pack` pragma for the `inFrame` and `outFrame` arguments. This pragma tells the compiler to aggregate all the elements of a struct into a single, wide vector, allowing all members of the struct to be accessed simultaneously.

        #pragma HLS data_pack variable=inFrame
        #pragma HLS data_pack variable=outFrame

   >**NOTE:** For more details about the pragma meanings, refer to the *SDx Pragma Reference Guide* ([UG1253](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2019_1/ug1253-sdx-pragma-reference.pdf)).

6. Save the `convolve_fpga.cpp` file.

## Run Hardware Emulation

1. Go to the `makefile` directory, and use the following command to run hardware emulation.

    ```
    cd design/makefile
    make run TARGET=hw_emu STEP=baseline SOLUTION=1 NUM_FRAMES=1
    ```

    >**TIP:** The modifications based on each lab can made from the `src` directory. The completed code is also provided for all the optimizations in this lab. You do not need to modify the code if you just want to run the results. You can enable this by setting `SOLUTION=1`.

    You should see the following text in the console both during and after the application has finished executing.

    ```
    Processed 0.02 MB in 3241.206s (0.00 MBps)

    INFO: [SDx-EM 22] [Wall clock time: 23:56, Emulation time: 9.17236 ms] Data transfer between kernel(s) and global memory(s)
    convolve_fpga_1:m_axi_gmem1-DDR          RD = 167.781 KB             WR = 20.000 KB
    convolve_fpga_1:m_axi_gmem2-DDR          RD = 167.781 KB             WR = 0.000 KB
    ```

    The first line prints the clock time and the emulation time of the previous run. The clock time is the current system time, and the emulation time is the time the emulator simulated for this run. The subsequent lines display the data size that was transferred between the FPGA device and on-board FPGA RAM.

## Generate Reports for Hardware Emulation

1. Use the following command to generate the Profile Summary report and Timeline Trace.

    ```
    make gen_report TARGET=hw_emu STEP=baseline
    ```

## View Profile Summary for Hardware Emulation

1. Use the following command to view the Profile Summary report.

    ```
    make view_prof_report TARGET=hw_emu STEP=baseline
    ```

1. Now, look at the Profile Summary report. In the *Kernel Execution* section, you can see that the kernel execution time is 10.807 ms. This number will be used to measure the performance boost after each optimization step.

1. The *Kernels to Global Memory* section shows that the average bytes per transfer is only 4 bytes, which will impact the performance.

1. Also, the number of transfers is 91024 which seems to be too much for the required data transfer times. In the next sections, you will optimize on those areas.

    ![][baseline_hwemu_profilesummary]

1. Record the performance results from the Profile Summary report, and fill in the following table. Your numbers might vary.

    For this lab, keep track of the following information:

* **Time**: The hardware emulation time. Retrieved from the Profile Summary report listed in the *Top Kernel Execution* section under *Duration*.
* **Kernel**: The name of the kernel.
* **Image Size**: The size and shape of the images.
* **Reads**: The number of bytes read from the on-board RAM. Printed in the Console window and also listed in the Profile Summary report.
* **Writes**: The number of bytes written to the on-board RAM. Printed in the Console window and also listed in the Profile Summary report.
* **Avg. Read**: The average read size of each memory transaction. Retrieved from the Profile Summary report in the *Data Transfer* tab under the *Data Transfer: Kernels and Global Memory* section.
* **Avg. Write**: The average write size of each memory transaction. Retrieved from the Profile Summary report in the *Data Transfer* tab under the *Data Transfer: Kernels and Global Memory* section.
* **BW**: The bandwidth is calculated by (512x10x4)/Time.

| Step          | Image Size | Time (HW-EM)(ms) | Reads (KB) | Writes (KB) | Avg. Read (KB) | Avg. Write (KB) | BW (MBps) |
| :------------ | :--------- | ---------------: | ---------: | ----------: | -------------: | --------------: | --------: |
| baseline      | 512x10     | 10.807            | 344        | 20.0        | 0.004          | 0.004           |   1.9     |

---------------------------------------

[baseline_hwemu_profilesummary]: ./images/baseline_hwemu_pfsummary_aws.JPG "Baseline version hardware emulation profile summary"

## Next Step

You have created the hardware kernel and run the application in hardware emulation mode. You will now begin to examine data transfers between the host application and the kernel, and determine some good strategies for [optimizing memory transfers](./localbuf.md).

</br>
<hr/>
<p align="center"><b><a href="./README.md">Return to Start of Tutorial</a></b></p>

<p align="center"><sup>Copyright&copy; 2019 Xilinx</sup></p>

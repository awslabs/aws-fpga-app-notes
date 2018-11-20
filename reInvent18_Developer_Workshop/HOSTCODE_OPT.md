<table style="width:100%">
  <tr>
     <th width="100%" colspan="5"><h2>re:Invent 2018 Developer Workshop</h2></th>
  </tr>
  <tr>
    <td width="20%" align="center"><a href="README.md">Introduction</a></td>
    <td width="20%" align="center"><a href="SETUP.md">1. Connecting to your F1 instance</a></td>
    <td width="20%" align="center"><a href="FILTER2D_Lab.md">2. Developing F1 applications</a></td>
	<td width="20%" align="center"><a href="SDAccelGUI_INTRO.md">3. Introduction to SDAccel GUI</a></td>
    <td width="20%" align="center"><b>4. Host Code Optimization</b></td>
    <td width="20%" align="center"><a href="WRAP_UP.md">5. Wrapping-up</td>
  </tr>
</table>

---------------------------------------
### Host Code Optimization

This tutorial concentrates on performance tuning of the host code associated with an FPGA Accelerated Application. Host code optimization is only one aspect of performance optimization, which also includes the following disciplines:

  * Host program optimization
  * Kernel Code optimization
  * Topological optimization
  * Implementation optimization

In the following, you will operate on a simple, single, generic C++ kernel implementation. This allows you to eliminate any aspects of the kernel code modifications, topological optimizations and implementation choices from the analysis of host code implementations. It should also be noted, that the host code optimization techniques shown in this tutorial are limited to aspects of optimizing the accelerator integration. Additional common techniques which allow utilization of multiple CPU cores or memory management on the host code are not part of this discussion, but are covered in the SDAccel Profiling and Optimization Guide (UG1207).

More specifically, in the following sections you will focus on four specific host code optimization concerns:
  * Software Pipelining / Event Queue
  * Kernel and Host Code Synchronization
  * Buffer Size


### Model

The kernel used in this example is created solely for the purpose of host code optimization. Thus the kernel doesn't implement a specific functionality, rather it is designed to be static throughout the experiments of this tutorial to let you see the effects of your optimizations on the host code.  

The C++ kernel has one input and one output port. These ports are 512 bits wide to optimally utilize the AXI bandwidth. The number of elements consumed by kernel per execution, is configurable through the "numInputs" parameter. Similarly the "processDelay" parameter can be used to alter the latency of the kernel. The algorithm increments the input value by the "processDelay" value. This increment is however implemented by a loop executing "processDelay" times incrementing the input value by one each time. As this loop is present with-in the kernel implementation, each iteration will end up requiring constant amount of cycles which can be multiplied by the processDelay number.

The kernel is also designed to enable AXI burst transfers. Towards that end, the kernel contains a read and a write process, executed in parallel with the actual kernel algorithm (exec). The read and the write process initiate the AXI transactions in a simple loop, and write the received values into internal FIFOs, or reads from internal FIFOs and writes to the AXI outputs. Vivado HLS implements these blocks as concurrent parallel processes since the DATAFLOW pragma was set on the surrounding "pass_dataflow" function.

*Please note, all instructions in this tutorial are intended to be run from the hostcode_opt directory.*

### Building the Kernel

Although some host code optimizations can be performed quite well with hardware emulation, accurate runtime information and running of large test vectors will require the kernel to be exected on the actual system. In general, during host code optimization the kernel is not expected to change, this is a one time hit and can easily be performed even before the hardware model is finalized.

For this tutorial, the example was setup to build the hardware bitstream one time by issuing the following commands:

``` bash
make TARGET=hw DEVICE=$AWS_PLATFORM kernel
```

*$AWS_PLATFORM* is F1 device Platform (.xpfm) in which the kernel runs.

**NOTE:** This build process will take several hours and you will need to wait for completion of the kernel compilation before actually analysing the host code impact. For this workshop we have precompiled the kernel for you. precompiled kernels awsxclbin file can be found in *xclbin* directory.

``` bash
  ./xclbin/pass.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.awsxclbin
```  

### Host Code

Before examining different implementation options for the host code, take a look at the structure of the code. The host code file is designed to let you focus on the key aspects of host code optimization. Towards that end, three classes are provided through header files in the common source directory (srcCommon):  

   * [hostcode_opt/srcCommon/AlignedAllocator.h](hostcode_opt/srcCommon/AlignedAllocator.h): Strictly speaking the AlignedAllocator is a small struct with two methods. This struct is provided as a helper class to support aligned memory allocation for the test vectors. Memory aligned blocks of data can be transfered much more rapidly and the OpenCL library will create warnings if the data transmitted isn't memory aligned.

   * [hostcode_opt/srcCommon/ApiHandle.h](hostcode_opt/srcCommon/ApiHandle.h): This class encapsulates the main OpenCL objects, namely the context, the program, the device_id, the execution kernel, and the command_queue. These structures are populated by the constructor which basically steps through the default sequence of OpenCL function calls. There are only two configuration parameters to the constructor: the first is a string containing the name of the bitstream (xclbin) to be used to program the FPGA, the second is a boolean to determine if an out-of-order queue or a sequential execution queue should be created.

   The class provides accessor functions to the queue, context, and kernel required for the generation of buffers and the scheduling of tasks on the accelerator. The class also automatically releases the allocated OpenCL objects when the ApiHandle destructor is called.

   * [hostcode_opt/srcCommon/Task.h](hostcode_opt/srcCommon/Task.h): An object of class "Task" represents a single instance of the workload to be executed on the accelerator. Whenever an object of this class is constructed, the input and output vectors are allocated and initialized, based on the buffer size to be transfered per task invocation. Similarly, the destructor will deallocate any object generated during the task execution. It should also be noted, this encapsulation of a single workload for the invocation of a module allows this class also to contain an output validator function (outputOk).

   The constructor of this class contains two parameters: the first, bufferSize, determines how many 512 bit values are transfered when this task is executed; the second, processDelay, simply provides the similary named kernel parameter and is also used during validation.

   The most important member function of this class is the "run"-function. This function fills the OpenCL queue with the three different steps for executing the algorithm. These steps are writing data to the FPGA accelerator, setting up the kernel and running the accelerator, and reading the data back from the DDR memory on the FPGA. To perform this task, buffers are allocated on the DDR for the communication. Also events are used to express the dependency between the different task (write before execute before read).

In addition to the ApiHandle object, the "run"-function has one conditional argument. This argument allows a task to be dependent on a previously generated event. This allows the host code to establish task order dependencies as illustrated later in the tutorial.

None of the code in any of these header files will be modified during this tutorial. All key concepts will be shown in the different host.cpp files, as found in the srcBuf, srcPipeline, or srcSync folders. However, even the main function in the host.cpp file follows a specific structure which is examined here.

The main function contains the following sections and these are marked in the source accordingly.

1. Environment / Usage Check
1. Common Parameters:
   1. numBuffers: Not expected to be modified, it is simply used to determine how many kernel invocations are performed.
   1. oooQueue: If true, this boolean value is used to declare the kind of OpenCL eventqueue is generated inside of the ApiHandle.
   1. processDelay: As stated, this parameter can be used to artificially delay the computation time required by the kernel. However, this will not be utilized in this version of the tutorial.
   1. bufferSize: This value is used to declare the number of 512 bit values to be transfered per kernel invocation.
   1. softwarePipelineInterval: is used as parameter to determine how many operations are allowed to be prescheduled before synchronization is happening.
1. Setup: To ensure that you are aware of the status of configuration variables, this section will print out the final configuration.
1. Execution: In this section, you will be able to model several different host code performance issues. These are the lines you will focus on for this tutorial.
1. Testing: Once execution has completed, this section performs a simple check on the output.
1. Performance Statistics: If the model is run on an actual accelerator card (not emulated), the host code will calculate and print the performance statisics based on system time measurements.

**NOTE:** the setup as well as the other sections can print additional messages recording system status as well as overall PASS or FAIL of the run.

### Pipelined Kernel Execution using Out of Order Event Queue

In this first exercise, you simply look at pipelined kernel execution. Note, you are dealing with a single compute unit (instance of a kernel), as a result at each point, only a single kernel can actually run in the hardware. However, as described above, the run of a kernel also requires the transmission of data to and from the compute unit. These activities should be overlapped to minimize idle-time of the kernel in working with the host application.

Start by compiling and running host code (srcPipeline/host.cpp):

``` bash
make TARGET=hw DEVICE=$AWS_PLATFORM pipeline
```

Again, *$AWS_PLATFORM* is F1 device accelarator Platform (.xpfm) in which the kernel runs. Compared to the kernel compilation time, this will seem like an instantaneous action.

Look at the execution loop in the host code:

``` bash
  // -- Execution -----------------------------------------------------------

  for(unsigned int i=0; i < numBuffers; i++) {
    tasks[i].run(api);
  }
  clFinish(api.getQueue());

```

In this case the code simply schedules all the buffers and lets them execute. Only at the end does it actually synchronize and wait for completion.

Once the build has completed, you can run the host executable via the following commands in sequence:

``` bash
cp auxFiles/sdaccel.ini runPipeline
cd runPipeline
sudo sh
sh-4.2# source /opt/xilinx/xrt/setup.sh
sh-4.2# ./pass ../xclbin/pass.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.awsxclbin
sh-4.2# exit
cd runPipeline
sdx_analyze profile -i sdaccel_profile_summary.csv -f html
sdx_analyze trace sdaccel_timeline_trace.csv -f wdb
sdx -workspace workspace -report *.wdb

```

This script is setup to run the application and then spawn the SDaccel GUI. The GUI will automatically be populated with the collected runtime data.

The runtime data is generated by using the sdaccel.ini file. The content of the sdaccel.ini file is:

``` .ini
[Debug]
profile=true
timeline_trace=true
data_transfer_trace=coarse
stall_trace=all
```

Details of the sdaccel.ini file can be found in the SDAccel Environment User Guide (UG1023).

The Application Timeline viewer illustrates the full run of the executable. The three main sections of the timeline are the OpenCL API Calls, the Data Transfer section, and the Kernel Enqueues. Zooming in on the section illustrating the actual accelerator execution and selecting one of the kernel enqueues, shows an image similar to the following.

![](images/hostcode_opt/OrderedQueue.PNG)

The blue arrows identify dependencies, and you can see that every Write/Execute/Read task execution has a dependency on the previous Write/Execute/Read operation set. This effectively serializes the execution.

Looking back at the execution loop in the host code, no dependency is specified between the Write/Execute/Read runs. Each call to **run** on a specific task only has a dependency on the apiHandle, and is otherwise fully encapsulated. Thus, what creates the dependency?

In this case the dependency is created by using an ordered queue. In the parameter section, the oooQueue parameter is set to false:

``` bash
  bool         oooQueue                 = false;
```

You can break this dependency simply by changing the out of order parameter to true:

``` bash
  bool         oooQueue                 = true;
```

Recompile and execute:

``` bash

make TARGET=hw DEVICE=$AWS_PLATFORM pipeline
cp auxFiles/sdaccel.ini runPipeline
cd runPipeline
sudo sh
sh-4.2# source /opt/xilinx/xrt/setup.sh
sh-4.2# ./pass ../xclbin/pass.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.awsxclbin
sh-4.2# exit
cd runPipeline  #run this if you are not already in the runPipeline directory
sdx_analyze profile -i sdaccel_profile_summary.csv -f html
sdx_analyze trace sdaccel_timeline_trace.csv -f wdb
sdx -workspace workspace -report *.wdb

```

Zooming into the Application Timeline and clicking any kernel enqueue, will result in a figure very similar to the following:

![](images/hostcode_opt/OutOfOrderQueue.PNG)

If you select other pass kernel enqueues, you will see that all 10 of them are now showing dependencies only within the write/execute/read group. This allows the read and write operations to overlap with the execution, and you are effectively pipelining the software write, execute, and read. This can considerably improve overall performance as the communication overhead is happening concurrently with the execution of the accelerator.


### Kernel and Host Code Synchronization

For this step, look at the source code in srcSync (srcSync/host.cpp), and examine the execution loop. This is the same as in the previous section of the tutorial.

``` bash
  // -- Execution -----------------------------------------------------------

  for(unsigned int i=0; i < numBuffers; i++) {
    tasks[i].run(api);
  }
  clFinish(api.getQueue());
```

In this example, the code basically implements a free running pipeline. No synchronization is performed all the way to the end, where a call to clFinish is performed on the event queue. While this creates an effective pipeline, this implementation has an issue related to buffer allocation as well as execution order.

Assume the numBuffer variable is increased to a large number, or might not even be known as would be the case when processing a video stream. In this case sooner or later, buffer allocation and memory usage will become a point of contention. In this example, the host memory is preallocated and shared with the FPGA such that this example will probably run out of memory.

Similarly, as each of the calls to execute the accelerator are independent and un-synchronized (out of order queue), it is quite likely that the order of execution between the different invocations is not aligned with the enqueue order. As a result, if the host code is waiting for a specific block to be finished, this might not happen until much later than expected. This effectively disables any host code parallelism while the accelerator is operating.

To aliviate these issues, OpenCL provides two methods of synchronization, the call to clFinish and the clWaitForEvents call.

First take a closer look at using the clFinish call. To illustrate it's behavior, you must modify the execution loop as follows:

``` bash
  // -- Execution -----------------------------------------------------------

  int count = 0;
  for(unsigned int i=0; i < numBuffers; i++) {
    count++;
    tasks[i].run(api);
    if(count == 3) {
	  count = 0;
	  clFinish(api.getQueue());
    }
  }
  clFinish(api.getQueue());
```

Recompile and execute:

``` bash
make TARGET=hw DEVICE=$AWS_PLATFORM sync
cp auxFiles/sdaccel.ini runSync
cd runSync
sudo sh
sh-4.2# source /opt/xilinx/xrt/setup.sh
sh-4.2# ./pass ../xclbin/pass.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.awsxclbin
sh-4.2# exit
cd runSync  #run this if you are not already in the runPipeline directory
sdx_analyze profile -i sdaccel_profile_summary.csv -f html
sdx_analyze trace sdaccel_timeline_trace.csv -f wdb
sdx -workspace workspace -report *.wdb

```

Then if you zoom into the Application Timeline, a figure quite similar to the one below should be observable.

![](images/hostcode_opt/clFinish.PNG)

The key elements in this figure are the red box named clFinish and the larger gap between the kernel enqueues every three invocations of the accelerator.  

The call to clFinish creates synchronization point on the complete OpenCL command queue. This implies that all commands enqueued onto the given queue will have to be completed before clFinish returns control to the host program. As a result, all activities including the buffer communication will have to be completed before the next set of 3 accelerator invocations can resume. This is effectively a barrier synchonization.

While this enables a synchronization point, where buffers can be released and all processes are guaranteed to have completed, it prevents overlap at the synchronization point.

Now look at an alternative synchonization scheme, where the synchonization is performed based on the completion of a previous excecution of a call to the accelerator. Edit the host.cpp file to change the execution loop as follows:

``` bash
  // -- Execution -----------------------------------------------------------

  for(unsigned int i=0; i < numBuffers; i++) {
    if(i < 3) {
      tasks[i].run(api);
	} else {
	  tasks[i].run(api, tasks[i-3].getDoneEv());
    }
  }
  clFinish(api.getQueue());
```

Recompile and execute:

``` bash

make TARGET=hw DEVICE=$AWS_PLATFORM sync
cp auxFiles/sdaccel.ini runSync
cd runSync
sudo sh
sh-4.2# source /opt/xilinx/xrt/setup.sh
sh-4.2# ./pass ../xclbin/pass.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.awsxclbin
sh-4.2# exit
cd runSync  #run this if you are not already in the runPipeline directory
sdx_analyze profile -i sdaccel_profile_summary.csv -f html
sdx_analyze trace sdaccel_timeline_trace.csv -f wdb
sdx -workspace workspace -report *.wdb

```

Then if you zoom into the Application Timeline, a figure should now be similar to the one below.

![](images/hostcode_opt/clEventSync.PNG)

As you can see, in the later part of the timeline there are five executions of pass executed without any unnecessary gaps. However, even more telling are the data transfers at the point of the marker. At this point, three packages were sent over to be processed by the accelerator, and one was already received back. However, as we have synchronized the next scheduling of write/execute/read on the completion of the first acclerator invocation, we now observe another write operation before any other package is received. This clearly identifies overlapping execution.

Now in this case, you simply synchronized the full next accelerator execution on the completion of the execution scheduled three invocations earlier using the following event synchronization in the run method of the class task.

``` bash
    if(prevEvent != nullptr) {
      clEnqueueMigrateMemObjects(api.getQueue(), 1, &m_inBuffer[0],
				 0, 1, prevEvent, &m_inEv);
    } else {
      clEnqueueMigrateMemObjects(api.getQueue(), 1, &m_inBuffer[0],
				 0, 0, nullptr, &m_inEv);
    }
```

While this is the common synchronization scheme between enqueued objects in OpenCL, it is possible to alternatively synchronize the host code by calling

``` bash
  clWaitForEvents(1,prevEvent);
```

which would allow for additional host code computation while the accelerator is operating on earlier enqueued tasks. This is not further explored here but rather left to the reader as an additional exercise.  


### OpenCL API Buffer Size

In this last section of this tutorial, you will investigate buffer size impact on total performance. Towards that end, you will focus on the host code in srcBuf/host.cpp. The execution loop is exactly the same as at the end of the previous section.

However, in this host code file, the number of tasks to be processed has increased to 100. The goal of this change is to get 100 accelerator calls, transfering 100 buffers and reading 100 buffers. This enables the tool to get a more accurate average throughput estimate per transfer.

In addition, a second command line option (SIZE=) has been added to specify the buffer size for a specific run. The actual buffer size to be transfered during a single write or read is determined by calculating 2 to the power of the specified argument (pow(2, argument)) multiplied by 512 bits.

You can compile the host code by calling:

``` bash
make TARGET=hw DEVICE=$AWS_PLATFORM buf
```

and run the executable with the following command sequence:

``` bash
cp auxFiles/sdaccel.ini runBuf
cd runBuf
sudo sh
sh-4.2# source /opt/xilinx/xrt/setup.sh
sh-4.2# ./pass ../xclbin/pass.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.awsxclbin 14
sh-4.2# exit

```

14 is used as second argument to the host code executable pass. This allows the code to execute the implementation with different buffer sizes and measure throughput by monitoring the total compute time. This number is calculated in the Testbench and reported via the FPGA Throughput output.

To ease this sweeping of the different buffer sizes, a python script called run.py was created and can be executed via the following command sequence:

``` bash

cp auxFiles/sdaccel.ini runBuf
cp auxFiles/run.py runBuf
cd runBuf
sudo sh
sh-4.2# source /opt/xilinx/xrt/setup.sh
sh-4.2# ./run.py
sh-4.2# exit
more results.csv

# if you have gnuplot installed you could plot this data
gnuplot -p -c auxFiles/plot.txt

```

Note, the sweeping script, [hostcode_opt/auxFiles/run.py](hostcode_opt/auxFiles/run.py) requires a python installation which is available in most systems. Executing the sweep, will run and record the FPGA Throughput for buffer size arguments of 8 to 19. The measured throughput values are recorded together with the actual number of bytes per transfer in the runBuf/results.csv file which is printed at the end of the makefile execution. When analyzing these numbers, a step function similar to the the following image should be observable:

![](images/hostcode_opt/stepFunc.PNG)

This image shows that the buffer size clearly impacts performance and starts to level out around 2 MBytes. Note, this image is created via gnuplot from the results.csv file and if found on your system will be displayed automatically after you run the sweep.

From a host code performance point of view, this step function clearly identifies a relationship between buffer size and total execution speed. As shown in this example, it is easy to take an algorithm and alter the buffer size when the default implementation is based on small amount of input data. It doesn't have to be dynamic and runtime deterministic as performed here but the principle remains the same. Instead of transmitting a single value set for one invocation of the algorithm you simply transmit multiple input values and repreat the algorithm execution on a single invocation of the accelerator.

### Conclusion

This tutorial illustrated three specific areas of host code optimization, namely

   * Pipelined Kernel Execution using an Out of Order Event Queue
   * Kernel and Host Code Synchronization
   * OpenCL API Buffer Size

It should now be clear that these areas need to be considered when trying to create an efficient acceleration implmentation. The tutorial shows how these performance bottlenecks can be analyzed and shows one way of how they can be improved.

Please note, there are many ways to implement your host code, and improve performance in general. This applies to improving host to accelerator performance, as well and other areas such as buffer management will need to be considered. Thus this tutorial does not claim to be complete with respect to host code optimization.

Please have a closer look at SDAccel Profiling and Optimization Guide (UG1207) which details the tools and processes to analyze the application performance in general, and also lists some common pitfalls.

<p align="center"><b>
Start the next module: <a href="WRAP_UP.md">4. Wrap-up and Next Steps</a>
</b></p>
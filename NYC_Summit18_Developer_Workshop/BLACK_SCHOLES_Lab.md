
<table style="width:100%">
  <tr>
    <th width="100%" colspan="5"><h2>AWS Summit NYC 2018 Developer Workshop</h2></th>
  </tr>
  <tr>
    <td width="20%" align="center"><a href="README.md">Introduction</a></td>
    <td width="20%" align="center"><a href="SETUP.md">1. Connecting to your F1 instance</a></td> 
    <td width="20%" align="center"><b>2. Experiencing F1 acceleration</b></td>
    <td width="20%" align="center"><a href="IDCT_Lab.md">3. Developing F1 applications</a></td>
    <td width="20%" align="center"><a href="WRAP_UP.md">4. Wrapping-up</td>
  </tr>
</table>

---------------------------------------

### Experiencing F1 Acceleration

In this module you will experience the acceleration potential of AWS F1 instances by using ```black-scholes``` to do financial risk analysis by simulating pricing options. 


The ```black-scholes``` uses two styles of stock transaction options which are considered the European vanilla option and Asian option (which is one of the exotic options). Call options and put options are defined reciprocally. Given the basic parameters for an option, namely expiration date and strike price, the call/put payoff price are estimated.  The model parameters are specified in a file (in protobuf form) called ```blackEuro.parameters``` and ```blackAsian.parameters``` respectively. The meaning of the parameters are as follows.

Parameter |  Meaning 
:-------- | :---
time      |  time period 
rate       |  interest rate of riskless asset 
volatility|  volatility of the risky asset 
initprice	 |  initial price of the stock 
strikeprice       |  strike price for the option 


#### Setting-up the workshop

1. Open a new terminal by right-clicking anywhere in the Desktop area and selecting **Open Terminal**. 
1. Navigate to the FinancialModels_AmazonF1 workshop directory.
    ```bash
    cd ~/aws-fpga
    source sdaccel_setup.sh
    source $XILINX_SDX/settings64.sh 
    export COMMON_REPO=$SDACCEL_DIR/examples/xilinx/
    cd ~/FinancialModels_AmazonF1/blackScholes_model/europeanOption/
    ```

#### Step 1: Running the simulator on the CPU 

1. Run simulation models on the CPU.
    ```bash
    make TARGETS=hw DEVICES=$AWS_PLATFORM PLATFORM=xilinx_aws-vu9p-f1_4ddr-xpr-2pr-4_0 pure_c
    ./blackeuro_c
    ```

    The simulator will finish with a message similar to this one:
    ```
    Starting execution. Time=1 rate=0.05 volatility =0.2 initprice=100 strikeprice=110 
    Execution completed
    Execution time 122.4 s
    the call price is: 6.03988
    the put price is: 10.6755
    ```
    
1. Repeat the previous step for the asianOption
    ```bash
    cd ~/FinancialModels_AmazonF1/blackScholes_model/asianOption/
    ```
 
#### Step 2: Running the simulator on the F1 FPGA 


1. Source the SDAccel runtime environment.
    ```bash
    sudo sh
    source /opt/Xilinx/SDx/2017.1.rte/setup.sh
    ```

1. Run the simulation model on the F1 FPGA.
    ```bash
    ./blackeuro 
    ```

    The simulation will finish with a message similar to this one: 
    ```
    Starting execution. Time=1 rate=0.05 volatility =0.2 initprice=100 strikeprice=110
    Execution completed
    Execution time 0.18 s
    the call price is: 6.03991
    the put price is: 10.6754
    ```   
1. Repeat the previous step for the asianOption



#### Step 3: Comparing performance 

1. The table below summarizes the performance on Amazon F1 FPGA

Target frequency is 250MHz. 
Target device is 'xcvu9p-flgb2104-2-i'

| Model | Option | N. random number generators | N. simulations | N. simulation groups | N. steps   | Time C5 CPU [s] | Time F1 CPU [s] | Time F1 FPGA [s] | LUT | LUTMem | REG | BRAM | DSP | 
|-|-|-|-|-|-|-|-|-|-|-|-|-|-|
| Black-Scholes | European option  |64|512| 1024|  1 | 125 |114|0.23|31% |2%|15% |26% |43%|
| Black-Scholes | Asian option     |64|512|65536|256 | 376 |497|0.83|31% |2%|16% |26% |43%|
| Heston | European option         |32|512|  512|256 | 226 |330|1.52|18% |2%| 9% |11% |26%|
| Heston | European barrier option |32|512|  512|256 | 32 | 40|0.75|18% |2%| 9% |11% |26%|

1. Close your terminal to conclude this module.
    ```bash
    exit
    exit
    ```

#### Conclusion

AWS F1 instances with Xilinx FPGAs can provide significant performance improvements over CPUs. Financial risk simulations running on F1 are significantly faster than running on the CPU. 

Multiple instances of the black-scholes simulation could be loaded in the FPGA, allowing parallel processing of multiple  simulations and easily delivering an increase in performance/$ over a CPU-based solution. 

In addition to financial risk analysis, F1 instances are very well suited to accelerate compute intensive workloads such as: genomics, big data analytics, security or machine learning.

Now that you have experienced the performance potential of AWS F1 instances, the next lab will introduce you to the SDAccel IDE and how to develop, profile and optimize an F1 application.

---------------------------------------

<p align="center"><b>
Start the next module: <a href="IDCT_Lab.md">3. Developing, profiling and optimizing F1 applications with SDAccel</a>
</b></p>


<table style="width:100%">
  <tr>
    <th width="100%" colspan="5"><h2>AWS Summit NYC 2018 Developer Workshop</h2></th>
  </tr>
  <tr>
    <td width="20%" align="center"><a href="README.md">Introduction</a></td>
    <td width="20%" align="center"><b>1. Connecting to your F1 instance</b></td>
    <td width="20%" align="center"><a href="BLACK_SCHOLES_Lab.md">2. Experiencing F1 acceleration</a></td>
    <td width="20%" align="center"><a href="IDCT_Lab.md">3. Developing F1 applications</a></td>
    <td width="20%" align="center"><a href="WRAP_UP.md">4. Wrapping-up</td>
  </tr>
</table>

---------------------------------------
### Connecting to your F1 instance

In this module you will start an EC2 F1 instance using the FPGA Developer AMI and connect to it using a remote desktop client. Once connected, you will download the workshop files and confirm you can execute a simple application on F1.

For this event, each registered participant will be required to start an EC2 F1 instance, therefore, participants need to have experience with:
- AWS account and launching EC2 instances
- EC2 Key Pair (.pem)
- EC2 Security Groups
- IAM username and credentials
- Setting up AWS CLI
- Connecting to a running instance using SSH or PuTTy

#### Start a EC2 F1 instance based FPGA developer AMI

1. Sign into your AWS account and go to EC2

1. Select Region **US-East (N. Virginia)**.  The pull down is located in the upper right corner of the console.

1. Launch an F1 Instance (f1.2xlarge) using the 1.6.0 version of the [FPGA developer AMI](https://aws.amazon.com/marketplace/pp/B06VVYBLZZ) from the EC2 Console.

    - Use Manual launch
    ![Remote](./images/setup_lab/manual_launch.png?raw=true)
    
    - Select launch with EC2 console
    ![Remote](./images/setup_lab/launch_with_console.png?raw=true)

    - Configure the root volume(/dev/sda1) to be 100GB
    ![Remote](./images/setup_lab/root_volume_storage.png?raw=true)
    
    - Setup your security groups to allow RDP and SSH ingress from your IP or from anywhere depending on your security preferences.
    ![Remote](./images/setup_lab/create_security_group.png?raw=true)
    
1. Once the instance is running, find and note the **IPv4 Public IP** address of your instance.
    - You will be using this IP address to connect to your instance.
    - The **IPv4 Public IP** address is displayed in EC2 Console next to the instance status.
    
**Some AWS accounts may have a F1 instance limit equal to 0.  If your F1 instance fails to launch due to your account has an instance limit equal to 0:  During the workshop, raise your hand for assistance so we can manually raise the limit on your account.  After the workshop, use [AWS support to submit for a F1 instance limit increase](http://docs.aws.amazon.com/AWSEC2/latest/UserGuide/ec2-resource-limits.html).**
    
#### Connect to your instance using SSH

**[Windows Users - Connecting to Your Linux Instance from Windows Using PuTTY](http://docs.aws.amazon.com/AWSEC2/latest/UserGuide/putty.html)**

1. In the SSH client, use the **IPv4 Public IP** of your instance:

   ```bash
    ssh -i <.pem file> centos@<IPv4 Public IP>
    ```

1. Run a setup script to configure GUI and download workshop files.

    ```bash
    source <(curl -s https://s3.amazonaws.com/aws-ec2-f1-reinvent-17/setup_script/setup_nycsummit18.sh)
    ```

    This setup script performs the following tasks:
    - Download and setup the aws-fpga repository in (/home/centos).
    - Download and setup the aws-fpga-app-notes repository in (/home/centos).
    - Install packages to setup a GUI Desktop on the FPGA Developer AMI.
    - Set a user password for the 'centos' user.
    - Download and setup the FinancialModels_AmazonF1 repository in (/home/centos)
    - **IMPORTANT: Please take a note of the password it generates at the end. You will need to use it to connect using RDP.**
    - The setup script is a one time setup and completes in 8-15 mins.  Feel free to use this time to read the workshop.

#### Connect to your instance using a remote desktop client

The instance you just started should now be configured with remote desktop protocol (RDP) services.

1. From your local machine, start a remote desktop protocol client
    - On Windows: press the Windows key and type "remote desktop".
      - You should see the "Remote Desktop Connection" in the list of programs.
      - Alternatively you can also simply type `mstsc.exe` in the Windows run prompt.
    - On Linux: RDP clients such a Remmina or Vinagre are suitable.
    - On macOS: use the Microsoft Remote Desktop from the Mac App Store.

1. In the RDP client, enter the **IPv4 Public IP** of your instance.

1. **IMPORTANT**: Before connecting, set your remote desktop client to use **24-bit or less for color depth**
    - On Windows: In the bottom-left corner of connection prompt, click Options, then select the Display tab and set Colors to True Colors (24 bit)

1. Click **Connect**. This should bring up a message about connection certificates.

1. Click **Yes** to dismiss the message. The Remote Desktop Connection window opens with a login prompt.

1. Login with the following credentials:
    - User: **centos**
    - Password: ******** _(Generated as part of the setup script)_
    - Look for output like below:
        ```bash
        INFO: Setting password for user 'centos'
        INFO: **************************************
        INFO: *** PASSWORD : reinvent2017_123   ****
        INFO: **************************************
        ```

    ![Remote](./images/setup_lab/remote1.png?raw=true)

1. Click **Ok**.

You should now be connected to the remote F1 instance running Centos 7.

#### Open the workshop instructions on the remote F1 instance

1. In the remote instance, double click on the **Chromium Web Browser** icon.
    - It opens the browser and loads the workshop instructions.

    _Note: if a "keyring" popup comes up, click Cancel._
    

1. Continue following the workshop instructions from within the remote instance and the Chromium browser.
    - We suggest you perform all your copy-paste from the instructions to the shell within the RDP session to avoid issues.

#### Configure the Xilinx SDAccel environment and load the workshop files

1. Open a new terminal by right-clicking anywhere in the Desktop area and selecting **Open Terminal**.

1. In the terminal, verify that the reInvent17_Developer_Workshop within the aws-fpga-apps-notes repository exists and that the aws-fpga repository exists.

    ```bash
    ls -lrtha ~/aws-fpga-app-notes
    ls -lrtha ~/aws-fpga
    ```

1. Source the SDAccel environment from the aws-fpga repository.

    ```bash
    cd ~/aws-fpga
    source sdaccel_setup.sh
    source $XILINX_SDX/settings64.sh
    export COMMON_REPO=$SDACCEL_DIR/examples/xilinx/
    ```

    *Note: the sdaccel_setup.sh script might generate warning messages, but these can be safely ignored.*


#### Run the hello_world example to validate the setup of your F1 instance

The hello world example is an OpenCL application with a simple vector-addition accelerator. This example uses a precompiled FPGA binary to reduce compilation time and streamline the workshop.

1.  Compile the host application

    ```bash
    # Go to the example directory
    cd ~/aws-fpga-app-notes/NYC_Summit18_Developer_Workshop/helloworld_ocl

    # Compile the host application (./helloworld)
    make TARGETS=hw DEVICES=$AWS_PLATFORM exe
    ```

1. Confirm the presence of the precompiled FPGA binary.

    ```bash
    ls -la ./xclbin/vector_addition.hw.xilinx_aws-vu9p-f1_4ddr-xpr-2pr_4_0.awsxclbin
    ```

1. Execute the host application with the precompiled FPGA binary on the F1 instance.

    ```bash
    sudo sh
    source /opt/Xilinx/SDx/2017.1.rte/setup.sh
    ./helloworld
    ```

1. The host application executes using the vector_addition kernel running in the FPGA and produces the following results:

    ```shell
    Device/Slot[0] (/dev/xdma0, 0:0:1d.0)
    xclProbe found 1 FPGA slots with XDMA driver running
    platform Name: Xilinx
    Vendor Name : Xilinx
    Found Platform
    Found Device=xilinx:aws-vu9p-f1:4ddr-xpr-2pr:4.0
    XCLBIN File Name: vector_addition
    INFO: Importing ./vector_addition.hw.xilinx_aws-vu9p-f1_4ddr-xpr-2pr_4_0.awsxclbin
    Loading: './vector_addition.hw.xilinx_aws-vu9p-f1_4ddr-xpr-2pr_4_0.awsxclbin'
    Result =
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    42 42 42 42 42 42 42 42 42 42 42 42 42 42 42 42
    TEST PASSED
    sh-4.2#
    ```

1. You compiled a host application and successfully executed it on F1 using a pre-compiled Amazon FPGA Image (AFI).

1. Close your terminal.

    ```bash
    exit
    exit
    ```

This concludes this first module.

---------------------------------------

<p align="center"><b>
Start the next module: <a href="BLACK_SCHOLES_Lab.md">2: Experiencing F1 acceleration</a>
</b></p>

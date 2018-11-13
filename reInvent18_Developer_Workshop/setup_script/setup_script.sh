#!/bin/bash

CENTOS_HOME=/home/centos
CENTOS_PWD=reinvent2017_
CENTOS_PWD+=$((RANDOM%1000))

pushd $CENTOS_HOME

function setup_password {
    info_msg "Setting password for user 'centos'"
    echo "centos:$CENTOS_PWD" | sudo chpasswd

    info_msg "**************************************"
    info_msg "*** PASSWORD : ${CENTOS_PWD}   ****"
    info_msg "**************************************"
}

# Please configure CLI at this point.

function setup_app_notes_repo {
    info_msg "Cloning the aws-fpga-app-notes repository"

    if [ -e ${CENTOS_HOME}/aws-fpga-app-notes ]; then
        info_msg "app-notes repo exists. Deleting first before cloning again."
        rm -rf ${CENTOS_HOME}/aws-fpga-app-notes
    fi

    git clone https://github.com/awslabs/aws-fpga-app-notes.git

}

function setup_aws_fpga_repo {
    info_msg "Fetching the aws-fpga repository"

    if [ -e ${CENTOS_HOME}/aws-fpga ]; then
        info_msg "aws-fpga repo exists. Deleting first before cloning again."
        rm -rf ${CENTOS_HOME}/aws-fpga
        info_msg "Fetching the aws-fpga repository"
    fi

    pushd ${CENTOS_HOME}

    curl -s https://s3.amazonaws.com/aws-ec2-f1-reinvent-17/github_repo/aws-fpga.tar.gz -o aws-fpga.tar.gz
    tar -xzf aws-fpga.tar.gz
    rm aws-fpga.tar.gz

    popd
}

function setup_gui {
    info_msg "Setting up the GUI packages"

    sudo yum install -y kernel-devel # Needed to re-build ENA driver
    sudo yum groupinstall -y "Server with GUI"
    sudo systemctl set-default graphical.target

    sudo yum -y install epel-release
    sudo rpm -Uvh http://li.nux.ro/download/nux/dextop/el7/x86_64/nux-dextop-release-0-5.el7.nux.noarch.rpm
    sudo yum install -y xrdp tigervnc-server
    sudo yum install -y chromium
    sudo systemctl start xrdp
    sudo systemctl enable xrdp
    sudo systemctl disable firewalld
    sudo systemctl stop firewalld
}

function setup_chromium {
    info_msg "Setting up the Chromium browser"

    if [ ! -e ${CENTOS_HOME}/Desktop ]; then
        mkdir ${CENTOS_HOME}/Desktop
    fi

    pushd ${CENTOS_HOME}/Desktop
    curl -s https://s3.amazonaws.com/aws-ec2-f1-reinvent-17/setup_script/chromium-browser.desktop -o chromium-browser.desktop
    chmod +x chromium-browser.desktop
    popd

    pushd /etc/chromium

    if [ ! -e master_preferences ]; then
        rm master_preferences
    fi

    sudo curl -s https://s3.amazonaws.com/aws-ec2-f1-reinvent-17/setup_script/developer_workshop_setup.json -o master_preferences
    sudo chown root:root master_preferences

    popd
}

function fetch_vectors {
    info_msg "Fetching example vectors"

    VECTOR_DIR=$CENTOS_HOME/vectors
    if [ ! -e $VECTOR_DIR ]; then
        mkdir -p $VECTOR_DIR
    fi

    pushd $VECTOR_DIR
    wget -N https://s3.amazonaws.com/aws-ec2-f1-reinvent-17/data_files/vectors/crowd8_420_1920x1080_50.yuv
    popd
}

function check_available_space {
    info_msg "Checking available free space"

    FREE=`df -k --output=avail "${CENTOS_HOME}" | tail -n1`
    if [[ ${FREE} -lt 5242880 ]]; then
        err_msg "Less than 5GB space available on root volume. Please start another instance with > 75GB as root volume and re-run"
        return 1
    else
        info_msg "More than 5GB space available. Processing next steps"
    fi
}

function info_msg {
  echo -e "INFO: $1"
}

function err_msg {
  echo -e >&2 "ERROR: $1"
}

check_available_space
setup_gui
setup_chromium
setup_aws_fpga_repo
setup_app_notes_repo
fetch_vectors
setup_password

popd

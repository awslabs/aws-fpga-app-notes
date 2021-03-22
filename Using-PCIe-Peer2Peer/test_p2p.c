/*


 * Amazon FPGA Hardware Development Kit
 *
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Amazon Software License (the "License"). You may not use
 * this file except in compliance with the License. A copy of the License is
 * located at
 *
 *    http://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
 * implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/mman.h>
#include <getopt.h>

#include "fpga_pci.h"
#include "fpga_mgmt.h"
#include "fpga_dma.h"
#include "utils/lcd.h"
#include "test_p2p_common.h"

#define MAX_SLOTS 8
#define MAX_INST 1
#define MAP_SIZE 65536UL

#define WR_INST_OFFSET 0x1c
#define RD_INST_OFFSET 0x3c

#define ATG_CONFIG      0x0
#define ATG_INST_CNT    0x10
#define ATG_RD_OUTSTD   0x14
#define WR_RD_GO        0x8
#define ATG_EN          0x30
#define ATG_STATUS      0xc
#define ERR_STATUS      0xb0
#define ERR_BRESP       0xd0
#define ERR_RRESP       0xd8


#define CONT_MODE       0x0 << 0
#define INCR_DATA       0x1 << 1
#define READ_COMPARE    0x3 << 3 


#define DOMAIN 0
#define BUS 0
#define FUNCTION 0
#define DDR_BAR 3
#define OCL_BAR 0

/* use the standard out logger */
static const struct logger *logger = &logger_stdout;

void usage(const char* program_name);
int p2p_example(int num_slot, int GROUP);
int p2p_write(pci_bar_handle_t pci_bar_handle, uint64_t address, uint32_t test_pattern);
int p2p_read_compare(pci_bar_handle_t pci_bar_handle, uint64_t address, uint32_t test_pattern);
void write_instruction(int inst, uint64_t address, uint32_t data, int length);
void read_instruction(int inst, uint64_t address, uint32_t data, int length);
uint64_t get_128GBar_address(char *dir_name);


int main(int argc, char **argv) {
    int rc;
    int num_slot;
    int GROUP;
    char *inst_type = "";

    static struct option long_options[] = {
        {"inst_type",         required_argument,  0, 'I' },
        {0, 0, 0, 0},
    };

    int opt;
    int long_index = 0;
    while ((opt = getopt_long(argc, argv, "I:",
                long_options, &long_index)) != -1) {
      switch (opt) {
      case 'I':
        inst_type = optarg;
        break;
      default:
       printf("unrecognized option: %s", argv[optind-1]);
       return -1;
      }
    }
   

    /* setup logging to print to stdout */
    rc = log_init("test_p2p");
    fail_on(rc, out, "Unable to initialize the log.");
    rc = log_attach(logger, NULL, 0);
    fail_on(rc, out, "%s", "Unable to attach to the log.");

    /* initialize the fpga_plat library */
    rc = fpga_mgmt_init();
    fail_on(rc, out, "Unable to initialize the fpga_mgmt library");


    /* run the p2p test example */
    if (strcmp(inst_type,"4xl") == 0) {
       log_info("Instance type selected: 4xl, number of slots: 2, Group: 1 \n"); 		    
       num_slot = 2;
       GROUP = 1;

       /* check that the AFI is loaded */
       log_info("Checking to see if the right AFI is loaded...");
       for (int slot_id=0; slot_id < num_slot; slot_id++) {
          rc = check_slot_config(slot_id);
          fail_on(rc, out, "slot config is not correct");
       }
    } else if (strcmp(inst_type, "16xl") == 0) {
       log_info("Instance type selected: 16xl, number of slots: 8, Group: 2 \n"); 		    
       num_slot = 8;
       GROUP = 2;

       /* check that the AFI is loaded */
       log_info("Checking to see if the right AFI is loaded...");
       for (int slot_id=0; slot_id < num_slot; slot_id++) {
          rc = check_slot_config(slot_id);
          fail_on(rc, out, "slot config is not correct");
       }
    }   
    rc = p2p_example(num_slot, GROUP);
    fail_on(rc, out, "p2p example failed");
out:
    log_info("TEST %s", (rc == 0) ? "PASSED" : "FAILED");
    return rc;
}

void usage(const char* program_name) {
    printf("usage: %s -I <instance_type> \n", program_name);
}


/**
 * This example demonstrates PCIe P2P write and read transfers
 */
int p2p_example(int num_slot, int GROUP) {
   
    int rc;

    int pf_id = 0;
    int bar_id = 0;
    int tgt_slot_id;
    int slot_id;
    uint32_t test_pattern = 0xf1f1f1f1;
    uint64_t address[num_slot];
    char *resource_name[8] = {};
    pci_bar_handle_t pci_bar_handle[num_slot];
    
    /* pci_bar_handle_t is a handler for an address space exposed by one PCI BAR on one of the PCI PFs of the FPGA */
    if (num_slot == 2) {
       resource_name[0] = "0000:00:1b.0";
       resource_name[1] = "0000:00:1d.0";
    } else if (num_slot == 8) {
       resource_name[0] = "0000:00:0f.0";
       resource_name[1] = "0000:00:11.0";
       resource_name[2] = "0000:00:13.0";
       resource_name[3] = "0000:00:15.0";
       resource_name[4] = "0000:00:17.0";
       resource_name[5] = "0000:00:19.0";
       resource_name[6] = "0000:00:1b.0";
       resource_name[7] = "0000:00:1d.0";
    }   

    for (int slot_id=0; slot_id < num_slot; slot_id++) {
       pci_bar_handle[slot_id] = PCI_BAR_HANDLE_INIT;

       bar_id =0;
       /* Attaching to OCL Bar */
       rc = fpga_pci_attach(slot_id, pf_id, bar_id, 0, &pci_bar_handle[slot_id]);
       fail_on(rc, out, "Unable to attach to the OCL Bar AFI on slot id %d", slot_id);

       printf ("Getting 128G Bar address for slot %d", slot_id);
       address[slot_id] = get_128GBar_address(resource_name[slot_id]);
    }

    log_info ("INFO: Starting the P2P test ");
    // Looping on Group0 slots (0,1,2,3)
    for (int Group_id=0; Group_id < GROUP; Group_id++) {
       log_info("INFO: Starting the Peer-2-Peer transfers in between Group%d", Group_id);
       for (int slot=0; slot < (num_slot/GROUP); slot++) {
          slot_id = Group_id*4 + slot;
          tgt_slot_id = Group_id*4 + (slot+1);
          if (((num_slot == 8) & ((slot_id == 3) | (slot_id == 7))) | ((num_slot == 2) & (slot_id == 1))) {
             tgt_slot_id = Group_id*4 + (0);
          }
             log_info("INFO: Starting P2P transfer from slot %d to slot%d Address %lx", slot_id, tgt_slot_id, address[tgt_slot_id]);
             rc = p2p_write(pci_bar_handle[slot_id], address[tgt_slot_id], test_pattern);
             sleep(1);
             rc = p2p_read_compare(pci_bar_handle[slot_id], address[tgt_slot_id], test_pattern);
             fail_on(rc, out, "Read Compare failed for the ATG");
       }
    }

out:
    /* if there is an error code, exit with status 1 */
    return (rc != 0 ? 1 : 0);
}

int p2p_write(pci_bar_handle_t pci_bar_handle, uint64_t address, uint32_t test_pattern) 
{
    int length = 0x0;
    int rc;
    uint32_t addr_low, addr_high;
    uint32_t read_data; 
    uint64_t inst_start_addr;
    uint64_t base_addr_scale;

    rc = fpga_pci_poke(pci_bar_handle, WR_RD_GO, 0x0);

    /* Writing a single write instruction before doing a Read*/
    log_info ("INFO: Writing write instructions for write transfers ");
    inst_start_addr = address;
    for(int inst=0; inst<MAX_INST; inst++) {
        addr_high = inst_start_addr >> 32;
        addr_low  = inst_start_addr & 0xffffffff;

        log_info("INFO: Write Instruction: index %d Address %"PRIx64" Data %x length %d ", inst, inst_start_addr, test_pattern, length);

        fpga_pci_poke(pci_bar_handle, WR_INST_OFFSET + 0x0, inst);
        fpga_pci_poke(pci_bar_handle, WR_INST_OFFSET + 0x4, addr_low);
        fpga_pci_poke(pci_bar_handle, WR_INST_OFFSET + 0x8, addr_high);
        fpga_pci_poke(pci_bar_handle, WR_INST_OFFSET + 0xC, test_pattern);
        fpga_pci_poke(pci_bar_handle, WR_INST_OFFSET + 0x10, length);

        base_addr_scale = ((length >> 2) * 0x100) + 0x100;
        inst_start_addr = inst_start_addr + base_addr_scale;
    }

    /* configure ATG */
    log_info ("INFO: Configuring the ATG ");
    rc = fpga_pci_poke(pci_bar_handle, 0x0, CONT_MODE| INCR_DATA);

    read_data = (((MAX_INST -1)<<16) + ((MAX_INST-1)));
    rc = fpga_pci_poke(pci_bar_handle, ATG_INST_CNT, read_data);

    /* Starting the ATG */
    log_info ("INFO: Starting the ATG to do write only transfers");
    rc = fpga_pci_poke(pci_bar_handle, WR_RD_GO, 0x1);

    return rc;
}

int p2p_read_compare(pci_bar_handle_t pci_bar_handle, uint64_t address, uint32_t test_pattern)
{
    int no_of_outstd = 0x1f;
    int length = 0x0;
    int rc;
    uint32_t addr_low, addr_high;
    uint32_t read_data; 
    uint32_t bresp_data, rresp_data; 
    uint64_t inst_start_addr;
    uint64_t base_addr_scale;

    /* Writing single read instruction to get the latency data */
    log_info ("INFO: Writing read instructions for read transfers ");
    inst_start_addr = address;
    for(int inst=0; inst<MAX_INST; inst++) {
        addr_high = inst_start_addr >> 32;
        addr_low  = inst_start_addr & 0xffffffff;

        log_info("INFO: Read Instruction: index %d Address %"PRIx64" Data %x length %d ", inst, inst_start_addr, test_pattern, length);
        fpga_pci_poke(pci_bar_handle, RD_INST_OFFSET + 0x0, inst);
        fpga_pci_poke(pci_bar_handle, RD_INST_OFFSET + 0x4, addr_low);
        fpga_pci_poke(pci_bar_handle, RD_INST_OFFSET + 0x8, addr_high);
        fpga_pci_poke(pci_bar_handle, RD_INST_OFFSET + 0xC, test_pattern);
        fpga_pci_poke(pci_bar_handle, RD_INST_OFFSET + 0x10, length);

        base_addr_scale = ((length >> 2) * 0x100) + 0x100;
        inst_start_addr = inst_start_addr + base_addr_scale;
    }

    /* write a read outstanding on 1 */
    rc = fpga_pci_poke(pci_bar_handle, ATG_RD_OUTSTD, no_of_outstd);

    /* configure ATG */
    log_info ("INFO: Configuring the ATG ");
    rc = fpga_pci_poke(pci_bar_handle, 0x0, CONT_MODE| INCR_DATA | READ_COMPARE);

    read_data = (((MAX_INST -1)<<16) + ((MAX_INST-1)));
    rc = fpga_pci_poke(pci_bar_handle, ATG_INST_CNT, read_data);

    /* Starting the ATG */
    log_info ("INFO: Starting the ATG to do read only transfers");
    rc = fpga_pci_poke(pci_bar_handle, WR_RD_GO, 0x2);

    sleep(1);
    rc = fpga_pci_poke(pci_bar_handle, WR_RD_GO, 0x0);

    /* Checking Error Status Register */
    log_info ("INFO: Checking Read Error Status Register ");
    fpga_pci_peek(pci_bar_handle, ERR_STATUS, &read_data);
    fpga_pci_peek(pci_bar_handle, ERR_BRESP, &bresp_data);
    fpga_pci_peek(pci_bar_handle, ERR_RRESP, &rresp_data);
    log_info ("INFO: Read Error status Register Value is %x ", read_data);
    log_info ("INFO: Read Bresp Error Register Value is %x ", bresp_data);
    log_info ("INFO: Read Rresp Error Register Value is %x ", rresp_data);
    if ((read_data != 0) | (bresp_data != 0) | (rresp_data != 0)) {
       rc = 1;
    }
    return rc;
}

uint64_t get_128GBar_address(char *dir_name)
{
        int ret;
        uint64_t physical_addr;
        fail_on(!dir_name, err, "dir_name is null");

        char sysfs_name[256];
        ret = snprintf(sysfs_name, sizeof(sysfs_name),
                        "/sys/bus/pci/devices/%s/resource", dir_name);
        fail_on(ret < 0, err, "Error building the sysfs path for resource");
        fail_on((size_t) ret >= sizeof(sysfs_name), err,
                        "sysfs path too long for resource");

        FILE *fp = fopen(sysfs_name, "r");
        fail_on_quiet(!fp, err, "Error opening %s", sysfs_name);

        for (size_t i = 0; i < FPGA_BAR_PER_PF_MAX; ++i) {
                uint64_t addr_begin = 0, addr_end = 0, flags = 0;
                ret = fscanf(fp, "0x%lx 0x%lx 0x%lx\n", &addr_begin, &addr_end, &flags);
                if (ret < 3 || addr_begin == 0) {
                        continue;
                }
                if (i == 4) {
                  physical_addr = addr_begin;
                }
        }

        fclose(fp);
        return physical_addr;
err:
        errno = 0;
        return FPGA_ERR_FAIL;
}







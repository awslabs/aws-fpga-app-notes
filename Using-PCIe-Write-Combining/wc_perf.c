// Amazon FPGA Hardware Development Kit
//
// Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License"). You may not use
// this file except in compliance with the License. A copy of the License is
// located at
//
//    http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
// implied. See the License for the specific language governing permissions and
// limitations under the License.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include <fpga_pci.h>
#include <fpga_mgmt.h>
#include <utils/lcd.h>

/*
 * pci_vendor_id and pci_device_id values below are Amazon's and avaliable to use for a given FPGA slot. 
 * Users may replace these with their own if allocated to them by PCI SIG
 */
static uint16_t pci_vendor_id = 0x1D0F; /* Amazon PCI Vendor ID */
static uint16_t pci_device_id = 0xF001; /* PCI Device ID preassigned by Amazon for F1 applications */


/* use the stdout logger for printing debug information  */
const struct logger *logger = &logger_stdout;

/* Declaring the local functions */

int wc_perf(int slot, int pf_id, int bar_id);

/* Declaring auxilary house keeping functions */
int initialize_log(char* log_name);
int check_afi_ready(int slot);

int num_of_uints = 16;
int num_of_passes = 100;
int write_combine = 0;
int use_custom = 0;
int verbose = 0;

/* Subtract timespec t2 from t1 */
static void compute_delta(struct timespec *t1, const struct timespec *t2)
{

  t1->tv_sec -= t2->tv_sec;
  t1->tv_nsec -= t2->tv_nsec;
  
  if (t1->tv_nsec >= 1000000000l) {
    t1->tv_sec++;
    t1->tv_nsec -= 1000000000l;
  } else if (t1->tv_nsec < 0l) {
    t1->tv_sec--;
    t1->tv_nsec += 1000000000l;
  }
  
}

void print_usage() {
  printf("SYNOPSIS\n\twc_perf [options] [-h]\n");
  printf("Example: wc_perf -i 1024 -w\n");
  printf("DESCRIPTION\n\tWrites bytes to AppPF BAR4 and reports the bandwidth in GB/s\n");
  printf("OPTIONS\n");
  printf("\t-i num - Maximum number of integers to move.\n");
  printf("\t-p num - Maximum number of passes for measurement.\n");
  printf("\t-w     - Use WriteCombine region.\n");
  printf("\t-c     - Use custom memory move.\n");
  printf("\t-v     - Enable verbose mode.\n");
}

int main(int argc, char **argv) {
  int rc;
  int slot_id;
  char c;
  
  /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
  rc = fpga_pci_init();
  fail_on(rc, out, "Unable to initialize the fpga_pci library");
  
  /* This demo works with single FPGA slot, we pick slot #0 as it works for both f1.2xl and f1.16xl */
  
  slot_id = 0;
  
  rc = check_afi_ready(slot_id);
  fail_on(rc, out, "AFI not ready");
  
  while ((c = getopt(argc, argv, "hvcwi:p:")) != -1)
    switch (c) {
    case 'i':
      num_of_uints = atoi(optarg);
      break;
    case 'p':
      num_of_passes = atoi(optarg);
      break;
    case 'w':
      write_combine = BURST_CAPABLE;
      break;
    case 'c':
      use_custom = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'h':
      print_usage();
      exit(0);
      break;
    default:
      exit(1);
      break;
    }
  
  printf("===== Starting WC Benchmark Test =====\n");	
  rc = wc_perf(slot_id, FPGA_APP_PF, APP_PF_BAR4);
  fail_on(rc, out, "WC Benchmark failed");
  
  
  return rc;
    
   
 out:
  return 1;
}





int custom_move(pci_bar_handle_t handle, uint64_t offset, uint32_t* datap, uint64_t dword_len) {

  /** get the pointer to the beginning of the range */
  uint32_t *reg_ptr;
  
  int rc  = fpga_pci_get_address(handle, offset, sizeof(uint32_t)*dword_len, (void **)&reg_ptr);
  fail_on(rc, err, "fpga_plat_get_mem_at_offset failed");

  memcpy((void *)reg_ptr, (void *)datap, sizeof(uint32_t)*dword_len);

  return 0;
 err:
  return FPGA_ERR_FAIL;
}



/*
 * An example to attach to an arbitrary slot, pf, and bar with register access.
 */
int wc_perf(int slot_id, int pf_id, int bar_id) {
  struct timespec ts_start, ts_end;
  float GB_per_s;
  int num_of_bytes;
  uint32_t *buffer = NULL;
  int rc;
  
  /* pci_bar_handle_t is a handler for an address space exposed by one PCI BAR on one of the PCI PFs of the FPGA */

    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;

    /* attach to the fpga, with a pci_bar_handle out param
     * To attach to multiple slots or BARs, call this function multiple times,
     * saving the pci_bar_handle to specify which address space to interact with in
     * other API calls.
     * This function accepts the slot_id, physical function, and bar number
     */
    
    rc = fpga_pci_attach(slot_id, pf_id, bar_id, write_combine, &pci_bar_handle);
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);

    buffer = (uint32_t *)calloc(num_of_uints, sizeof(uint32_t));

    for(int j=16; j <= num_of_uints; j *= 2) {
      
      /* grab start time */
      rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);

      // perform multiple passes to minimize the affects introduced by clock_gettime
      for(int pass=0; pass < num_of_passes; pass++) {
	if (use_custom)
	  custom_move(pci_bar_handle, 0, buffer, j);
	else
	  fpga_pci_write_burst(pci_bar_handle, 0, buffer, j);
      }

      /* grab end time */
      rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);
    
      compute_delta(&ts_end, &ts_start);
      
      num_of_bytes = sizeof(uint32_t) * j * num_of_passes;
    
      if (verbose)
	printf("time %ld.%09ld seconds for transfer of %u bytes\n",
	       ts_end.tv_sec, ts_end.tv_nsec, num_of_bytes);
    
    
      GB_per_s = (float)num_of_bytes / (float)ts_end.tv_nsec / (float)num_of_passes;
      if (verbose)
	printf("bandwidth %f GB/s\n", GB_per_s);
      else
	printf("%u\t%f\n", num_of_bytes, GB_per_s);
    }
    
    
out:
    /* clean up */
    if (pci_bar_handle >= 0) {
        rc = fpga_pci_detach(pci_bar_handle);
        if (rc) {
            printf("Failure while detaching from the fpga.\n");
        }
    }

    /* if there is an error code, exit with status 1 */
    return (rc != 0 ? 1 : 0);
}


/*
 * check if the corresponding AFI for cl_dram_dma is loaded
 */

int check_afi_ready(int slot_id) {
    struct fpga_mgmt_image_info info = {0}; 
    int rc;

    /* get local image description, contains status, vendor id, and device id. */
    rc = fpga_mgmt_describe_local_image(slot_id, &info, 0);
    fail_on(rc, out, "Unable to get AFI information from slot %d. Are you running as root?",slot_id);

    /* check to see if the slot is ready */
    if (info.status != FPGA_STATUS_LOADED) {
        rc = 1;
        fail_on(rc, out, "AFI in Slot %d is not in READY state !", slot_id);
    }

    if (verbose)
      printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
        info.spec.map[FPGA_APP_PF].vendor_id,
        info.spec.map[FPGA_APP_PF].device_id);

    /* confirm that the AFI that we expect is in fact loaded */
    if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
        info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
        printf("AFI does not show expected PCI vendor id and device ID. If the AFI "
               "was just loaded, it might need a rescan. Rescanning now.\n");

        rc = fpga_pci_rescan_slot_app_pfs(slot_id);
        fail_on(rc, out, "Unable to update PF for slot %d",slot_id);
        /* get local image description, contains status, vendor id, and device id. */
        rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
        fail_on(rc, out, "Unable to get AFI information from slot %d",slot_id);

        printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
            info.spec.map[FPGA_APP_PF].vendor_id,
            info.spec.map[FPGA_APP_PF].device_id);

        /* confirm that the AFI that we expect is in fact loaded after rescan */
        if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
             info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
            rc = 1;
            fail_on(rc, out, "The PCI vendor id and device of the loaded AFI are not "
                             "the expected values.");
        }
    }

    return rc;

out:
    return 1;
}


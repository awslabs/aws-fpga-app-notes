/*
 * Copyright 2015 Amazon.com, Inc. or its affiliates.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

const char *test_msg[2] = {"0 This is a test.", "1 Print Test Pattern"};

char msg_result[100];

int main(int argc, char **argv) {
  int fd;

  fd = open("/dev/atg_driver", O_RDWR);

  if (fd < 0) {
    perror("Error");
    return -1;
  }

  // write msg == read msg
  pwrite(fd, test_msg[0], sizeof(test_msg[0]), 0);
  pread(fd, msg_result, sizeof(test_msg[0]), 0);
  printf("msg_result: %s\n", msg_result);

  // write msg != read msg
  pwrite(fd, test_msg[1], sizeof(test_msg[1]), 0);
  pread(fd, msg_result, sizeof(test_msg[1]), 0);
  printf("msg_result: %s\n", msg_result);

  close(fd);

  return 0;
}




#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libusb-1.0/libusb.h"

#include "ezusb_comm.h"

int main(int argc, char **argv) {

  if (argc != 2) {
    printf("Usage: %s bus,dev\n", argv[0]);
    return EXIT_FAILURE;
  }

  unsigned busnum = 0, devaddr = 0;

  if (sscanf(argv[1], "%u,%u", &busnum, &devaddr) != 2) {
    printf("Usage: %s bus,dev\n", argv[0]);
    return EXIT_FAILURE;
  }

  libusb_device_handle *device;
  int status = ezusb_init(busnum, devaddr, &device);
  assert(device);
  assert(status == 0);

  unsigned char rx_buf[64] = {0};

  // TODO: 0x81
  if ((status = ezusb_cmd_read(device, "read version", 0x81, 0x7F00, rx_buf,
                               sizeof(rx_buf)) < 0)) {
    printf("[ERROR] Could not get the version\n");
  }
  printf("status = %d, version = %s\n", status, rx_buf);

  ezusb_uart_send_buffer(device, "Hello", 5);

  memset(rx_buf, 0, sizeof(rx_buf));
  int recv_size;
  ezusb_uart_recv_buffer(device, rx_buf, sizeof(rx_buf), &recv_size);

close:
  ezusb_close(device);
}
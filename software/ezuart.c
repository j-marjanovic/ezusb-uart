

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libusb-1.0/libusb.h"

#include "ezusb_comm.h"
#include "terminal_mgmt.h"

int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "Usage: %s bus,dev\n", argv[0]);
    return EXIT_FAILURE;
  }

  unsigned busnum = 0, devaddr = 0;

  if (sscanf(argv[1], "%u,%u", &busnum, &devaddr) != 2) {
    fprintf(stderr, "Usage: %s bus,dev\n", argv[0]);
    return EXIT_FAILURE;
  }

  libusb_device_handle *device;
  int status = ezusb_init(busnum, devaddr, &device);
  assert(device);
  assert(status == 0);

  unsigned char verion_buf[64] = {0};
  if ((status = ezusb_cmd_read(device, "read version", EZUSB_CMD_GET_VERSION,
                               0x7F00, verion_buf, sizeof(verion_buf)) < 0)) {
    fprintf(stderr, "[ERROR] Could not get the version\n");
  }
  printf("EZ-USB UART\n");
  printf("firmware version = %s\n", verion_buf);
  printf("press Ctrl-D to exit\n");

  set_conio_terminal_mode();

  char tx_buf[32];
  int tx_buf_pos = 0;
  unsigned char rx_buf[32];
  int rx_size;

  while (1) {
    while (!kbhit()) {
      // check for received data
      memset(rx_buf, 0, sizeof(rx_buf));
      ezusb_uart_recv_buffer(device, rx_buf, sizeof(rx_buf), &rx_size);

      for (int i = 0; i < rx_size; i++) {
        putchar(rx_buf[i]);
        if (rx_buf[i] == '\r') {
          // putchar('\n');
        }
      }
      fflush(stdout);
    }

    // get character from the terminal
    char ch = getch();

    // echo
    putchar(ch);
    fflush(stdout);

    // Ctrl+C and Ctrl+D handling
    if ((ch == 0x3) || (ch == 0x4)) {
      goto close;
    }

    // append to tx buffer
    tx_buf[tx_buf_pos++] = ch;

    // transmit the data
    if ((ch == '\r') || (tx_buf_pos == sizeof(tx_buf) - 1)) {
      if (ch == '\r') {
        tx_buf[tx_buf_pos++] = '\n';
        printf("\r\n");
      }
      ezusb_uart_send_buffer(device, tx_buf, tx_buf_pos);
      memset(tx_buf, 0, sizeof(tx_buf));
      tx_buf_pos = 0;
    }
  }

close:
  ezusb_close(device);
}
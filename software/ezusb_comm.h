
// inspired by fxload.c and ezusb.c from libusb

#pragma once

#define CMD_SET_BUFFER 0xd0
#define CMD_GET_BUFFER 0xd1
#define RW_INTERNAL 0xA0 /* hardware implements this one */

static int ezusb_verbose = 1;

int ezusb_init(unsigned busnum, unsigned devaddr,
               libusb_device_handle **device) {

  /* open the device using libusb */
  int status = libusb_init(NULL);
  if (status < 0) {
    printf("[ERROR] libusb_init() failed: %s\n", libusb_error_name(status));
    return -1;
  }
  libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, ezusb_verbose);

  libusb_device *dev = NULL, *tmp, **devs;

  if (libusb_get_device_list(NULL, &devs) < 0) {
    printf("[ERROR] libusb_get_device_list() failed: %s\n",
           libusb_error_name(status));
    return -1;
  }

  for (int i = 0; (tmp = devs[i]) != NULL; i++) {
    if ((libusb_get_bus_number(tmp) == busnum) &&
        (libusb_get_device_address(tmp) == devaddr)) {
      printf("device found\n");
      dev = tmp;
      break;
    }
  }

  if (!dev) {
    printf("[ERROR] could not open device\n");
    return -1;
  }

  // libusb_device_handle *device = NULL;
  status = libusb_open(dev, device);

  libusb_free_device_list(devs, 1);
  if (status < 0) {
    printf("[ERROR] libusb_open() failed: %s\n", libusb_error_name(status));
    return -1;
  }

  /* We need to claim the first interface */
  libusb_set_auto_detach_kernel_driver(*device, 1);
  status = libusb_claim_interface(*device, 0);
  if (status != LIBUSB_SUCCESS) {
    libusb_close(*device);
    printf("[ERROR] libusb_claim_interface failed: %s\n",
           libusb_error_name(status));
    return -1;
  }

  return 0;
}

void ezusb_close(libusb_device_handle *device) {
  libusb_release_interface(device, 0);
  libusb_close(device);
  libusb_exit(NULL);
}

int ezusb_cmd_read(libusb_device_handle *device, const char *label,
                   uint8_t opcode, uint32_t addr, const unsigned char *data,
                   size_t len) {

  if (ezusb_verbose > 1) {
    printf("[TRACE] %s, addr 0x%08x len %4u (0x%04x)\n", label, addr,
           (unsigned)len, (unsigned)len);
  }

  int status = libusb_control_transfer(
      device,
      LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
      opcode, addr & 0xFFFF, addr >> 16, (unsigned char *)data, (uint16_t)len,
      1000);

  return status;
}

int ezusb_uart_send_buffer(libusb_device_handle *device, char *buffer,
                           unsigned int len) {
  int transferred;
  unsigned char tx_buf[64];
  assert(len < sizeof(tx_buf - 1));
  tx_buf[0] = CMD_SET_BUFFER;
  tx_buf[1] = len;
  memcpy(tx_buf + 2, buffer, len);

  int status = libusb_bulk_transfer(device, LIBUSB_ENDPOINT_OUT | 2, tx_buf, 64,
                                    &transferred, 1000);
  printf("status = %d (%s), transferred = %d\n", status,
         libusb_error_name(status), transferred);

  return status;
}

int ezusb_uart_recv_buffer(libusb_device_handle *device, unsigned char *rx_buf,
                           int rx_buf_size, int *recv_size) {

  int transferred;

  rx_buf[0] = CMD_GET_BUFFER;
  int status = libusb_bulk_transfer(device, LIBUSB_ENDPOINT_OUT | 2, rx_buf, 64,
                                    &transferred, 1000);
  printf("status = %d (%s), transferred = %d\n", status,
         libusb_error_name(status), transferred);

  status = libusb_bulk_transfer(device, LIBUSB_ENDPOINT_IN | 2, rx_buf, 64,
                                &transferred, 1000);
  printf("status = %d (%s), transferred = %d\n", status,
         libusb_error_name(status), transferred);

  if (recv_size) {
    printf("recv size = %d\n", rx_buf[0]);
    *recv_size = rx_buf[0];
  }

  // TODO: copy buffer

  return status;
}
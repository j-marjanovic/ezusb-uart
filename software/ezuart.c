
// inspired by fxload.c and ezusb.c from libusb

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libusb-1.0/libusb.h"

#define RW_INTERNAL 0xA0 /* hardware implements this one */
#define CMD_SET_BUFFER 0x85
#define CMD_GET_BUFFER 0x86

int verbose = 1;

struct cmd {
  uint32_t len;
  char buffer[32];
};

static int ezusb_write(libusb_device_handle *device, const char *label,
                       uint8_t opcode, uint32_t addr, const unsigned char *data,
                       size_t len) {

  if (verbose > 1) {
    printf("[TRACE] %s, addr 0x%08x len %4u (0x%04x)\n", label, addr,
           (unsigned)len, (unsigned)len);
  }

  int status =
      libusb_control_transfer(device,
                              LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                                  LIBUSB_RECIPIENT_DEVICE,
                              opcode, addr & 0xFFFF, addr >> 16,
                              (unsigned char *)data, (uint16_t)len, 1000);

  return status;
}

static int ezusb_read(libusb_device_handle *device, const char *label,
                      uint8_t opcode, uint32_t addr, const unsigned char *data,
                      size_t len) {

  if (verbose > 1) {
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

int main(int argc, char **argv) {

  if (argc != 2) {
    printf("Usage: %s bus,dev\n", argv[0]);
    return EXIT_FAILURE;
  }

  unsigned busnum = 0, devaddr = 0;
  unsigned _busnum = 0, _devaddr = 0;

  if (sscanf(argv[1], "%u,%u", &busnum, &devaddr) != 2) {
    printf("Usage: %s bus,dev\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* open the device using libusb */
  int status = libusb_init(NULL);
  if (status < 0) {
    printf("[ERROR] libusb_init() failed: %s\n", libusb_error_name(status));
    return -1;
  }
  libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, verbose);

  libusb_device *dev = NULL, *tmp, **devs;

  if (libusb_get_device_list(NULL, &devs) < 0) {
    printf("[ERROR] libusb_get_device_list() failed: %s\n",
           libusb_error_name(status));
    return EXIT_FAILURE;
  }

  for (int i = 0; (tmp = devs[i]) != NULL; i++) {
    _busnum = libusb_get_bus_number(tmp);
    _devaddr = libusb_get_device_address(tmp);
    // if both a type and bus,addr were specified, we just need to find our
    // match
    if ((libusb_get_bus_number(tmp) == busnum) &&
        (libusb_get_device_address(tmp) == devaddr)) {
      printf("device found\n");
      dev = tmp;
      break;
    }
  }

  if (!dev) {
    printf("[ERROR] could not open device\n");
    return EXIT_FAILURE;
  }

  libusb_device_handle *device = NULL;
  status = libusb_open(dev, &device);

  libusb_free_device_list(devs, 1);
  if (status < 0) {
    printf("[ERROR] libusb_open() failed: %s\n", libusb_error_name(status));
    return EXIT_FAILURE;
  }

  /* We need to claim the first interface */
  libusb_set_auto_detach_kernel_driver(device, 1);
  status = libusb_claim_interface(device, 0);
  if (status != LIBUSB_SUCCESS) {
    libusb_close(device);
    printf("[ERROR] libusb_claim_interface failed: %s\n",
           libusb_error_name(status));
    return EXIT_FAILURE;
  }

  char rBuf[128] = {0};

  struct cmd cmd = {.len = 7, .buffer = "test1234"};

  if ((status = ezusb_read(device, "read version", 0x81, 0x7F00, rBuf,
                           sizeof(rBuf)) < 0)) {
    printf("[ERROR] Could not get the version\n");
  }
  printf("status = %d, version = %s\n", status, rBuf);

  uint32_t transferred;
  rBuf[0] = 0xd0;
  rBuf[1] = 0x12;
  status = libusb_bulk_transfer(device, LIBUSB_ENDPOINT_OUT | 2, rBuf, 64,
                                &transferred, 1000);
  printf("status = %d (%s), transferred = %d\n", status,
         libusb_error_name(status), transferred);

  rBuf[0] = 0xd1;
  status = libusb_bulk_transfer(device, LIBUSB_ENDPOINT_OUT | 2, rBuf, 64,
                                &transferred, 1000);
  printf("status = %d (%s), transferred = %d\n", status,
         libusb_error_name(status), transferred);

  status = libusb_bulk_transfer(device, LIBUSB_ENDPOINT_IN | 2, rBuf, 64,
                                &transferred, 1000);
  printf("status = %d (%s), transferred = %d\n", status,
         libusb_error_name(status), transferred);

  printf("%s\n", rBuf);

close:
  libusb_release_interface(device, 0);
  libusb_close(device);
  libusb_exit(NULL);
}
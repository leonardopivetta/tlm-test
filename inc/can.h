#ifndef CAN_H
#define CAN_H

#include <stdio.h>
#include <string.h>
#include <iostream>

#include <unistd.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

// CAN libraries (can_utils)
#include <linux/can.h>
#include <linux/can/raw.h>

using namespace std;
class Can
{
public:
  /**
   *  Must set device and address with init
   */
  Can();

  /**
   * Sets device string (can0, vcan0)
   * Sets address pointer
   */
  Can(const char *device, struct sockaddr_can *address);

  /**
   * Sets device string (can0, vcan0)
   * Sets address pointer
   */
  void init(const char *device, struct sockaddr_can *address);

  /**
   * Opens device updating address,
   *
   * return socket fd
   */
  int open_socket();

  /**
   * Closes the can socket
   *
   * return true if close was successfull
   */
  bool close_socket();

  /**
   * Returns if the socket is opened
   */
  bool is_open();

  /**
   * Returns device name
   */
  const char *get_device();

  /**
   * Sends an array of bytes
   *
   * @param id message id
   * @param data array of bytes
   * @param len num of bytes to be sent (0~8)
   * return success
   */
  int send(int id, char *data, int len);

  /**
   * Receive a can frame from device
   *
   * @param frame pointer of struct that will be filled
   * return success
   */
  int receive(can_frame *frame);

  /**
   * Setup filters so select id that can be received
   *
   * @param filter struct filled (id, mask)
   * return success
   */
  int set_filters(can_filter &filter);

private:
  int sock;              // socket fd
  const char *device;    // name of device
  sockaddr_can *address; // address of device

  bool opened;
};

#endif

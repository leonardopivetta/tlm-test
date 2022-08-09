#include "can.h"

Can::Can()
{
}

Can::Can(const char *device, sockaddr_can *address)
{
	init(device, address);
}

void Can::init(const char *device, sockaddr_can *address)
{
	this->device = device;
	this->address = address;
	opened = false;
}

bool Can::is_open()
{
	return opened;
}

const char *Can::get_device()
{
	return device;
}

int Can::open_socket()
{
	int can_socket;
	struct ifreq ifr;

	can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (can_socket < 0)
		return -1;

	// If can_socket is set to 0,
	// messages are received from all interfaces (can0, can1, vcan0)
	strcpy(ifr.ifr_name, this->device);
	ioctl(can_socket, SIOCGIFINDEX, &ifr);

	(*address).can_family = AF_CAN;
	(*address).can_ifindex = ifr.ifr_ifindex;

	if (bind(can_socket, (struct sockaddr *)address, sizeof(*address)) < 0)
	{
		return -2;
	}

	opened = true;
	this->sock = can_socket;
	return can_socket;
}

bool Can::close_socket()
{
	if (opened)
	{
		if (close(this->sock) < 0)
		{
			cout << "Error closing can socket: " << errno << endl;
			return false;
		}
	}
	return true;
}

int Can::send(int id, char *data, int len)
{
	if (len < 0 || len > 8)
	{
		return -1;
	}

	struct can_frame frame;
	frame.can_id = id;
	frame.can_dlc = len;

	for (int i = 0; i < len; ++i)
	{
		frame.data[i] = data[i];
	}

	return write(this->sock, &frame, sizeof(frame));
}

int Can::receive(can_frame *frame)
{
	return read(this->sock, frame, sizeof(struct can_frame));
}

int Can::set_filters(can_filter &filter)
{
	return setsockopt(this->sock, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter));
}

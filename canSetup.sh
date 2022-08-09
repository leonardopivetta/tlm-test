modprobe can_dev
modprobe can
modprobe can_raw
ip link set can0 type can bitrate 1000000
ifconfig can0 up
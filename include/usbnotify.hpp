
#include <iostream>
#include <asm/types.h>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <optional>
#include <array>

constexpr int NL_BUFF_LEN = 8192;
constexpr std::array<std::string_view, 3> requiredFields{"ACTION=", "DEVTYPE=", "SUBSYSTEM="};
constexpr std::array<std::string_view, 2> targetFields{ "DEVPATH=", "PRODUCT="}; // ACTION= goes here too but is handled by required fields


struct usbEvent {
  std::string action;
  std::string devpath;
  std::string product;

};


std::optional<usbEvent> parseEvent(std::string& msg); 






























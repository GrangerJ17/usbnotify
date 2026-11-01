
#include <include/usbnotify.hpp>




std::optional<usbEvent> parseEvent(std::string& msg) {
  std::string_view sv{msg};

  usbEvent event;

  for(std::string_view svField : requiredFields) {

    std::string field {svField};
    std::size_t pos = sv.find(field);
    if(pos == -1) return std::nullopt;

    if(field == "ACTION=")  {
      std::size_t start = pos + field.length();
      std::string action = msg.substr(start, msg.find('\n',start)-start);
      if(action != "add" && action != "remove") return std::nullopt;

      event.action = action;
    }
  }


  for(std::string_view svField : targetFields) {

    std::string field {svField};
    std::size_t pos = sv.find(field);
    if(pos == -1) return std::nullopt;

    if(field == "DEVPATH=")  {
      std::size_t start = pos + field.length();
      event.devpath = msg.substr(start, msg.find('\n',start)-start);
    }

    if(field == "PRODUCT=")  {
      std::size_t start = pos + field.length();
      event.product = msg.substr(start, msg.find('\n',start)-start);
    }
  }


  return event;

}

int main()  {
    
    int nl_socket = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(nl_socket < 0) {
      perror("socket");
      exit(1);
    }

    struct sockaddr_nl nl_addr;

    std::memset(&nl_addr, 0, sizeof(nl_addr));
    nl_addr.nl_family = AF_NETLINK;
    nl_addr.nl_pid = 0;
    nl_addr.nl_groups = 1;

    if(bind(nl_socket, (struct sockaddr *)&nl_addr, sizeof(nl_addr)) < 0) {
      perror("bind");
      close(nl_socket);
      exit(1);
    }

    std::vector<std::byte> buf;
    buf.resize(NL_BUFF_LEN);




    while(true) {
      ssize_t len = recv(nl_socket, buf.data(), NL_BUFF_LEN, 0);

      std::string msg(reinterpret_cast<char*>(buf.data()), len);
      for(int i = 0; i < msg.length(); i++) {
        if(msg[i] == '\0') msg[i] = '\n';
      }
  
      std::optional<usbEvent> event = parseEvent(msg) ;

      if(event) { 
        std::cout << event->action << std::endl;
        std::cout << event->devpath << std::endl;
        std::cout << event->product << std::endl;
      }

     }

}

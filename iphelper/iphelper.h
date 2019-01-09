#ifndef iphelper_h
#define iphelper_h

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#include <iostream>
#include <vector>

namespace net {

typedef struct _IP_INFO {
  std::string ip;
  std::string mask;
  std::string getway;
} IP_INFO;

typedef struct _NETCARD_INFO {
  std::string name;             // 名字
  std::string des;              // 描述
  std::string type;             // 类型

  size_t      rtype;            // 数字版 类型

  std::string mac;              // mac
  std::vector<IP_INFO> ipis;    // ip info
} NETCARD_INFO;


struct iphelper {
  static constexpr char request[] = 
    "GET /ic.asp HTTP/1.1\r\nHost:2018.ip138.com\r\nConnection: close\r\n\r\n";
    
public:
  iphelper();
  ~iphelper();

  bool getPublicNetIp() {
    return getPublicNetIp_D();
  }

  bool getPublicNetIp_D();
  bool getLocalInfo();

  std::string pip;
  std::vector<NETCARD_INFO> cards;
};

};  // net

#endif  // iphelper

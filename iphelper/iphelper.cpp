#include "iphelper.h"

#pragma comment(lib,"Iphlpapi.lib") // ��Ҫ���Iphlpapi.lib��
#pragma comment(lib,"Ws2_32.lib")   // ��Ҫ���Iphlpapi.lib��

namespace net {
iphelper::iphelper() {
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
}

iphelper::~iphelper() {
  WSACleanup();
}


// note. 
//  �ú�����������
//  �ú�����ȡ���� Ĭ��(�Զ�ѡ��) �� ��ָ���ĸ����ȼ� �����Ĺ���ip
bool iphelper::getPublicNetIp_D() {
  bool           ret       = false;
  SOCKET         sock      = INVALID_SOCKET;
  sockaddr_in    destAddr  = { 0 };
  hostent*       rhost     = NULL;
  char*          rip       = NULL;

  //  receive
  int            recv_size = 0;
  std::string    receiver;
  size_t         s = 0, e = 0;

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (INVALID_SOCKET == sock) {
    return ret;
  }

  destAddr.sin_family = AF_INET;
  destAddr.sin_port   = htons(80);

  rhost = gethostbyname("2018.ip138.com");
  if (NULL == rhost)
    goto __ret;

  rip = inet_ntoa(*((struct in_addr *)rhost->h_addr));
  if (NULL == rip)
    goto __ret;

  destAddr.sin_addr.S_un.S_addr = inet_addr(rip);

  // �����Ǽ����������
  if (S_OK != connect(sock, (struct sockaddr*)&destAddr, sizeof(destAddr)))
    goto __ret;

  // �����Ƿ��ͻ���ľ������
  if (SOCKET_ERROR == send(sock, request, (int)strlen(request), 0))
    goto __ret;

  if (SOCKET_ERROR == shutdown(sock, SD_SEND))
    goto __ret;
  
  receiver.resize(500);

  while ((recv_size = recv(sock, (char*)receiver.c_str(), 500, 0)) > 0) {
    // ��ԶԤ���� 500 �ֽ�
    receiver.resize(receiver.size() + recv_size);
  }

  s = receiver.find("[");
  e = receiver.find("]");

  if (s == std::string::npos ||
      e == std::string::npos)
    goto __ret;

  pip = receiver.substr(s + 1, e - s - 1);

  ret = true;

__ret:
  closesocket(sock);
  return ret;
}

bool iphelper::getLocalInfo() {

  PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
  unsigned long stSize = sizeof(IP_ADAPTER_INFO);
  int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);

  int IPnumPerNetCard = 0;

  if (ERROR_BUFFER_OVERFLOW == nRel) {
    delete pIpAdapterInfo;

    pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];

    nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
  }
    
  if (ERROR_SUCCESS != nRel) {
    goto __ret;
  }

  while (pIpAdapterInfo) {
    NETCARD_INFO card;

    card.name  = pIpAdapterInfo->AdapterName;
    card.des   = pIpAdapterInfo->Description;
    card.rtype = pIpAdapterInfo->Type;

    switch (pIpAdapterInfo->Type) {
    case MIB_IF_TYPE_OTHER:
      card.type = "OTHER";
      break;
    case MIB_IF_TYPE_ETHERNET:
      card.type = "ETHERNET";
      break;
    case MIB_IF_TYPE_TOKENRING:
      card.type = "TOKENRING";
      break;
    case MIB_IF_TYPE_FDDI:
      card.type = "FDDI";
      break;
    case MIB_IF_TYPE_PPP:
      card.type = "PPP";
      break;
    case MIB_IF_TYPE_LOOPBACK:
      card.type = "LOOPBACK";
      break;
    case MIB_IF_TYPE_SLIP:
      card.type = "SLIP";
      break;
    default:
      card.type = "UNKNOW";
      break;
    }

    // �� ����ѭ���ڲ��� if ����
    char c[4] = { 0 };
    for (size_t i = 0; i < pIpAdapterInfo->AddressLength - 1; i++) {
      memset(c, 0, 4);
      sprintf_s(c, 4, "%02X-", pIpAdapterInfo->Address[i]);
      card.mac += c;
    }
    sprintf_s(c, 3, "%02X", pIpAdapterInfo->Address[pIpAdapterInfo->AddressLength - 1]);
    card.mac += c;
 
    IP_ADDR_STRING *pIpAddrString = &(pIpAdapterInfo->IpAddressList);
    do
    {
      IP_INFO ipi;

      ipi.ip     = pIpAddrString->IpAddress.String;
      ipi.mask   = pIpAddrString->IpMask.String;
      ipi.getway = pIpAdapterInfo->GatewayList.IpAddress.String;

      card.ipis.push_back(ipi);

      pIpAddrString = pIpAddrString->Next;
    } while (pIpAddrString);

    cards.push_back(card);
    pIpAdapterInfo = pIpAdapterInfo->Next;
  }

__ret:
  // �ͷ��ڴ�ռ�
  if (pIpAdapterInfo)
    delete pIpAdapterInfo;

  return true;
}

};  // net
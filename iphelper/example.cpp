#include "iphelper.h"

int main(int argc, char* argv[]) {

  net::iphelper obj;

  while (!obj.getLocalInfo()) Sleep(200);
  while (!obj.getPublicNetIp()) Sleep(200);

  std::cout << "本机公网 ip 为 : " << obj.pip.c_str() << std::endl << std::endl;

  std::cout << "发现网络适配器 " << obj.cards.size() << " 个" << std::endl;

  for(auto& cs : obj.cards) {
    std::cout << cs.des.c_str() << " --- " << cs.mac.c_str() << std::endl;
    for (auto& ip : cs.ipis) {
      std::cout << "  " << ip.ip.c_str() << std::endl;
    }
  }

  system("pause");
  return 0;
}
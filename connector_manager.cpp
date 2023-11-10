
#include"connector_manager.h"
void connector::init_return_data(return_data* data){
  data->callback=NULL;
  data->json_send.clear();
  data->respon_id=-1;
  data->server_hash="";
}
void connector::init_task(task* ev){
  ev->json.clear();
  ev->note=false;
  ev->empty=true;
}
std::string connector::GetLocalIP() {
     struct ifaddrs* ifAddrStruct = nullptr;
    void* tmpAddrPtr = nullptr;
    std::string ipAddress;

    getifaddrs(&ifAddrStruct);

    struct ifaddrs* ifa = ifAddrStruct;
    while (ifa != nullptr) {
        if (ifa->ifa_addr->sa_family == AF_INET) {
            tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (std::string(ifa->ifa_name) == "lo") {
                ipAddress = std::string(addressBuffer);
            } else {
                ipAddress = std::string(addressBuffer);
                break;
            }
        }
        ifa = ifa->ifa_next;
    }

    freeifaddrs(ifAddrStruct);

    return ipAddress;
}
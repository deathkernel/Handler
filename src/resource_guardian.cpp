#include "handler/resource_guardian.h"
#ifdef _WIN32
#include <winsock2.h>
#endif
namespace handler {
std::vector<PortFinding> inspectPorts(const std::vector<int>& ports) {
    std::vector<PortFinding> out;
#ifdef _WIN32
    for (const int port : ports) {
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) { out.push_back({port, false, "socket initialization failed"}); continue; }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(static_cast<u_short>(port));
        const bool available = bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
        closesocket(s);
        out.push_back({port, available, available ? "available" : "in use or restricted"});
    }
#else
    for (const int port : ports) out.push_back({port, true, "port probe unavailable on this build"});
#endif
    return out;
}
}
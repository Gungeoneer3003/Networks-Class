#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <thread>
#include <string>
#define WIN32_LEAN_AND_MEAN
#define STB_IMAGE_IMPLEMENTATION
#define num 10
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

typedef unsigned char BYTE;
struct ThreadArgs {
    SOCKET sender;
    SOCKET receiver;
};


void sendAll(SOCKET c, char* buf, int len) {
    int totalSent = 0;

    while (totalSent < len) {
        int s = send(c, buf + totalSent, len - totalSent, 0);
        totalSent += s;
    }
}

void writeAll(SOCKET c, char* buf, int len) {
    cout << "Beginning to write" << endl;
    int totalWrote = 0;

    while (totalWrote < len) {
        int ws = recv(c, buf + totalWrote, len - totalWrote, 0);
        cout << "Successfully wrote " << ws << endl;
        totalWrote += ws;
    }
}

static SOCKET connect_tcp(const char* host, const char* port)
{
    struct addrinfo hints, * res = 0, * p = 0;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, port, &hints, &res) != 0) return INVALID_SOCKET;

    for (p = res; p; p = p->ai_next)
    {
        SOCKET s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == INVALID_SOCKET) continue;
        if (connect(s, p->ai_addr, (int)p->ai_addrlen) == 0)
        {
            freeaddrinfo(res);
            return s;
        }
        closesocket(s);
    }

    freeaddrinfo(res);
    return INVALID_SOCKET;
}

DWORD WINAPI feedbackLoop(LPVOID p) {
    ThreadArgs arg = *((ThreadArgs*)p);
    SOCKET sender = arg.sender;
    SOCKET receiver = arg.receiver;

    for (int i = 0; i < num; i++) {
        char buf[4096];
        int n;
        while ((n = recv(sender, buf, sizeof(buf), 0)) > 0)
        {
            send(receiver, buf, n, 0);
        }
    }

    return 0;
}

int main(int argc, char** argv) {
    //Hardcoded inputs
    int n = 10;
    char file[] = "calc";

    int port = 5000;

    WSADATA w;
    if (WSAStartup(MAKEWORD(2, 2), &w) != 0) return 1;

    int yes = 1;
    SOCKET ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(ls, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return 1;

    if (listen(ls, n) == SOCKET_ERROR) return 1;

    printf("Echo server listening on port %d\n", port);
}
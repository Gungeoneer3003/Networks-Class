#include <cstring>
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

static uint64_t now_us(void)
{
    static LARGE_INTEGER f;
    static int init = 0;
    if (!init) { QueryPerformanceFrequency(&f); init = 1; }
    LARGE_INTEGER t; QueryPerformanceCounter(&t);
    return (uint64_t)((t.QuadPart * 1000000ULL) / (uint64_t)f.QuadPart);
}

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
    buf[totalWrote] = '\0'; 
}

int main(int argc, char** argv)
{
    int port = 5000;

    WSADATA w;
    if (WSAStartup(MAKEWORD(2, 2), &w) != 0) return 1;

    SOCKET ls = connect_tcp("localhost", "5000");

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    //Making a server
    //bind(ls, (struct sockaddr*)&addr, sizeof(addr));
    //listen(ls, 5);

    printf("Echo server listening on port %d\n", port);

    //wait for ready from server
    char buff[50];
    char check[50] = "ready\0";
    for (;;) {
        writeAll(ls, buff, strlen(check));
        if (strcmp(buff, check) == 0)
            break;
        else
            cout << "didn't match, it was" << buff << endl; 
        Sleep(100);
    }

    cout << "Now starting" << endl;
    //send ping 0
    //wait for pong 0
    //sleep(1s) 
    //Incremement ping
    //send ping 1

    char c;
    char msg[100] = "ping 0\0";
    for (int i = 0; i < num; i++) {
        c = i + '0'; 
        msg[5] = c; 
        
        for (;;) {
            writeAll(ls, buff, 7);
            if (strcmp(buff, msg) == 0)
                break;

            Sleep(100);
        }

        msg[1] = 'o';
        sendAll(ls, msg, 7);
        
        //Sleep(1000);
        msg[1] = 'i';
    }

    closesocket(ls);
    return 0;
}
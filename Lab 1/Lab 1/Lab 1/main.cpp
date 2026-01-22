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
}

int main(int argc, char** argv)
{
    int port = 5000;

    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);

    //SOCKET ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    //bind(ls, (struct sockaddr*)&addr, sizeof(addr));
    //listen(ls, 5);

    printf("Echo server listening on port %d\n", port);


    //Initialize the buffer to send and receive
    srand(time(0));
    char buf[3200];
    for (int i = 0; i < 3200; i++) {
        buf[i] = 'a' + rand() % 26;
        cout << buf[i] << " ";
    }
    cout << endl << "End of the input stage" << endl << endl;

    SOCKET c = connect_tcp("localhost", "5000");

    //Send small messages to find latency
    long long totaldt = 0;
    for (int i = 0; i < 100; i++) {
        cout << "Now starting test " << i << endl;
        long long t1 = now_us();

        sendAll(c, buf, 32);
        writeAll(c, buf, 32);

        long long t2 = now_us();
        long long dt = t2 - t1;
        totaldt += dt;
    }
    totaldt /= 100;
    cout << "The small package latency is" << totaldt << endl;

    //Repeat but for large buffer
    long long t1 = now_us(); 

    sendAll(c, buf, 3200); 
    writeAll(c, buf, 3200); 

    long long t2 = now_us(); 
    long long dt = t2 - t1;
    cout << "The large package latency is" << dt << endl;

    closesocket(c);
   /* for (;;)
    {
        SOCKET c = accept(ls, NULL, NULL);
        if (c == INVALID_SOCKET) continue;

        char buf[4096];
        int n;
        while ((n = recv(c, buf, sizeof(buf), 0)) > 0)
        {
            send(c, buf, n, 0);
        }

        closesocket(c);
    }*/
}

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
#define contrast 10
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

typedef unsigned char BYTE;

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

int adjust(int x) {
    int c = contrast;
    double f = (259.0 * (c + 255)) / (255.0 * (259 - c));
    int y = (int)(f * (x - 128) + 128);
    if (y < 0) y = 0;
    if (y > 255) y = 255;
    return y;
}

int main(int argc, char** argv) {
    int port = 5000;

    WSADATA w;
    if (WSAStartup(MAKEWORD(2, 2), &w) != 0) return 1;

    SOCKET ls = connect_tcp("localhost", "5000");

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    printf("Echo server listening on port %d\n", port);

    //Get how much data is being sent
    int rows, pixelPerRow;
    char buff[10];

    for (;;) {
        writeAll(ls, buff, 4);
        buff[4] = '\0';
        rows = atoi(buff);

        writeAll(ls, buff, 4);
        buff[4] = '\0'; 
        pixelPerRow = atoi(buff);

        if (rows != 0)
            break;
    }

    //Setup variables for the data
    int pixelCount = rows * pixelPerRow;
    BYTE* arr = new BYTE[pixelCount]();
    char* BUFFer = new char[pixelCount];
    char flag[5];

    //Get the data, ended by a flag saying "ready"
    for (;;) {
        writeAll(ls, BUFFer, pixelCount * sizeof(char));
        
        writeAll(ls, flag, 5);
        if (strcmp(flag, "ready") == 0)
            break;
    }
    memcpy(BUFFer, arr, pixelCount * sizeof(char)); 

    //Perform contrast on the data
    int x, y;
    for (int i = 0; i < pixelCount; i++) {
        x = static_cast<int>(arr[i]);
        y = adjust(x);
        arr[i] = static_cast<BYTE>(y);
    }

    //Return the data
    memcpy(arr, BUFFer, pixelCount * sizeof(char)); 
    sendAll(ls, BUFFer, pixelCount * sizeof(char));

    //Conclude Program
    delete [] arr;
    delete [] BUFFer;
    return 0;
}
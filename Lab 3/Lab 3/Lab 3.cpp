#include <iostream>
#include <stdatomic.h>
using namespace std;

//Structs for the thread and bmp 
struct BMPFileHeader{
unsigned short bfType; // 'BM' = 0x4D42
unsigned int bfSize; // file size in bytes
unsigned short bfReserved1; // must be 0
unsigned short bfReserved2; // must be 0
unsigned int bfOffBits; // offset to pixel data
};

struct BMPInfoHeader {
    unsigned int biSize; // header size (40)
    int biWidth; // image width
    int biHeight; // image height
    unsigned short biPlanes; // must be 1
    unsigned short biBitCount; // 24 for RGB
    unsigned int biCompression; // 0 = BI_RGB
    unsigned int biSizeImage; // image data size (can be 0 for BI_RGB)
    int biXPelsPerMeter; // resolution
    int biYPelsPerMeter; // resolution
    unsigned int biClrUsed; // colors used (0)
    unsigned int biClrImportant; // important colors (0)
};


atomic<int> count(0);
atomic<int> gs(0);

//Thread Function

//Gather
void gather(int n, int* ls) {
    extern atomic<int> count, gs;
    int oldCount = count.fetch_add(1);
    
    if (oldCount == n) {
        count.store(0);
        gs.store(gs.load() - 1);
    } 
    else while (gs.load() == *ls) {}
    
    *ls = 1 - *ls;
    return;
}

int main()
{
    //Load BMP

    //Free memory

    return 0;
}
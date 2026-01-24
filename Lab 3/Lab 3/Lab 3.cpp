#include <iostream>
#include <stdatomic.h>
#include <windows.h>
#include <cmath>
using namespace std;

struct TA {
    BYTE* data;
    int process;
    int processCount;
    int pixels;
    int rwb;
    LONG w;
    LONG h;
};

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;
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

//Global Atomic Variables
atomic<int> count(0);
atomic<int> gs(0);
atomic<double> LTotal(0);
atomic<double> LAvg(0);

//Global Constants
double a = 0.18;
double r = 0.2126;
double g = 0.7152;
double b = 0.0722;


//Thread Function
DWORD WINAPI processImage(LPVOID p) { 
    extern atomic<double> LTotal, LAvg;
    extern double a, r, g, b;
    
    //Load in the data
    TA arg = *((TA*)p); 
    BYTE* data = arg.data;
    int i = arg.process;
    int n = arg.processCount;
    int rwb = arg.rwb;
    int pixels = arg.pixels;
    LONG h = arg.h;
    LONG w = arg.w;

    //Stage 1 - add all L's
    double currentL;
    for (int x = i * w / n; x < (i + 1) * w / n; x++)
    {
        for (int y = 0; y < h; y++)
        {
            int c = x * 3 + y * rwb; // c for cursor

            currentL = b * data[c];
            currentL += g * data[c + 1];
            currentL += r * data[c + 2]; 

            currentL = log(currentL + 1); //logL at this point
            double old = LTotal.load();
            while (!LTotal.compare_exchange_weak(old, old + currentL)) {}
        }
    }

    
    //Wait
    int* ls;
    *ls = 0;
    gather(i, ls);

    //Wait again as Process 0 works
    if (i == 0) {
        double properVal = exp(LTotal.load() / pixels) - 1;
        double old = LAvg.load();
        while (!LAvg.compare_exchange_weak(old, old + properVal));
    }
    gather(i, ls);

    //Stage 2 - Apply LAvg
    double Lm, Ld, La, scale; //La refers to LAvg
    La = LAvg.load();
    for (int x = i * w / n; x < (i + 1) * w / n; x++)
    {
        for (int y = 0; y < h; y++)
        {
            int c = x * 3 + y * rwb; // c for cursor

            currentL = b * data[c];
            currentL += g * data[c + 1];
            currentL += r * data[c + 2];

            Lm = (a / La) * currentL;

            Ld = Lm / (1 + Lm);
            if (currentL > 0)
                scale = Ld / currentL;
            else
                scale = 0;

            data[c] = data[c] * scale;
            data[c + 1] = data[c + 1] * scale;
            data[c + 2] = data[c + 2] * scale;
        }
    }

    return 0;
}

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
    char input[] = "picture.bmp";

    //Handle input
    FILE* f1 = fopen(input, "rb");
    if (f1 == NULL) {
        perror("Failed to open the file.");
        return 1;
    }

    //Take BMP Input1
    BMPFileHeader fh;
    BMPInfoHeader fih;

    fread(&fh.bfType, 2, 1, f1);
    fread(&fh.bfSize, 4, 1, f1);
    fread(&fh.bfReserved1, 2, 1, f1);
    fread(&fh.bfReserved2, 2, 1, f1);
    fread(&fh.bfOffBits, 4, 1, f1);

    fread(&fih, sizeof(fih), 1, f1);

    int byteWidth = fih.biWidth * 3;
    int padding = 4 - byteWidth % 4;
    if (padding == 4)
        padding = 0;
    int rwb = byteWidth + padding;

    LONG w = fih.biWidth;
    LONG h = fih.biHeight;


    BYTE* data = new BYTE[fih.biSizeImage]();
    fread(data, fih.biSizeImage, 1, f1);

    //Create thread arguments
    int n = 4;
    TA** arr = new TA*[n]();


    for (int i = 0; i < n; i++) {
        arr[i]->data = data;
        arr[i]->processCount = n;
        arr[i]->process = i;
        arr[i]->h = h;
        arr[i]->w = w;
        arr[i]->rwb = rwb;
        arr[i]->pixels = fih.biSizeImage;
    }

    HANDLE* handles = new HANDLE[4]();
    
    //Begin processes
    for (int i = 0; i < n; i++) {
        handles[i] = CreateThread(nullptr, 0, processImage, arr[i], 0, nullptr);
    }


    //Conclude Processes


    //Conclude file
    char output[] = "output.bmp";
    FILE* secondFile = fopen(output, "wb"); 

    fwrite(&fh.bfType, 2, 1, secondFile); 
    fwrite(&fh.bfSize, 4, 1, secondFile); 
    fwrite(&fh.bfReserved1, 2, 1, secondFile); 
    fwrite(&fh.bfReserved2, 2, 1, secondFile); 
    fwrite(&fh.bfOffBits, 4, 1, secondFile); 

    fwrite(&fih, sizeof(fih), 1, secondFile); 

    fwrite(data, fih.biSizeImage, 1, secondFile);

    fclose(secondFile);

    //Free memory
    delete[] data;
    delete[] arr;
    delete[] handles;
    return 0;
}
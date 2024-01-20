#include "stdafx.h"
#include "Noise.h"

#include <vector>



Noise::Noise(int seed){
    Randomize(seed);
}

Noise::Noise() {
    Randomize(0);
}


float Noise::Evaluate(DirectX::XMFLOAT3 point) {

    double x = point.x;
    double y = point.y;
    double z = point.z;
    double n0 = 0, n1 = 0, n2 = 0, n3 = 0;
    double s = (x + y + z) * F3;
    int i = FastFloor(x + s);
    int j = FastFloor(y + s);
    int k = FastFloor(z + s);
    double t = (i + j + k) * G3;
    double x0 = x - (i - t);
    double y0 = y - (j - t);
    double z0 = z - (k - t);
    int i1, j1, k1;
    int i2, j2, k2;
    if (x0 >= y0)
    {
        if (y0 >= z0)
        {
            // X Y Z order
            i1 = 1;
            j1 = 0;
            k1 = 0;
            i2 = 1;
            j2 = 1;
            k2 = 0;
        }
        else if (x0 >= z0)
        {
            // X Z Y order
            i1 = 1;
            j1 = 0;
            k1 = 0;
            i2 = 1;
            j2 = 0;
            k2 = 1;
        }
        else
        {
            // Z X Y order
            i1 = 0;
            j1 = 0;
            k1 = 1;
            i2 = 1;
            j2 = 0;
            k2 = 1;
        }
    }
    else
    {
        // x0 < y0
        if (y0 < z0)
        {
            // Z Y X order
            i1 = 0;
            j1 = 0;
            k1 = 1;
            i2 = 0;
            j2 = 1;
            k2 = 1;
        }
        else if (x0 < z0)
        {
            // Y Z X order
            i1 = 0;
            j1 = 1;
            k1 = 0;
            i2 = 0;
            j2 = 1;
            k2 = 1;
        }
        else
        {
            // Y X Z order
            i1 = 0;
            j1 = 1;
            k1 = 0;
            i2 = 1;
            j2 = 1;
            k2 = 0;
        }
    }

    double x1 = x0 - i1 + G3;
    double y1 = y0 - j1 + G3;
    double z1 = z0 - k1 + G3;
    
    double x2 = x0 - i2 + F3;
    double y2 = y0 - j2 + F3;
    double z2 = z0 - k2 + F3;
    
    double x3 = x0 - 0.5;
    double y3 = y0 - 0.5;
    double z3 = z0 - 0.5;

    int ii = i & 0xff;
    int jj = j & 0xff;
    int kk = k & 0xff;

    double t0 = 0.6 - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 > 0)
    {
        t0 *= t0;
        int gi0 = _random[ii + _random[jj + _random[kk]]] % 12;
        n0 = t0 * t0 * Dot(Grad3[gi0], x0, y0, z0);
    }
    double t1 = 0.6 - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 > 0)
    {
        t1 *= t1;
        int gi1 = _random[ii + i1 + _random[jj + j1 + _random[kk + k1]]] % 12;
        n1 = t1 * t1 * Dot(Grad3[gi1], x1, y1, z1);
    }
    double t2 = 0.6 - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 > 0)
    {
        t2 *= t2;
        int gi2 = _random[ii + i2 + _random[jj + j2 + _random[kk + k2]]] % 12;
        n2 = t2 * t2 * Dot(Grad3[gi2], x2, y2, z2);
    }
    double t3 = 0.6 - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 > 0)
    {
        t3 *= t3;
        int gi3 = _random[ii + 1 + _random[jj + 1 + _random[kk + 1]]] % 12;
        n3 = t3 * t3 * Dot(Grad3[gi3], x3, y3, z3);
    }

    return (float)(n0 + n1 + n2 + n3) * 32;

}

void Noise::Randomize(int seed) {
    _random = new int[RandomSize * 2];
    if (seed != 0) {
        std::vector<unsigned char> F(4);
        UnpackLittleUint32(seed, F);
        for (int i = 0; i < 256; i++) {
            _random[i] = Source[i] ^ F[0];
            _random[i] ^= F[1];
            _random[i] ^= F[2];
            _random[i] ^= F[3];
            _random[i + RandomSize] = _random[i];
        }
    } else {
        for (int i = 0; i < RandomSize; i++)
            _random[i + RandomSize] = _random[i] = Source[i];
    }
}

double Noise::Dot(const int g[3], double x, double y, double z, double t)
{
    return g[0] * x + g[1] * y + g[2] * z + g[3] * t;
}

double Noise::Dot(const int g[3], double x, double y, double z)
{
    return g[0] * x + g[1] * y + g[2] * z;
}

double Noise::Dot(const int g[3], double x, double y)
{
    return g[0] * x + g[1] * y;
}

int Noise::FastFloor(double x)
{
    return x >= 0 ? (int)x : (int)x - 1;
}

std::vector<uint8_t> Noise::UnpackLittleUint32(int value, std::vector<uint8_t>& buffer)
{
    if (buffer.size() < 4)
        buffer.resize(4);

    buffer[0] = static_cast<uint8_t>(value & 0x00ff);
    buffer[1] = static_cast<uint8_t>((value & 0xff00) >> 8);
    buffer[2] = static_cast<uint8_t>((value & 0x00ff0000) >> 16);
    buffer[3] = static_cast<uint8_t>((value & 0xff000000) >> 24);

    return buffer;
}


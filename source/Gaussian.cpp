#include "..\include\VapourSynth.h"
#include "..\include\Gaussian.h"
#include "..\include\Bilateral.h"


void Recursive_Gaussian_Parameters(const double sigma, double & B, double & B1, double & B2, double & B3)
{
    const double q = sigma < 2.5 ? 3.97156 - 4.14554*sqrt(1 - 0.26891*sigma) : 0.98711*sigma - 0.96330;

    const double b0 = 1.57825 + 2.44413*q + 1.4281*q*q + 0.422205*q*q*q;
    const double b1 = 2.44413*q + 2.85619*q*q + 1.26661*q*q*q;
    const double b2 = -(1.4281*q*q + 1.26661*q*q*q);
    const double b3 = 0.422205*q*q*q;

    B = 1 - (b1 + b2 + b3) / b0;
    B1 = b1 / b0;
    B2 = b2 / b0;
    B3 = b3 / b0;
}

void Recursive_Gaussian2D_Vertical(double * output, const double * input, int width, int height, int stride, const double B, const double B1, const double B2, const double B3)
{
    int i, j, lower, upper;
    double P0, P1, P2, P3;
    int pcount = stride*height;

    for (j = 0; j < width; j++)
    {
        lower = j;
        upper = pcount;

        i = lower;
        output[i] = P3 = P2 = P1 = input[i];

        for (i += stride; i < upper; i += stride)
        {
            P0 = B*input[i] + B1*P1 + B2*P2 + B3*P3;
            P3 = P2;
            P2 = P1;
            P1 = P0;
            output[i] = P0;
        }

        i -= stride;
        P3 = P2 = P1 = output[i];

        for (i -= stride; i >= lower; i -= stride)
        {
            P0 = B*output[i] + B1*P1 + B2*P2 + B3*P3;
            P3 = P2;
            P2 = P1;
            P1 = P0;
            output[i] = P0;
        }
    }
}

void Recursive_Gaussian2D_Horizontal(double * output, const double * input, int width, int height, int stride, const double B, const double B1, const double B2, const double B3)
{
    int i, j, lower, upper;
    double P0, P1, P2, P3;

    for (j = 0; j < height; j++)
    {
        lower = stride*j;
        upper = lower + width;

        i = lower;
        output[i] = P3 = P2 = P1 = input[i];

        for (i++; i < upper; i++)
        {
            P0 = B*input[i] + B1*P1 + B2*P2 + B3*P3;
            P3 = P2;
            P2 = P1;
            P1 = P0;
            output[i] = P0;
        }

        i--;
        P3 = P2 = P1 = output[i];

        for (i--; i >= lower; i--)
        {
            P0 = B*output[i] + B1*P1 + B2*P2 + B3*P3;
            P3 = P2;
            P2 = P1;
            P1 = P0;
            output[i] = P0;
        }
    }
}
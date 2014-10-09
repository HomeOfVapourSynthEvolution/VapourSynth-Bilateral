#ifndef BILATERAL_H_
#define BILATERAL_H_


#include "Numeric.h"
#include "Gaussian.h"
#include "VapourSynth.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct BilateralData {
    VSNodeRef *node;
    const VSVideoInfo *vi;
    VSNodeRef *rnode;
    const VSVideoInfo *rvi;
    bool joint;
    double sigmaS[3];
    double sigmaR[3];
    int PBFICnum[3];
    int process[3];

    FLType * GR_LUT[3];

    BilateralData()
    {
        GR_LUT[0] = nullptr;
        GR_LUT[1] = nullptr;
        GR_LUT[2] = nullptr;
    }
};


void VS_CC BilateralCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template < typename T >
void Bilateral2D(VSFrameRef * dst, const VSFrameRef * src, const VSFrameRef * ref, BilateralData * d, const VSAPI * vsapi)
{
    int i, j, k, upper;
    const T *srcp, *refp;
    T *dstp;
    int stride, width, height, pcount;
    int PBFICnum;

    const VSFormat *fi = vsapi->getFrameFormat(src);
    int Floor = 0;
    int Ceil = (1 << fi->bitsPerSample) - 1;
    FLType FloorFL = static_cast<FLType>(Floor);
    FLType CeilFL = static_cast<FLType>(Ceil);

    int plane;
    for (plane = 0; plane < fi->numPlanes; plane++)
    {
        if (d->process[plane])
        {
            srcp = reinterpret_cast<const T *>(vsapi->getReadPtr(src, plane));
            refp = reinterpret_cast<const T *>(vsapi->getReadPtr(ref, plane));
            dstp = reinterpret_cast<T *>(vsapi->getWritePtr(dst, plane));
            stride = vsapi->getStride(src, plane) / sizeof(T);
            width = vsapi->getFrameWidth(src, plane);
            height = vsapi->getFrameHeight(src, plane);
            pcount = stride * height;
            PBFICnum = d->PBFICnum[plane];

            // Value range of Plane "ref"
            T rLower, rUpper, rRange;

            rLower = Floor;
            rUpper = Ceil;
            rRange = rUpper - rLower;

            // Generate quantized PBFICs' parameters
            T * PBFICk = new T[PBFICnum];

            for (k = 0; k < PBFICnum; k++)
            {
                PBFICk[k] = static_cast<T>(static_cast<double>(rRange)*k / (PBFICnum - 1) + rLower + 0.5);
            }

            // Generate recursive Gaussian parameters
            FLType B, B1, B2, B3;
            Recursive_Gaussian_Parameters(d->sigmaS[plane], B, B1, B2, B3);

            // Generate quantized PBFICs
            FLType * Wk = new FLType[pcount];
            FLType * Jk = new FLType[pcount];
            FLType ** PBFIC = new FLType*[PBFICnum];

            for (k = 0; k < PBFICnum; k++)
            {
                PBFIC[k] = new FLType[pcount];

                for (j = 0; j < height; j++)
                {
                    i = stride * j;
                    for (upper = i + width; i < upper; i++)
                    {
                        Wk[i] = Gaussian_Distribution2D_Range_LUT_Lookup(d->GR_LUT[plane], PBFICk[k], refp[i]);
                        Jk[i] = Wk[i] * static_cast<FLType>(srcp[i]);
                    }
                }

                Recursive_Gaussian2D_Horizontal(Wk, Wk, width, height, stride, B, B1, B2, B3);
                Recursive_Gaussian2D_Vertical(Wk, Wk, width, height, stride, B, B1, B2, B3);
                Recursive_Gaussian2D_Horizontal(Jk, Jk, width, height, stride, B, B1, B2, B3);
                Recursive_Gaussian2D_Vertical(Jk, Jk, width, height, stride, B, B1, B2, B3);

                for (j = 0; j < height; j++)
                {
                    i = stride * j;
                    for (upper = i + width; i < upper; i++)
                    {
                        PBFIC[k][i] = Wk[i] == 0 ? 0 : Jk[i] / Wk[i];
                    }
                }
            }

            // Generate filtered result from PBFICs using linear interpolation
            for (j = 0; j < height; j++)
            {
                i = stride * j;
                for (upper = i + width; i < upper; i++)
                {
                    for (k = 0; k < PBFICnum - 2; k++)
                    {
                        if (refp[i] < PBFICk[k + 1] && refp[i] >= PBFICk[k]) break;
                    }

                    dstp[i] = static_cast<T>(Clip(((PBFICk[k + 1] - refp[i])*PBFIC[k][i] + (refp[i] - PBFICk[k])*PBFIC[k + 1][i]) / (PBFICk[k + 1] - PBFICk[k]) + FLType(0.5), FloorFL, CeilFL));
                }
            }

            // Clear
            for (k = 0; k < PBFICnum; k++)
                delete[] PBFIC[k];
            delete[] PBFIC;
            delete[] Jk;
            delete[] Wk;
            delete[] PBFICk;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#endif
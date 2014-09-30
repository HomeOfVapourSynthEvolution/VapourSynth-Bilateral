#include "..\include\VapourSynth.h"
#include "..\include\VSHelper.h"
#include "..\include\VSScript.h"
#include "..\include\Numeric.h"
#include "..\include\Bilateral.h"


static void VS_CC BilateralInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    BilateralData *d = (BilateralData *)* instanceData;

    vsapi->setVideoInfo(d->vi, 1, node);
}

static const VSFrameRef *VS_CC BilateralGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    BilateralData *d = (BilateralData *)* instanceData;

    if (activationReason == arInitial)
    {
        vsapi->requestFrameFilter(n, d->node, frameCtx);

        for (int i = 0; i < 3; i++)
        {
            if (d->process[i])
            {
                Gaussian_Function_Range_LUT_Free(d->GR_LUT[i]);
                d->GR_LUT[i] = Gaussian_Function_Range_LUT_Generation((1 << d->vi->format->bitsPerSample) - 1, d->sigmaR[i]);
            }
        }
    }
    else if (activationReason == arAllFramesReady)
    {
        const VSFrameRef *src = vsapi->getFrameFilter(n, d->node, frameCtx);
        const VSFormat *fi = vsapi->getFrameFormat(src);
        int width = vsapi->getFrameWidth(src, 0);
        int height = vsapi->getFrameHeight(src, 0);
        const int planes[] = { 0, 1, 2 };
        const VSFrameRef * cp_planes[] = { d->process[0] ? nullptr : src, d->process[1] ? nullptr : src, d->process[2] ? nullptr : src };
        VSFrameRef *dst = vsapi->newVideoFrame2(fi, width, height, cp_planes, planes, src, core);

        const VSFrameRef *ref = d->joint ? vsapi->getFrameFilter(n, d->rnode, frameCtx) : src;
        const VSFormat *rfi = d->joint ? vsapi->getFrameFormat(ref) : fi;
        
        if (d->vi->format->bytesPerSample == 1)
        {
            Bilateral2D<uint8_t>(dst, src, ref, d, vsapi);
        }
        else if (d->vi->format->bytesPerSample == 2)
        {
            Bilateral2D<uint16_t>(dst, src, ref, d, vsapi);
        }

        vsapi->freeFrame(src);
        if (d->joint) vsapi->freeFrame(ref);

        return dst;
    }

    return nullptr;
}

static void VS_CC BilateralFree(void *instanceData, VSCore *core, const VSAPI *vsapi)
{
    BilateralData *d = (BilateralData *)instanceData;

    for (int i = 0; i < 3; i++)
    {
        if (d->process[i])
        {
            Gaussian_Function_Range_LUT_Free(d->GR_LUT[i]);
        }
    }

    vsapi->freeNode(d->node);
    if (d->joint) vsapi->freeNode(d->rnode);

    delete d;
}

void VS_CC BilateralCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    BilateralData d;
    int error;
    int i, n, m, o;

    d.node = vsapi->propGetNode(in, "input", 0, nullptr);
    d.vi = vsapi->getVideoInfo(d.node);

    if (!d.vi->format)
    {
        vsapi->freeNode(d.node);
        vsapi->setError(out, "Bilateral: Invalid input clip, Only constant format input supported");
        return;
    }
    if (d.vi->format->sampleType != stInteger || (d.vi->format->bytesPerSample != 1 && d.vi->format->bytesPerSample != 2))
    {
        vsapi->freeNode(d.node);
        vsapi->setError(out, "Bilateral: Invalid input clip, Only 8-16 bit int formats supported");
        return;
    }

    d.rnode = vsapi->propGetNode(in, "ref", 0, &error);
    if (error)
    {
        d.joint = false;
    }
    else
    {
        d.rvi = vsapi->getVideoInfo(d.rnode);
        d.joint = true;

        if (!d.rvi->format)
        {
            vsapi->freeNode(d.node);
            if (d.joint) vsapi->freeNode(d.rnode);
            vsapi->setError(out, "Bilateral: Invalid clip \"ref\", Only constant format input supported");
            return;
        }
        if (d.rvi->format->sampleType != stInteger || (d.rvi->format->bytesPerSample != 1 && d.rvi->format->bytesPerSample != 2))
        {
            vsapi->freeNode(d.node);
            if (d.joint) vsapi->freeNode(d.rnode);
            vsapi->setError(out, "Bilateral: Invalid clip \"ref\", Only 8-16 bit int formats supported");
            return;
        }
        if (d.vi->width != d.rvi->width || d.vi->height != d.rvi->height)
        {
            vsapi->freeNode(d.node);
            if (d.joint) vsapi->freeNode(d.rnode);
            vsapi->setError(out, "Bilateral: input clip and clip \"ref\" must be of the same size");
            return;
        }
        if (d.vi->format->colorFamily != d.rvi->format->colorFamily)
        {
            vsapi->freeNode(d.node);
            if (d.joint) vsapi->freeNode(d.rnode);
            vsapi->setError(out, "Bilateral: input clip and clip \"ref\" must be of the same color family");
            return;
        }
        if (d.vi->format->subSamplingH != d.rvi->format->subSamplingH || d.vi->format->subSamplingW != d.rvi->format->subSamplingW)
        {
            vsapi->freeNode(d.node);
            if (d.joint) vsapi->freeNode(d.rnode);
            vsapi->setError(out, "Bilateral: input clip and clip \"ref\" must be of the same subsampling");
            return;
        }
        if (d.vi->format->bitsPerSample != d.rvi->format->bitsPerSample)
        {
            vsapi->freeNode(d.node);
            if (d.joint) vsapi->freeNode(d.rnode);
            vsapi->setError(out, "Bilateral: input clip and clip \"ref\" must be of the same bit depth");
            return;
        }
    }

    bool chroma = d.vi->format->colorFamily == cmYUV || d.vi->format->colorFamily == cmYCoCg;

    m = vsapi->propNumElements(in, "sigmaS");
    for (i = 0; i < 3; i++)
    {
        if (i < m)
        {
            d.sigmaS[i] = vsapi->propGetFloat(in, "sigmaS", i, nullptr);
        }
        else if (i == 0)
        {
            d.sigmaS[i] = 5.0;
        }
        else if (i > 0 && chroma && d.vi->format->subSamplingH && d.vi->format->subSamplingW) // Reduce sigmaS for sub-sampled chroma planes by default
        {
            d.sigmaS[i] = d.sigmaS[i - 1] / std::sqrt((1 << d.vi->format->subSamplingH)*(1 << d.vi->format->subSamplingH));
        }
        else
        {
            d.sigmaS[i] = d.sigmaS[i - 1];
        }

        if (d.sigmaS[i] < 0)
        {
            vsapi->freeNode(d.node);
            if (d.joint) vsapi->freeNode(d.rnode);
            vsapi->setError(out, "Bilateral: Invalid \"sigmaS\" specified, must be non-negative float number");
            return;
        }
    }

    m = vsapi->propNumElements(in, "sigmaR");
    for (i = 0; i < 3; i++)
    {
        if (i < m)
        {
            d.sigmaR[i] = vsapi->propGetFloat(in, "sigmaR", i, nullptr);
        }
        else if (i == 0)
        {
            d.sigmaR[i] = 0.04;
        }
        else
        {
            d.sigmaR[i] = d.sigmaR[i - 1];
        }

        if (d.sigmaR[i] < 0)
        {
            vsapi->freeNode(d.node);
            if (d.joint) vsapi->freeNode(d.rnode);
            vsapi->setError(out, "Bilateral: Invalid \"sigmaR\" specified, must be non-negative float number");
            return;
        }
    }

    m = vsapi->propNumElements(in, "PBFICnum");
    for (i = 0; i < 3; i++)
    {
        if (i < m)
        {
            d.PBFICnum[i] = int64ToIntS(vsapi->propGetInt(in, "PBFICnum", i, nullptr));
        }
        else if (i > 0 && chroma)
        {
            if (d.sigmaR[i] >= 0.08)
            {
                d.PBFICnum[i] = 4;
            }
            else
            {
                d.PBFICnum[i] = Min(256, static_cast<int>(4 * 0.08 / d.sigmaR[i] + 0.5));
            }
            if (d.PBFICnum[i] % 2 == 0 && d.PBFICnum[i] < 256) // Set odd PBFIC number to chroma planes by default
                d.PBFICnum[i]++;
        }
        else
        {
            if (d.sigmaR[i] >= 0.08)
            {
                d.PBFICnum[i] = 4;
            }
            else
            {
                d.PBFICnum[i] = Min(256, static_cast<int>(4 * 0.08 / d.sigmaR[i] + 0.5));
            }
        }

        if (d.PBFICnum[i] < 2 || d.PBFICnum[i] > 256)
        {
            vsapi->freeNode(d.node);
            if (d.joint) vsapi->freeNode(d.rnode);
            vsapi->setError(out, "Bilateral: Invalid \"PBFICnum\" specified, valid range is integer in [2,256]");
            return;
        }
    }

    n = d.vi->format->numPlanes;
    m = vsapi->propNumElements(in, "planes");
    for (i = 0; i < 3; i++)
    {
        if (i > 0 && chroma) // Chroma planes are not processed by default
            d.process[i] = 0;
        else
            d.process[i] = m <= 0;
    }
    for (i = 0; i < m; i++) {
        o = int64ToIntS(vsapi->propGetInt(in, "planes", i, nullptr));
        if (o < 0 || o >= n)
            vsapi->setError(out, "Bilateral: plane index out of range");
        if (d.process[o])
            vsapi->setError(out, "Bilateral: plane specified twice");
        d.process[o] = 1;
    }
    for (i = 0; i < 3; i++)
    {
        if (d.sigmaS[i] == 0 || d.sigmaR[i] == 0)
            d.process[i] = 0;
    }

    BilateralData *data = new BilateralData(d);

    vsapi->createFilter(in, out, "Bilateral", BilateralInit, BilateralGetFrame, BilateralFree, fmParallel, 0, data, core);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
    configFunc("mawen1250.Bilateral", "bilateral",
        "O(1) bilateral filter based on PBFIC.",
        VAPOURSYNTH_API_VERSION, 1, plugin);

    registerFunc("Bilateral", "input:clip;ref:clip:opt;sigmaS:float[]:opt;sigmaR:float[]:opt;PBFICnum:int[]:opt;planes:int[]:opt", BilateralCreate, nullptr, plugin);
}

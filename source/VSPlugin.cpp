#include "..\include\VapourSynth.h"
#include "..\include\Bilateral.h"
#include "..\include\Gaussian.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
    configFunc("mawen1250.Bilateral", "bilateral",
        "O(1) bilateral filter based on PBFIC.",
        VAPOURSYNTH_API_VERSION, 1, plugin);

    registerFunc("Bilateral", "input:clip;ref:clip:opt;sigmaS:float[]:opt;sigmaR:float[]:opt;PBFICnum:int[]:opt;planes:int[]:opt", BilateralCreate, nullptr, plugin);
    registerFunc("Gaussian", "input:clip;sigma:float[]:opt;sigmaV:float[]:opt", GaussianCreate, nullptr, plugin);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

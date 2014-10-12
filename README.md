# VapourSynth-Bilateral

Bilateral.dll 2014 mawen1250

VapourSynth plugin
namespace: bilateral
functions: Bilateral, Gaussian

## Bilateral

### Description

Bilateral filter, can be used to perform spatial de-noise, spatial smoothing while preserving edges.
Larger spatial sigma results in larger smoothing radius, while smaller range sigma preserves edges better.
Now there're 2 different algorithms implemented in this function, algorithm=1 is suitable for large sigmaS and large sigmaR, and algorithm=2 is suitable for small sigmaS and small sigmaR. By default, algorithm=0 will choose the appropriate algorithm to perform the filtering.
If clip ref is specified, it will perform joint/cross Bilateral filtering, which means clip ref is used to determine the range weight of Bilateral filter.
clip input should be of 8-16bit integer, and clip ref should be of the same format with clip input.
By default, this function will only process Y plane for YUV format, and process all the planes for other formats.

### Usage

bilateral.Bilateral(clip input, clip ref=input, float[] sigmaS=3.0, float[] sigmaR=0.02, int[] planes=[], int[] algorithm=0, int[] PBFICnum=[])

- input: clip to process

- ref: Reference clip to calculate range weight (Default: input)
Specify it if you want to perform joint/cross Bilateral filter.

- sigmaS: sigma of Gaussian function to calculate spatial weight. (Default: 3.0)
The scale of this parameter is equivalent to pixel distance.
Larger sigmaS results in larger filtering radius as well as stronger smoothing.
Use an array to specify sigmaS for each plane. If sigmaS for the second plane is not specified, it will be set according to the sigmaS of first plane and sub-sampling.
algorithm=1: It is of constant processing time regardless of sigmaS, while in small sigmaS the smoothing effect is stronger compared to Bilateral filter prototype.
algorithm=2: It will be slower as sigmaS increases, and for large sigmaS the approximation will be bad compared to Bilateral filter prototype.

- sigmaR: sigma of Gaussian function to calculate range weight. (Default: 0.02)
The scale of this parameter is the same as pixel value ranging in [0,1].
Smaller sigmaR preserves edges better, may also leads to weaker smoothing.
Use an array to specify sigmaR for each plane, otherwise the same sigmaR is used for all the planes.
algorithm=1: As sigmaR decreases, the approximation of this algorithm gets worse, so more PBFICs should be used to produce satisfying result. If PBFICnum is not assigned, the number of PBFICs used will be set according to sigmaR.
algorithm=2: It is of constant processing time regardless of sigmaR, while for large sigmaR the approximation will be bad compared to Bilateral filter prototype.

- planes: An array to specify which planes to process. By default, chroma planes is not processed.

- algorithm: (Default: 1)
0 = Automatically determine the algorithm according to sigmaS, sigmaR and PBFICnum.
1 = O(1) Bilateral filter uses quantized PBFICs. (IMO it should be O(PBFICnum))
2 = Bilateral filter with truncated spatial window and sub-sampling. O(sigmaS^2)

- PBFICnum: number of PBFICs used in algorithm=1
Default: 4 when sigmaR>=0.08. It will increase as sigmaR decreases, upper to 32. For chroma plane default value will be odd to better preserve neutral value of chromiance.
Available range is [2,256].
Use an array to specify PBFICnum for each plane.

## Gaussian

### Description

Recursive implementation of Gaussian filter, which is of constant processing time regardless of sigma.
The kernel of this function is also used to implement the algorithm of bilateral.Bilateral(algorithm=1).

### Usage

bilateral.Gaussian(clip input, float[] sigma=3.0, float[] sigmaV=sigma)

- input: clip to process

- sigma: sigma of Gaussian function to calculate spatial weight for horizontal filtering. (Default: 3.0)
Use an array to specify sigma for each plane. If sigma for the second plane is not specified, it will be set according to the sigma of first plane and horizontal sub-sampling.
Larger sigma results in larger filtering radius as well as stronger blurring in horizontal direction.

- sigmaV: sigma of Gaussian function to calculate spatial weight for vertical filtering. (Default: sigma)
Use an array to specify sigmaV for each plane. If sigmaV for the second plane is not specified, it will be set according to the sigmaV of first plane and vertical sub-sampling.
Larger sigmaV results in larger filtering radius as well as stronger blurring in vertical direction.

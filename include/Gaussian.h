#ifndef GAUSSIAN_H_
#define GAUSSIAN_H_


#include <iostream>
#include <cmath>
#include "Numeric.h"
#include "VapourSynth.h"


const double Pi = 3.1415926535897932384626433832795;
const double sqrt_2Pi = sqrt(2 * Pi);

const FLType sigmaRMul = 30.;


void Recursive_Gaussian_Parameters(const double sigma, FLType & B, FLType & B1, FLType & B2, FLType & B3);
void Recursive_Gaussian2D_Vertical(FLType * output, const FLType * input, int width, int height, int stride, const FLType B, const FLType B1, const FLType B2, const FLType B3);
void Recursive_Gaussian2D_Horizontal(FLType * output, const FLType * input, int width, int height, int stride, const FLType B, const FLType B1, const FLType B2, const FLType B3);


inline double Gaussian_Function(double x, double sigma)
{
    x /= sigma;
    return std::exp(x*x / -2);
}

inline double Gaussian_Function_sqr_x(double sqr_x, double sigma)
{
    return std::exp(sqr_x / (sigma*sigma*-2));
}

inline double Normalized_Gaussian_Function(double x, double sigma)
{
    x /= sigma;
    return std::exp(x*x / -2) / (sqrt_2Pi*sigma);
}

inline double Normalized_Gaussian_Function_sqr_x(double sqr_x, double sigma)
{
    return std::exp(sqr_x / (sigma*sigma*-2)) / (sqrt_2Pi*sigma);
}


inline FLType * Gaussian_Function_Range_LUT_Generation(const int ValueRange, double sigmaR)
{
    int i;
    int Levels = ValueRange + 1;
    sigmaR *= ValueRange;
    const int upper = Min(ValueRange, static_cast<int>(sigmaR*sigmaRMul + 0.5));
    FLType * GR_LUT = new FLType[Levels];

    for (i = 0; i <= upper; i++)
    {
        GR_LUT[i] = static_cast<FLType>(Normalized_Gaussian_Function(static_cast<double>(i), sigmaR));
    }
    // For unknown reason, when more range weights equal 0, the runtime speed gets lower - mainly in function Recursive_Gaussian2D_Horizontal.
    // To avoid this problem, we set range weights whose range values are larger than sigmaR*sigmaRMul to the Gaussian function value at sigmaR*sigmaRMul.
    if (i < Levels)
    {
        const FLType upperLUTvalue = GR_LUT[upper];
        for (; i < Levels; i++)
        {
            GR_LUT[i] = upperLUTvalue;
        }
    }

    return GR_LUT;
}

template < typename T >
inline FLType Gaussian_Distribution2D_Range_LUT_Lookup(const FLType * GR_LUT, const T Value1, const T Value2)
{
    return GR_LUT[Value1 > Value2 ? Value1 - Value2 : Value2 - Value1];
}

inline void Gaussian_Function_Range_LUT_Free(FLType * GR_LUT)
{
    delete[] GR_LUT;
}


#endif
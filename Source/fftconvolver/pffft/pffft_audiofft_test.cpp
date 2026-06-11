// ==================================================================================
// Standalone unit check for the pffft AudioFFT backend (Task 3b of the
// convolution optimization spec). NOT part of the plugin build.
//
// Verifies, for sizes {256, 1024, 16384}:
//   (a) ifft(fft(x)) == x to 1e-5 (the AudioFFT contract the convolver uses)
//   (b) fft output matches a naive O(N^2) reference DFT to 1e-3 relative
//       error (the Ooura/Accelerate/FFTW3 impls all match the standard DFT,
//       so matching it guarantees bin-for-bin backend equivalence).
//
// Build & run (x64 Native Tools Command Prompt for VS):
//   cl /O2 /EHsc /DAUDIOFFT_PFFFT=1 /I.. pffft_audiofft_test.cpp ..\AudioFFT.cpp pffft.c pffft_common.c
//   pffft_audiofft_test.exe
// ==================================================================================

#include "../AudioFFT.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace
{
    // Standard DFT: X[k] = sum_n x[n] * e^{-2*pi*i*n*k/N}
    void naiveDFT(const std::vector<float>& x, std::vector<double>& re, std::vector<double>& im)
    {
        const size_t n = x.size();
        const size_t bins = n / 2 + 1;
        re.assign(bins, 0.0);
        im.assign(bins, 0.0);
        const double twoPi = 6.283185307179586476925286766559;
        for (size_t k = 0; k < bins; ++k)
        {
            double sr = 0.0, si = 0.0;
            for (size_t i = 0; i < n; ++i)
            {
                const double ph = twoPi * (double)i * (double)k / (double)n;
                sr += (double)x[i] * std::cos(ph);
                si -= (double)x[i] * std::sin(ph);
            }
            re[k] = sr;
            im[k] = si;
        }
    }

    bool testSize(size_t n)
    {
        std::srand(12345);
        std::vector<float> x(n);
        for (auto& v : x)
            v = 2.0f * (float)std::rand() / (float)RAND_MAX - 1.0f;

        const size_t bins = audiofft::AudioFFT::ComplexSize(n);
        std::vector<float> re(bins), im(bins), y(n);

        audiofft::AudioFFT fft;
        fft.init(n);
        fft.fft(x.data(), re.data(), im.data());
        fft.ifft(y.data(), re.data(), im.data());

        // (a) roundtrip
        double maxRt = 0.0;
        for (size_t i = 0; i < n; ++i)
            maxRt = std::max(maxRt, (double)std::fabs(y[i] - x[i]));
        const bool rtOk = maxRt < 1.0e-5;

        // (b) match naive DFT (relative to spectrum peak magnitude)
        std::vector<double> rRe, rIm;
        naiveDFT(x, rRe, rIm);
        double peak = 0.0;
        for (size_t k = 0; k < bins; ++k)
            peak = std::max(peak, std::sqrt(rRe[k]*rRe[k] + rIm[k]*rIm[k]));
        double maxRel = 0.0;
        for (size_t k = 0; k < bins; ++k)
        {
            maxRel = std::max(maxRel, std::fabs((double)re[k] - rRe[k]) / peak);
            maxRel = std::max(maxRel, std::fabs((double)im[k] - rIm[k]) / peak);
        }
        const bool dftOk = maxRel < 1.0e-3;

        std::printf("N=%6zu  roundtrip max err %.3e (%s)   vs naive DFT max rel err %.3e (%s)\n",
                    n, maxRt, rtOk ? "PASS" : "FAIL", maxRel, dftOk ? "PASS" : "FAIL");
        return rtOk && dftOk;
    }
}

int main()
{
    bool ok = true;
    ok &= testSize(256);
    ok &= testSize(1024);
    ok &= testSize(16384);
    std::printf(ok ? "ALL PASS\n" : "FAILURES\n");
    return ok ? 0 : 1;
}

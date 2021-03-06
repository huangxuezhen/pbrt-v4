// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#include <pbrt/pbrt.h>

#include <pbrt/gpu/pathintegrator.h>
#include <pbrt/samplers.h>

#include <type_traits>

#ifdef PBRT_GPU_DBG
#ifndef TO_STRING
#define TO_STRING(x) TO_STRING2(x)
#define TO_STRING2(x) #x
#endif  // !TO_STRING
#define DBG(...) printf(__FILE__ ":" TO_STRING(__LINE__) ": " __VA_ARGS__)
#else
#define DBG(...)
#endif  // PBRT_GPU_DBG

namespace pbrt {

template <typename Sampler>
void GPUPathIntegrator::GenerateRaySamples(int depth, int sampleIndex) {
    std::string desc = std::string("Generate ray samples - ") + Sampler::Name();

    ForAllQueued(desc.c_str(), rayQueues[depth & 1], maxQueueSize,
                 [=] PBRT_GPU(const RayWorkItem w, int index) {
                     // Figure out how many dimensions have been consumed so far: 5
                     // are used for the initial camera sample and then either 7 or
                     // 10 per ray, depending on whether there's subsurface
                     // scattering.
                     int dimension = 5 + 7 * depth;
                     if (haveSubsurface)
                         dimension += 3 * depth;

                     // Initialize a Sampler
                     Sampler pixelSampler = *sampler.Cast<Sampler>();
                     Point2i pPixel = pixelSampleState.pPixel[w.pixelIndex];
                     pixelSampler.StartPixelSample(pPixel, sampleIndex, dimension);

                     // Generate the samples for the ray and store them with it in
                     // the ray queue.
                     RaySamples rs;
                     rs.direct.u = pixelSampler.Get2D();
                     rs.direct.uc = pixelSampler.Get1D();
                     rs.indirect.u = pixelSampler.Get2D();
                     rs.indirect.uc = pixelSampler.Get1D();
                     rs.indirect.rr = pixelSampler.Get1D();
                     rs.haveSubsurface = haveSubsurface;
                     if (haveSubsurface) {
                         rs.subsurface.uc = pixelSampler.Get1D();
                         rs.subsurface.u = pixelSampler.Get2D();
                     }

                     rayQueues[depth & 1]->raySamples[index] = rs;
                 });
}

void GPUPathIntegrator::GenerateRaySamples(int depth, int sampleIndex) {
    auto generateSamples = [=](auto sampler) {
        using Sampler = std::remove_reference_t<decltype(*sampler)>;
        if constexpr (!std::is_same_v<Sampler, MLTSampler> &&
                      !std::is_same_v<Sampler, DebugMLTSampler>)
            GenerateRaySamples<Sampler>(depth, sampleIndex);
    };
    // Call the appropriate GenerateRaySamples specialization based on the
    // Sampler's actual type.
    sampler.DispatchCPU(generateSamples);
}

}  // namespace pbrt

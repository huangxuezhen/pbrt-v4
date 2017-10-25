
/*
    pbrt source code is Copyright(c) 1998-2016
                        Matt Pharr, Greg Humphreys, and Wenzel Jakob.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

// integrators/ao.cpp*
#include <pbrt/integrators/ao.h>

#include <pbrt/core/error.h>
#include <pbrt/core/sampling.h>
#include <pbrt/core/sampler.h>
#include <pbrt/core/interaction.h>
#include <pbrt/core/paramset.h>
#include <pbrt/core/camera.h>
#include <pbrt/core/film.h>
#include <pbrt/core/scene.h>


namespace pbrt {

// AOIntegrator Method Definitions
AOIntegrator::AOIntegrator(bool cosSample, int ns,
                           std::shared_ptr<const Camera> camera,
                           std::unique_ptr<Sampler> sampler,
                           const Bounds2i &pixelBounds)
    : SamplerIntegrator(camera, std::move(sampler), pixelBounds),
      cosSample(cosSample) {
    nSamples = sampler->RoundCount(ns);
    if (ns != nSamples)
        Warning("Taking %d samples, not %d as specified", nSamples, ns);
    sampler->Request2DArray(nSamples);
}

Spectrum AOIntegrator::Li(const RayDifferential &r, const Scene &scene,
                          Sampler &sampler, MemoryArena &arena,
                          int depth) const {
    ProfilePhase p(Prof::SamplerIntegratorLi);
    Spectrum L(0.f);
    RayDifferential ray(r);

    // Intersect _ray_ with scene and store intersection in _isect_
    SurfaceInteraction isect;
 retry:
    if (scene.Intersect(ray, &isect)) {
        isect.ComputeScatteringFunctions(ray, arena);
        if (!isect.bsdf) {
            VLOG(2) << "Skipping intersection due to null bsdf";
            ray = isect.SpawnRay(ray.d);
            goto retry;
        }

        // Compute coordinate frame based on true geometry, not shading
        // geometry.
        Normal3f n = Faceforward(isect.n, -ray.d);
        Vector3f s = Normalize(isect.dpdu);
        Vector3f t = Cross(isect.n, s);

        absl::Span<const Point2f> u = sampler.Get2DArray(nSamples);
        for (int i = 0; i < nSamples; ++i) {
            Vector3f wi;
            Float pdf;
            if (cosSample) {
                wi = CosineSampleHemisphere(u[i]);
                pdf = CosineHemispherePdf(std::abs(wi.z));
            } else {
                wi = UniformSampleHemisphere(u[i]);
                pdf = UniformHemispherePdf();
            }

            // Transform wi from local frame to world space.
            wi = Vector3f(s.x * wi.x + t.x * wi.y + n.x * wi.z,
                          s.y * wi.x + t.y * wi.y + n.y * wi.z,
                          s.z * wi.x + t.z * wi.y + n.z * wi.z);

            if (!scene.IntersectP(isect.SpawnRay(wi)))
                L += Dot(wi, n) / (pdf * nSamples);
        }
    }
    return L;
}

std::unique_ptr<AOIntegrator> CreateAOIntegrator(
    const ParamSet &params, std::unique_ptr<Sampler> sampler,
    std::shared_ptr<const Camera> camera) {
    int np;
    absl::Span<const int> pb = params.GetIntArray("pixelbounds");
    Bounds2i pixelBounds = camera->film->GetSampleBounds();
    if (!pb.empty()) {
        if (pb.size() != 4)
            Error("Expected four values for \"pixelbounds\" parameter. Got %d.",
                  (int)pb.size());
        else {
            pixelBounds = Intersect(pixelBounds,
                                    Bounds2i{{pb[0], pb[2]}, {pb[1], pb[3]}});
            if (pixelBounds.Empty())
                Error("Degenerate \"pixelbounds\" specified.");
        }
    }
    Float rrThreshold = params.GetOneFloat("rrthreshold", 1.);
    bool cosSample = params.GetOneBool("cossample", "true");
    int nSamples = params.GetOneInt("nsamples", 64);
    return std::make_unique<AOIntegrator>(cosSample, nSamples, camera,
                                          std::move(sampler), pixelBounds);
}

}  // namespace pbrt
// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PBRT_BASE_LIGHT_H
#define PBRT_BASE_LIGHT_H

#include <pbrt/pbrt.h>

#include <pbrt/base/medium.h>
#include <pbrt/base/shape.h>
#include <pbrt/util/pstd.h>
#include <pbrt/util/taggedptr.h>

#include <string>

namespace pbrt {

// LightType Definition
enum class LightType : int { DeltaPosition, DeltaDirection, Area, Infinite };

// LightSamplingMode Definition
enum class LightSamplingMode { WithMIS, WithoutMIS };

class PointLight;
class DistantLight;
class ProjectionLight;
class GoniometricLight;
class DiffuseAreaLight;
class UniformInfiniteLight;
class ImageInfiniteLight;
class PortalImageInfiniteLight;
class SpotLight;

class LightSampleContext;
struct LightBounds;
struct LightLiSample;
struct LightLeSample;

// LightHandle Definition
class LightHandle
    : public TaggedPointer<PointLight, DistantLight, ProjectionLight, GoniometricLight,
                           SpotLight, DiffuseAreaLight, UniformInfiniteLight,
                           ImageInfiniteLight, PortalImageInfiniteLight> {
  public:
    // Light Interface
    using TaggedPointer::TaggedPointer;

    static LightHandle Create(const std::string &name,
                              const ParameterDictionary &parameters,
                              const Transform &renderFromLight,
                              const CameraTransform &cameraTransform,
                              MediumHandle outsideMedium, const FileLoc *loc,
                              Allocator alloc);
    static LightHandle CreateArea(const std::string &name,
                                  const ParameterDictionary &parameters,
                                  const Transform &renderFromLight,
                                  const MediumInterface &mediumInterface,
                                  const ShapeHandle shape, const FileLoc *loc,
                                  Allocator alloc);

    void Preprocess(const Bounds3f &sceneBounds);

    PBRT_CPU_GPU inline LightType Type() const;

    SampledSpectrum Phi(const SampledWavelengths &lambda) const;

    PBRT_CPU_GPU inline LightLiSample SampleLi(
        LightSampleContext ctx, Point2f u, SampledWavelengths lambda,
        LightSamplingMode mode = LightSamplingMode::WithoutMIS) const;

    PBRT_CPU_GPU inline Float PDF_Li(
        LightSampleContext ctx, Vector3f wi,
        LightSamplingMode mode = LightSamplingMode::WithoutMIS) const;

    std::string ToString() const;

    // AreaLights only
    PBRT_CPU_GPU inline SampledSpectrum L(const Point3f &p, const Normal3f &n,
                                          const Point2f &uv, const Vector3f &w,
                                          const SampledWavelengths &lambda) const;
    PBRT_CPU_GPU
    void PDF_Le(const Interaction &intr, Vector3f &w, Float *pdfPos, Float *pdfDir) const;

    // InfiniteAreaLights only
    PBRT_CPU_GPU inline SampledSpectrum Le(const Ray &ray,
                                           const SampledWavelengths &lambda) const;

    LightBounds Bounds() const;

    PBRT_CPU_GPU
    LightLeSample SampleLe(const Point2f &u1, const Point2f &u2,
                           SampledWavelengths &lambda, Float time) const;

    // Note shouldn't be called for area lights..
    PBRT_CPU_GPU
    void PDF_Le(const Ray &ray, Float *pdfPos, Float *pdfDir) const;
};

}  // namespace pbrt

#endif  // PBRT_BASE_LIGHT_H

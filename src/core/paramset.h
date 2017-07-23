
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

#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_CORE_PARAMSET_H
#define PBRT_CORE_PARAMSET_H

// core/paramset.h*
#include "pbrt.h"

#include "geometry.h"
#include "texture.h"
#include "spectrum.h"
#include "ext/google/array_slice.h"

#include <string>
#include <memory>
#include <vector>
#include <map>

namespace pbrt {

// ParamSet Declarations
class ParamSet {
  public:
    // ParamSet Public Methods
    ParamSet() {}
    void AddFloat(const std::string &, std::unique_ptr<Float[]> v,
                  int nValues = 1);
    void AddInt(const std::string &, std::unique_ptr<int[]> v, int nValues);
    void AddBool(const std::string &, std::unique_ptr<bool[]> v, int nValues);
    void AddPoint2f(const std::string &, std::unique_ptr<Point2f[]> v,
                    int nValues);
    void AddVector2f(const std::string &, std::unique_ptr<Vector2f[]> v,
                     int nValues);
    void AddPoint3f(const std::string &, std::unique_ptr<Point3f[]> v,
                    int nValues);
    void AddVector3f(const std::string &, std::unique_ptr<Vector3f[]> v,
                     int nValues);
    void AddNormal3f(const std::string &, std::unique_ptr<Normal3f[]> v,
                     int nValues);
    void AddString(const std::string &, std::unique_ptr<std::string[]> v,
                   int nValues);
    void AddTexture(const std::string &, const std::string &);
    void AddRGBSpectrum(const std::string &, std::unique_ptr<Float[]> v,
                        int nValues);
    void AddXYZSpectrum(const std::string &, std::unique_ptr<Float[]> v,
                        int nValues);
    void AddBlackbodySpectrum(const std::string &, std::unique_ptr<Float[]> v,
                              int nValues);
    void AddSampledSpectrumFiles(const std::string &, const char **,
                                 int nValues);
    void AddSampledSpectrum(const std::string &, std::unique_ptr<Float[]> v,
                            int nValues);
    bool EraseInt(const std::string &);
    bool EraseBool(const std::string &);
    bool EraseFloat(const std::string &);
    bool ErasePoint2f(const std::string &);
    bool EraseVector2f(const std::string &);
    bool ErasePoint3f(const std::string &);
    bool EraseVector3f(const std::string &);
    bool EraseNormal3f(const std::string &);
    bool EraseSpectrum(const std::string &);
    bool EraseString(const std::string &);
    bool EraseTexture(const std::string &);
    Float FindOneFloat(const std::string &, Float d) const;
    int FindOneInt(const std::string &, int d) const;
    bool FindOneBool(const std::string &, bool d) const;
    Point2f FindOnePoint2f(const std::string &, const Point2f &d) const;
    Vector2f FindOneVector2f(const std::string &, const Vector2f &d) const;
    Point3f FindOnePoint3f(const std::string &, const Point3f &d) const;
    Vector3f FindOneVector3f(const std::string &, const Vector3f &d) const;
    Normal3f FindOneNormal3f(const std::string &, const Normal3f &d) const;
    Spectrum FindOneSpectrum(const std::string &, const Spectrum &d) const;
    std::string FindOneString(const std::string &, const std::string &d) const;
    std::string FindOneFilename(const std::string &,
                                const std::string &d) const;
    std::string FindTexture(const std::string &) const;
    gtl::ArraySlice<Float> FindFloat(const std::string &) const;
    gtl::ArraySlice<int> FindInt(const std::string &) const;
    gtl::ArraySlice<bool> FindBool(const std::string &) const;
    gtl::ArraySlice<Point2f> FindPoint2f(const std::string &) const;
    gtl::ArraySlice<Vector2f> FindVector2f(const std::string &) const;
    gtl::ArraySlice<Point3f> FindPoint3f(const std::string &) const;
    gtl::ArraySlice<Vector3f> FindVector3f(const std::string &) const;
    gtl::ArraySlice<Normal3f> FindNormal3f(const std::string &) const;
    gtl::ArraySlice<Spectrum> FindSpectrum(const std::string &) const;
    gtl::ArraySlice<std::string> FindString(const std::string &) const;
    void ReportUnused() const;
    void Clear();
    std::string ToString() const;
    void Print(int indent) const;

  private:
    // ParamSet Private Data
    std::vector<std::shared_ptr<ParamSetItem<bool>>> bools;
    std::vector<std::shared_ptr<ParamSetItem<int>>> ints;
    std::vector<std::shared_ptr<ParamSetItem<Float>>> floats;
    std::vector<std::shared_ptr<ParamSetItem<Point2f>>> point2fs;
    std::vector<std::shared_ptr<ParamSetItem<Vector2f>>> vector2fs;
    std::vector<std::shared_ptr<ParamSetItem<Point3f>>> point3fs;
    std::vector<std::shared_ptr<ParamSetItem<Vector3f>>> vector3fs;
    std::vector<std::shared_ptr<ParamSetItem<Normal3f>>> normals;
    std::vector<std::shared_ptr<ParamSetItem<Spectrum>>> spectra;
    std::vector<std::shared_ptr<ParamSetItem<std::string>>> strings;
    std::vector<std::shared_ptr<ParamSetItem<std::string>>> textures;
    static std::map<std::string, Spectrum> cachedSpectra;
};

template <typename T>
struct ParamSetItem {
    // ParamSetItem Public Methods
    ParamSetItem(const std::string &name, std::unique_ptr<T[]> val,
                 size_t nValues = 1);

    // ParamSetItem Data
    const std::string name;
    const std::unique_ptr<T[]> values;
    const size_t nValues;
    mutable bool lookedUp = false;
};

// ParamSetItem Methods
template <typename T>
ParamSetItem<T>::ParamSetItem(const std::string &name, std::unique_ptr<T[]> v,
                              size_t nValues)
    : name(name), values(std::move(v)), nValues(nValues) {}

// TextureParams Declarations
class TextureParams {
  public:
    // TextureParams Public Methods
    TextureParams(
        const ParamSet &params,
        std::map<std::string, std::shared_ptr<Texture<Float>>> &fTex,
        std::map<std::string, std::shared_ptr<Texture<Spectrum>>> &sTex)
        : floatTextures(fTex),
          spectrumTextures(sTex),
          params(params) {}
    std::shared_ptr<Texture<Spectrum>> GetSpectrumTexture(
        const std::string &name, const Spectrum &def) const;
    std::shared_ptr<Texture<Spectrum>> GetSpectrumTextureOrNull(
        const std::string &name) const;
    std::shared_ptr<Texture<Float>> GetFloatTexture(const std::string &name,
                                                    Float def) const;
    std::shared_ptr<Texture<Float>> GetFloatTextureOrNull(
        const std::string &name) const;
    Float FindFloat(const std::string &name, Float def) const {
        return params.FindOneFloat(name, def);
    }
    std::string FindString(const std::string &name,
                           const std::string &def = "") const {
        return params.FindOneString(name, def);
    }
    std::string FindFilename(const std::string &name,
                             const std::string &def = "") const {
        return params.FindOneFilename(name, def);
    }
    int FindInt(const std::string &name, int def) const {
        return params.FindOneInt(name, def);
    }
    bool FindBool(const std::string &name, bool def) const {
        return params.FindOneBool(name, def);
    }
    Point3f FindPoint3f(const std::string &name, const Point3f &def) const {
        return params.FindOnePoint3f(name, def);
    }
    Vector3f FindVector3f(const std::string &name, const Vector3f &def) const {
        return params.FindOneVector3f(name, def);
    }
    Normal3f FindNormal3f(const std::string &name, const Normal3f &def) const {
        return params.FindOneNormal3f(name, def);
    }
    Spectrum FindSpectrum(const std::string &name, const Spectrum &def) const {
        return params.FindOneSpectrum(name, def);
    }
    void ReportUnused() const {
        params.ReportUnused();
    }

  private:
    // TextureParams Private Data
    std::map<std::string, std::shared_ptr<Texture<Float>>> &floatTextures;
    std::map<std::string, std::shared_ptr<Texture<Spectrum>>> &spectrumTextures;
    const ParamSet &params;
};

}  // namespace pbrt

#endif  // PBRT_CORE_PARAMSET_H

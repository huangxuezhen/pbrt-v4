// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// It is licensed under the BSD license; see the file LICENSE.txt
// SPDX: BSD-3-Clause

// media.cpp*
#include <pbrt/media.h>

#include <pbrt/interaction.h>
#include <pbrt/paramdict.h>
#include <pbrt/samplers.h>
#include <pbrt/textures.h>
#include <pbrt/util/color.h>
#include <pbrt/util/colorspace.h>
#include <pbrt/util/error.h>
#include <pbrt/util/memory.h>
#include <pbrt/util/print.h>
#include <pbrt/util/sampling.h>
#include <pbrt/util/scattering.h>
#include <pbrt/util/stats.h>

#include <algorithm>
#include <cmath>

namespace pbrt {

std::string MediumInterface::ToString() const {
    return StringPrintf("[ MediumInterface inside: %s outside: %s ]",
                        inside ? inside.ToString().c_str() : "(nullptr)",
                        outside ? outside.ToString().c_str() : "(nullptr)");
}

std::string PhaseFunctionHandle::ToString() const {
    if (ptr() == nullptr)
        return "(nullptr)";

    switch (Tag()) {
    case TypeIndex<HenyeyGreensteinPhaseFunction>():
        return Cast<HenyeyGreensteinPhaseFunction>()->ToString();
    default:
        LOG_FATAL("Unhandled phase function");
        return {};
    }
}

std::string NewMediumSample::ToString() const {
    return StringPrintf("[ NewMediumSample intr: %s t: %f sigma_a: %s sigma_s: %s "
                        "sigma_nt: %f Tn: %f ]",
                        "YOLO INTR" /*intr*/, t, sigma_a, sigma_s, sigma_nt, Tn);
}

// Media Definitions

// HenyeyGreenstein Method Definitions
std::string HenyeyGreensteinPhaseFunction::ToString() const {
    return StringPrintf("[ HenyeyGreensteinPhaseFunction g: %f ]", g);
}

// Media Local Definitions
struct MeasuredSS {
    const char *name;
    RGB sigma_prime_s, sigma_a;  // mm^-1
};

bool GetMediumScatteringProperties(const std::string &name, SpectrumHandle *sigma_a,
                                   SpectrumHandle *sigma_s, Allocator alloc) {
    static MeasuredSS SubsurfaceParameterTable[] = {
        // From "A Practical Model for Subsurface Light Transport"
        // Jensen, Marschner, Levoy, Hanrahan
        // Proc SIGGRAPH 2001
        // clang-format off
        {"Apple", RGB(2.29, 2.39, 1.97), RGB(0.0030, 0.0034, 0.046)},
        {"Chicken1", RGB(0.15, 0.21, 0.38), RGB(0.015, 0.077, 0.19)},
        {"Chicken2", RGB(0.19, 0.25, 0.32), RGB(0.018, 0.088, 0.20)},
        {"Cream", RGB(7.38, 5.47, 3.15), RGB(0.0002, 0.0028, 0.0163)},
        {"Ketchup", RGB(0.18, 0.07, 0.03), RGB(0.061, 0.97, 1.45)},
        {"Marble", RGB(2.19, 2.62, 3.00), RGB(0.0021, 0.0041, 0.0071)},
        {"Potato", RGB(0.68, 0.70, 0.55), RGB(0.0024, 0.0090, 0.12)},
        {"Skimmilk", RGB(0.70, 1.22, 1.90), RGB(0.0014, 0.0025, 0.0142)},
        {"Skin1", RGB(0.74, 0.88, 1.01), RGB(0.032, 0.17, 0.48)},
        {"Skin2", RGB(1.09, 1.59, 1.79), RGB(0.013, 0.070, 0.145)},
        {"Spectralon", RGB(11.6, 20.4, 14.9), RGB(0.00, 0.00, 0.00)},
        {"Wholemilk", RGB(2.55, 3.21, 3.77), RGB(0.0011, 0.0024, 0.014)},

        // From "Acquiring Scattering Properties of Participating Media by
        // Dilution",
        // Narasimhan, Gupta, Donner, Ramamoorthi, Nayar, Jensen
        // Proc SIGGRAPH 2006
        {"Lowfat Milk", RGB(0.89187, 1.5136, 2.532), RGB(0.002875, 0.00575, 0.0115)},
        {"Reduced Milk", RGB(2.4858, 3.1669, 4.5214), RGB(0.0025556, 0.0051111, 0.012778)},
        {"Regular Milk", RGB(4.5513, 5.8294, 7.136), RGB(0.0015333, 0.0046, 0.019933)},
        {"Espresso", RGB(0.72378, 0.84557, 1.0247), RGB(4.7984, 6.5751, 8.8493)},
        {"Mint Mocha Coffee", RGB(0.31602, 0.38538, 0.48131), RGB(3.772, 5.8228, 7.82)},
        {"Lowfat Soy Milk", RGB(0.30576, 0.34233, 0.61664), RGB(0.0014375, 0.0071875, 0.035937)},
        {"Regular Soy Milk", RGB(0.59223, 0.73866, 1.4693), RGB(0.0019167, 0.0095833, 0.065167)},
        {"Lowfat Chocolate Milk", RGB(0.64925, 0.83916, 1.1057), RGB(0.0115, 0.0368, 0.1564)},
        {"Regular Chocolate Milk", RGB(1.4585, 2.1289, 2.9527), RGB(0.010063, 0.043125, 0.14375)},
        {"Coke", RGB(8.9053e-05, 8.372e-05, 0), RGB(0.10014, 0.16503, 0.2468)},
        {"Pepsi", RGB(6.1697e-05, 4.2564e-05, 0), RGB(0.091641, 0.14158, 0.20729)},
        {"Sprite", RGB(6.0306e-06, 6.4139e-06, 6.5504e-06), RGB(0.001886, 0.0018308, 0.0020025)},
        {"Gatorade", RGB(0.0024574, 0.003007, 0.0037325), RGB(0.024794, 0.019289, 0.008878)},
        {"Chardonnay", RGB(1.7982e-05, 1.3758e-05, 1.2023e-05), RGB(0.010782, 0.011855, 0.023997)},
        {"White Zinfandel", RGB(1.7501e-05, 1.9069e-05, 1.288e-05), RGB(0.012072, 0.016184, 0.019843)},
        {"Merlot", RGB(2.1129e-05, 0, 0), RGB(0.11632, 0.25191, 0.29434)},
        {"Budweiser Beer", RGB(2.4356e-05, 2.4079e-05, 1.0564e-05), RGB(0.011492, 0.024911, 0.057786)},
        {"Coors Light Beer", RGB(5.0922e-05, 4.301e-05, 0), RGB(0.006164, 0.013984, 0.034983)},
        {"Clorox", RGB(0.0024035, 0.0031373, 0.003991), RGB(0.0033542, 0.014892, 0.026297)},
        {"Apple Juice", RGB(0.00013612, 0.00015836, 0.000227), RGB(0.012957, 0.023741, 0.052184)},
        {"Cranberry Juice", RGB(0.00010402, 0.00011646, 7.8139e-05), RGB(0.039437, 0.094223, 0.12426)},
        {"Grape Juice", RGB(5.382e-05, 0, 0), RGB(0.10404, 0.23958, 0.29325)},
        {"Ruby Grapefruit Juice", RGB(0.011002, 0.010927, 0.011036), RGB(0.085867, 0.18314, 0.25262)},
        {"White Grapefruit Juice", RGB(0.22826, 0.23998, 0.32748), RGB(0.0138, 0.018831, 0.056781)},
        {"Shampoo", RGB(0.0007176, 0.0008303, 0.0009016), RGB(0.014107, 0.045693, 0.061717)},
        {"Strawberry Shampoo", RGB(0.00015671, 0.00015947, 1.518e-05), RGB(0.01449, 0.05796, 0.075823)},
        {"Head & Shoulders Shampoo", RGB(0.023805, 0.028804, 0.034306), RGB(0.084621, 0.15688, 0.20365)},
        {"Lemon Tea Powder", RGB(0.040224, 0.045264, 0.051081), RGB(2.4288, 4.5757, 7.2127)},
        {"Orange Powder", RGB(0.00015617, 0.00017482, 0.0001762), RGB(0.001449, 0.003441, 0.007863)},
        {"Pink Lemonade Powder", RGB(0.00012103, 0.00013073, 0.00012528), RGB(0.001165, 0.002366, 0.003195)},
        {"Cappuccino Powder", RGB(1.8436, 2.5851, 2.1662), RGB(35.844, 49.547, 61.084)},
        {"Salt Powder", RGB(0.027333, 0.032451, 0.031979), RGB(0.28415, 0.3257, 0.34148)},
        {"Sugar Powder", RGB(0.00022272, 0.00025513, 0.000271), RGB(0.012638, 0.031051, 0.050124)},
        {"Suisse Mocha Powder", RGB(2.7979, 3.5452, 4.3365), RGB(17.502, 27.004, 35.433)},
        {"Pacific Ocean Surface Water", RGB(0.0001764, 0.00032095, 0.00019617), RGB(0.031845, 0.031324, 0.030147)}
        // clang-format on
    };

    for (MeasuredSS &mss : SubsurfaceParameterTable) {
        if (name == mss.name) {
            *sigma_a = alloc.new_object<RGBSpectrum>(*RGBColorSpace::sRGB, mss.sigma_a);
            *sigma_s =
                alloc.new_object<RGBSpectrum>(*RGBColorSpace::sRGB, mss.sigma_prime_s);
            return true;
        }
    }
    return false;
}

std::string MediumHandle::ToString() const {
    if (ptr() == nullptr)
        return "(nullptr)";

    switch (Tag()) {
    case TypeIndex<HomogeneousMedium>():
        return Cast<HomogeneousMedium>()->ToString();
    case TypeIndex<GridDensityMedium>():
        return Cast<GridDensityMedium>()->ToString();
    default:
        LOG_FATAL("Unhandled medium");
        return {};
    };
}

// HomogeneousMedium Method Definitions
pstd::optional<NewMediumSample> HomogeneousMedium::SampleTn(
    const Ray &ray, Float tMax, Float u, const SampledWavelengths &lambda,
    ScratchBuffer *scratchBuffer) const {
    // So t corresponds to distance...
    tMax *= Length(ray.d);
    Ray rayp(ray.o, Normalize(ray.d));

    SampledSpectrum sigma_a = sigma_a_spec.Sample(lambda);
    SampledSpectrum sigma_s = sigma_s_spec.Sample(lambda);
    SampledSpectrum sigma_t = sigma_a + sigma_s;
    Float sigma_nt = sigma_t.MaxComponentValue();

    Float t = SampleExponential(u, sigma_nt);
    if (t >= tMax)
        return {};

    Float Tn = FastExp(-t * sigma_nt);

    MediumInteraction intr(rayp(t), -rayp.d, ray.time, this, &phase);
    return NewMediumSample{intr, t, sigma_a, sigma_s, sigma_nt, Tn};
}

HomogeneousMedium *HomogeneousMedium::Create(const ParameterDictionary &dict,
                                             const FileLoc *loc, Allocator alloc) {
    SpectrumHandle sig_a = nullptr, sig_s = nullptr;
    std::string preset = dict.GetOneString("preset", "");
    if (!preset.empty()) {
        if (!GetMediumScatteringProperties(preset, &sig_a, &sig_s, alloc))
            Warning(loc, "Material preset \"%s\" not found.", preset);
    }
    if (sig_a == nullptr) {
        sig_a = dict.GetOneSpectrum("sigma_a", nullptr, SpectrumType::General, alloc);
        if (sig_a == nullptr)
            sig_a = alloc.new_object<RGBSpectrum>(*RGBColorSpace::sRGB,
                                                  RGB(.0011f, .0024f, .014f));
    }
    if (sig_s == nullptr) {
        sig_s = dict.GetOneSpectrum("sigma_s", nullptr, SpectrumType::General, alloc);
        if (sig_s == nullptr)
            sig_s = alloc.new_object<RGBSpectrum>(*RGBColorSpace::sRGB,
                                                  RGB(2.55f, 3.21f, 3.77f));
    }

    Float scale = dict.GetOneFloat("scale", 1.f);
    if (scale != 1) {
        sig_a = alloc.new_object<ScaledSpectrum>(scale, sig_a);
        sig_s = alloc.new_object<ScaledSpectrum>(scale, sig_s);
    }

    Float g = dict.GetOneFloat("g", 0.0f);

    return alloc.new_object<HomogeneousMedium>(sig_a, sig_s, g, alloc);
}

std::string HomogeneousMedium::ToString() const {
    return StringPrintf("[ Homogeneous medium sigma_a: %s sigma_s: %s g: %f ]",
                        sigma_a_spec, sigma_s_spec, g);
}

STAT_MEMORY_COUNTER("Memory/Volume density grid", densityBytes);
STAT_MEMORY_COUNTER("Memory/Volume density octree", densityOctreeBytes);

// GridDensityMedium Method Definitions
GridDensityMedium::GridDensityMedium(SpectrumHandle sigma_a, SpectrumHandle sigma_s,
                                     Float g, int nx, int ny, int nz,
                                     const Transform &worldFromMedium,
                                     std::vector<Float> d, Allocator alloc)
    : sigma_a_spec(sigma_a, alloc),
      sigma_s_spec(sigma_s, alloc),
      phase(g),
      g(g),
      mediumFromWorld(Inverse(worldFromMedium)),
      worldFromMedium(worldFromMedium),
      densityGrid(d, nx, ny, nz, alloc),
      treeBufferResource(256 * 1024, alloc.resource()) {
    densityBytes += densityGrid.BytesAllocated();

    // Create densityOctree. For starters, make the full thing. (Probably
    // not the best approach).
    const int maxDepth = 6;
    Bounds3f bounds(Point3f(0, 0, 0), Point3f(1, 1, 1));
    Allocator treeAllocator(&treeBufferResource);
    buildOctree(&densityOctree, treeAllocator, bounds, maxDepth);

    // Want world-space bounds, but not including the rotation, so that the bbox
    // doesn't expand
    Vector3f T;
    SquareMatrix<4> R, S;
    worldFromMedium.Decompose(&T, &R, &S);
    Bounds3f worldBounds = Transform(S)(bounds);
    Float SE = worldBounds.SurfaceArea();
    Float sigma_t = sigma_a_spec.MaxValue() + sigma_s_spec.MaxValue();
    simplifyOctree(&densityOctree, worldBounds, SE, sigma_t);

    // FIXME
    // densityOctreeBytes += octreeArena.BytesAllocated();
}

void GridDensityMedium::simplifyOctree(OctreeNode *node, const Bounds3f &bounds, Float SE,
                                       Float sigma_t) {
    if (node->children == nullptr)
        // leaf
        return;

    // Equation (14) from Yue et al: Toward Optimal Space Partitioning for
    // Unbiased, Adaptive Free Path Sampling.
    Float kParent = node->maxDensity * sigma_t;
    Float Nparent = 4 * kParent * bounds.Volume() / SE;

    Float kChildSum = 0;
    for (int i = 0; i < 8; ++i)
        kChildSum += node->child(i)->maxDensity * sigma_t;
    Float childVolume = bounds.Volume() / 8;
    // |P_k| = bounds.SurfaceArea() / 2, but then there is a factor of 2 in
    // equation (14)...
    Float Nsplit = (4 * childVolume * kChildSum + bounds.SurfaceArea()) / SE;

    // LOG(INFO) << bounds << ": Nparent " << Nparent << " (kparent) " <<
    // kParent << ", NSplit " << Nsplit;
    if (1.1 * Nparent < Nsplit) {
        // LOG(INFO) << " -> SIMPLIFYING";
        node->children = nullptr;
    } else {
        for (int i = 0; i < 8; ++i)
            simplifyOctree(node->child(i), OctreeChildBounds(bounds, i), SE, sigma_t);
    }
}

void GridDensityMedium::buildOctree(OctreeNode *node, Allocator alloc,
                                    const Bounds3f &bounds, int depth) {
    if (depth == 0) {
        // leaf
        node->maxDensity = densityGrid.MaximumValue(bounds);
        CHECK_GE(node->maxDensity, 0);
        return;
    }

    node->children = alloc.new_object<pstd::array<OctreeNode *, 8>>();
    for (int i = 0; i < 8; ++i) {
        node->child(i) = alloc.new_object<OctreeNode>();
        buildOctree(node->child(i), alloc, OctreeChildBounds(bounds, i), depth - 1);
    }

    node->minDensity = node->child(0)->minDensity;
    node->maxDensity = node->child(0)->maxDensity;
    for (int i = 1; i < 8; ++i) {
        node->minDensity = std::min(node->minDensity, node->child(i)->minDensity);
        node->maxDensity = std::max(node->maxDensity, node->child(i)->maxDensity);
    }
}

pstd::optional<NewMediumSample> GridDensityMedium::SampleTn(
    const Ray &rWorld, Float raytMax, Float u, const SampledWavelengths &lambda,
    ScratchBuffer *scratchBuffer) const {
    raytMax *= Length(rWorld.d);
    Ray ray = mediumFromWorld(Ray(rWorld.o, Normalize(rWorld.d)), &raytMax);
    // Compute $[\tmin, \tmax]$ interval of _ray_'s overlap with medium bounds
    const Bounds3f b(Point3f(0, 0, 0), Point3f(1, 1, 1));
    Float tMin, tMax;
    if (!b.IntersectP(ray.o, ray.d, raytMax, &tMin, &tMax))
        return {};

    CHECK_LE(tMax, raytMax);

    SampledSpectrum sigma_a = sigma_a_spec.Sample(lambda);
    SampledSpectrum sigma_s = sigma_s_spec.Sample(lambda);
    SampledSpectrum sigma_t = sigma_a + sigma_s;

#if 0
    Float sigma_nt = sigma_t.MaxComponentValue() * densityOctree.maxDensity;
    // Simple, octree-free...
    Float t = tMin + SampleExponential(u, sigma_nt);
    if (t >= tMax)
        return {};

    Point3f p = ray(t);
    Float Tn = Exp(-sigma_nt * (t - tMin));

    Float density = densityGrid.Lookup(p);
    sigma_a *= density;
    sigma_s *= density;

    MediumInteraction intr(worldFromMedium(p), -Normalize(rWorld.d), rWorld.time, this,
                           &phase);
    return NewMediumSample{intr, t, sigma_a, sigma_s, sigma_nt, Tn};

#else  // SIMPLE - no octree
    pstd::optional<NewMediumSample> mediumSample;

    TraverseOctree(
        &densityOctree, ray.o, ray.d, raytMax,
        [&](const OctreeNode &node, Float t0, Float t1) {
            if (node.maxDensity == 0)
                // Empty--skip it!
                return OctreeTraversal::Continue;

            DCHECK_RARE(1e-5, densityGrid.Lookup(ray((t0 + t1) / 2)) > 1.001f * node.maxDensity);

            Float sigma_nt = sigma_t.MaxComponentValue() * node.maxDensity;

            // At what u value do we hit the the cell exit point?
            Float uEnd = InvertExponentialSample(t1 - t0, sigma_nt);
            if (u >= uEnd) {
                // exit this cell w/o a scattering event
                u = (u - uEnd) / (1 - uEnd);
                return OctreeTraversal::Continue;
            }

            Float t = t0 + SampleExponential(u, sigma_nt);
            CHECK_RARE(1e-5, t > t1);

            if (t >= tMax)  // raytMax)
                // Nothing before the geom intersection; get out of here
                return OctreeTraversal::Abort;

            // Scattering event (of some sort)
            Point3f p = ray(t);
            Float Tn = FastExp(-sigma_nt * (t - t0));

            Float density = densityGrid.Lookup(p);

            static bool doRGB = getenv("RGB");
            if (doRGB) {
                RGB rgb(FBm(p, Vector3f(0, 0, 0), Vector3f(0, 0, 0), 1.99f, 5),
                        FBm(.5f * p + Vector3f(5.1, 25.2, -3.2), Vector3f(0, 0, 0),
                            Vector3f(0, 0, 0), 1.99f, 5),
                        FBm(2.5f * p + Vector3f(-6.8, 5.7, -2.09), Vector3f(0, 0, 0),
                            Vector3f(0, 0, 0), 1.99f, 5));
                rgb = RGB(0.25, 0.25, 0.25) + 0.5 * Clamp(rgb, 0, 1);
                RGBSpectrum spec(*RGBColorSpace::sRGB, rgb);
                sigma_a *= spec.Sample(lambda);
                sigma_s *= spec.Sample(lambda);
            }
            sigma_a *= density;
            sigma_s *= density;

            MediumInteraction intr(worldFromMedium(p), -Normalize(rWorld.d), rWorld.time,
                                   this, &phase);
            mediumSample = NewMediumSample{intr, t, sigma_a, sigma_s, sigma_nt, Tn};
            return OctreeTraversal::Abort;
        });

    return mediumSample;
#endif  // !SIMPLE - no octree
}

GridDensityMedium *GridDensityMedium::Create(const ParameterDictionary &dict,
                                             const Transform &worldFromMedium,
                                             const FileLoc *loc, Allocator alloc) {
    SpectrumHandle sig_a = nullptr, sig_s = nullptr;
    std::string preset = dict.GetOneString("preset", "");
    if (!preset.empty()) {
        if (!GetMediumScatteringProperties(preset, &sig_a, &sig_s, alloc))
            Warning(loc, "Material preset \"%s\" not found.", preset);
    }

    if (sig_a == nullptr) {
        sig_a = dict.GetOneSpectrum("sigma_a", nullptr, SpectrumType::General, alloc);
        if (sig_a == nullptr)
            sig_a = alloc.new_object<RGBSpectrum>(*RGBColorSpace::sRGB,
                                                  RGB(.0011f, .0024f, .014f));
    }
    if (sig_s == nullptr) {
        sig_s = dict.GetOneSpectrum("sigma_s", nullptr, SpectrumType::General, alloc);
        if (sig_s == nullptr)
            sig_s = alloc.new_object<RGBSpectrum>(*RGBColorSpace::sRGB,
                                                  RGB(2.55f, 3.21f, 3.77f));
    }

    Float scale = dict.GetOneFloat("scale", 1.f);
    if (scale != 1) {
        sig_a = alloc.new_object<ScaledSpectrum>(scale, sig_a);
        sig_s = alloc.new_object<ScaledSpectrum>(scale, sig_s);
    }

    Float g = dict.GetOneFloat("g", 0.0f);

    std::vector<Float> density = dict.GetFloatArray("density");
    if (density.empty()) {
        Error(loc, "No \"density\" values provided for heterogeneous medium?");
        return nullptr;
    }
    int nx = dict.GetOneInt("nx", 1);
    int ny = dict.GetOneInt("ny", 1);
    int nz = dict.GetOneInt("nz", 1);
    Point3f p0 = dict.GetOnePoint3f("p0", Point3f(0.f, 0.f, 0.f));
    Point3f p1 = dict.GetOnePoint3f("p1", Point3f(1.f, 1.f, 1.f));
    if (density.size() != nx * ny * nz) {
        Error(loc,
              "GridDensityMedium has %d density values; expected nx*ny*nz = "
              "%d",
              (int)density.size(), nx * ny * nz);
        return nullptr;
    }

    Transform MediumFromData =
        Translate(Vector3f(p0)) * Scale(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
    return alloc.new_object<GridDensityMedium>(sig_a, sig_s, g, nx, ny, nz,
                                               worldFromMedium * MediumFromData,
                                               std::move(density), alloc);
}

std::string GridDensityMedium::ToString() const {
    Float maxDensity = densityOctree.maxDensity;
    return StringPrintf("[ GridDensityMedium sigma_a: %s sigma_s: %s mediumFromWorld: %s "
                        "root maxDensity: %f ]",
                        sigma_a_spec, sigma_s_spec, mediumFromWorld, maxDensity);
}

MediumHandle MediumHandle::Create(const std::string &name,
                                  const ParameterDictionary &dict,
                                  const Transform &worldFromMedium, const FileLoc *loc,
                                  Allocator alloc) {
    MediumHandle m = nullptr;
    if (name == "homogeneous")
        m = HomogeneousMedium::Create(dict, loc, alloc);
    else if (name == "heterogeneous")
        m = GridDensityMedium::Create(dict, worldFromMedium, loc, alloc);
    else
        ErrorExit(loc, "%s: medium unknown.", name);

    if (!m)
        ErrorExit(loc, "%s: unable to create medium.", name);

    dict.ReportUnused();
    return m;
}

}  // namespace pbrt

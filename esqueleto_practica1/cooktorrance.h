#ifndef __COOKTORRANCE_H__
#define __COOKTORRANCE_H__

#include <material.h>
#include <gmtl/gmtl.h>

class CookTorrance : public Material {
public:
    CookTorrance(const char* name) : Material(name), Ks_color(0.0f), roughness(1.0f) {}
    CookTorrance(const char* name, float rough, const Color& ks_color) : Material(name), roughness(rough), Ks_color(ks_color) {}

    float roughness;
    Color Ks_color;


    virtual Spectrum BRDF(const Spectrum& Li, const gmtl::Vec3f& L, const gmtl::Vec3f& V, const IntersectInfo& info) const {
        gmtl::Vec3f H = (L + V) / 2.0f;
        gmtl::Vec3f N = info.normal;
        Spectrum FGD = Fresnel(L, H, info) * GeometricFactor(L, V, H, info) * BeckmannDistribution(H, info);
        Spectrum cookTorranceSpectrum = FGD / (4.0f * gmtl::dot(N, L) * gmtl::dot(N, V));
        //cookTorranceSpectrum = Spectrum(min(cookTorranceSpectrum[0], 1.0f), min(cookTorranceSpectrum[1], 1.0f), min(cookTorranceSpectrum[2], 1.0f));
        return cookTorranceSpectrum * Li;
    }

    virtual bool Sample(gmtl::Vec3f& wi, float& pdf, const IntersectInfo& info) const {


        return false;
    }

    virtual float pdf(const gmtl::Vec3f& wi, const gmtl::Vec3f& wo) const {


        return 0.0f;
    }

    Spectrum Fresnel(const gmtl::Vec3f& L, const gmtl::Vec3f& H, const IntersectInfo& info) const {
        const Spectrum WHITE = Spectrum(1.0f, 1.0f, 1.0f);
        Spectrum c_spec = Ks_color.GetColor(info);
        return c_spec + (WHITE - c_spec) * powf(1.0f - max(gmtl::dot(L, H), 0.0f), 5.0f);
    }

    float BeckmannDistribution(const gmtl::Vec3f& H, const IntersectInfo& info) const {
        float NdotH = max(gmtl::dot(info.normal, H), 0.0001f);
        float cos2Alpha = NdotH * NdotH;
        float tan2Alpha = (cos2Alpha - 1.0) / cos2Alpha;
        float roughness2 = roughness * roughness;
        float denom = M_PI * roughness2 * cos2Alpha * cos2Alpha;
        return exp(tan2Alpha / roughness2) / denom;
    }
    float GeometricFactor(const gmtl::Vec3f& L, const gmtl::Vec3f& V, const gmtl::Vec3f& H, const IntersectInfo& info) const {
        gmtl::Vec3f N = info.normal;
        float NdotH = gmtl::dot(N, H);
        float NdotV = gmtl::dot(N, V);
        float NdotL = gmtl::dot(N, L);
        float VdotH = gmtl::dot(V, H);
        float c = 2 * NdotH / VdotH;
        return min(1.0f, min(c * NdotV, c * NdotL));
    }
};


#endif
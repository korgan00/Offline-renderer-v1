#ifndef __LAMBERTCT_H__
#define __LAMBERTCT_H__

#include <material.h>
#include <gmtl/gmtl.h>
#include <lambert.h>
#include <cooktorrance.h>

class LambertCT : public Material {
public:
    LambertCT(const char* name) : Material(name), lambert(name, 0.0f, 1.0f), cookTorrance(name, 1.0f, 0.0f) {}
    LambertCT(const char* name, float kd, const Color& kd_color, float rough, const Color& ks_color) :
        Material(name), lambert(name, kd, kd_color), cookTorrance(name, rough, ks_color) {}

    Lambert lambert;
    CookTorrance cookTorrance;

    virtual Spectrum BRDF(const Spectrum& Li, const gmtl::Vec3f& L, const gmtl::Vec3f& V, const IntersectInfo& info) const {
        gmtl::Vec3f N = info.normal;
        gmtl::Vec3f H = (L + V) / 2.0f;
        float F = powf(1.0f - max(gmtl::dot(L, H), 0.0f), 5.0f);
        return (1.0f - F) * lambert.BRDF(Li, L, V, info) + cookTorrance.BRDF(Li, L, V, info);
    }
    virtual bool Sample(gmtl::Vec3f& wi, float& pdf, const IntersectInfo& info) const {


        return false;
    }

    virtual float pdf(const gmtl::Vec3f& wi, const gmtl::Vec3f& wo) const {


        return 0.0f;
    }
};


#endif
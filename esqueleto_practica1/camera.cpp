#include <camera.h>
#include "random_helpers.h"

gmtl::Rayf Camera::generateRay(float pixelX, float pixelY) {
    gmtl::Point3f wCamPos = mCameraToWorld(gmtl::Point3f(0, 0, 0));

    if (_useDof) {
        gmtl::Point2f pixelOffset = halton2D(randomInt(10000), HALTON_SEED_X, HALTON_SEED_Y);
        pixelOffset = (pixelOffset - gmtl::Point2f(0.5f, 0.5f)) * 100000.0f * _dofPower;
        gmtl::Point3f wPoint = mCameraToWorld(mRasterToCamera(gmtl::Point3f(pixelX + pixelOffset[0], pixelY + pixelOffset[1], 0)));

        gmtl::Point3f wPixelCenter = mCameraToWorld(mRasterToCamera(gmtl::Point3f(pixelX + 0.5f, pixelY + 0.5f, 0)));
        gmtl::Vec3f centerVec = (gmtl::Vec3f)(wPixelCenter - wCamPos);
        gmtl::normalize(centerVec);
        gmtl::Point3f focusPoint = (centerVec * _focusDistance) + wCamPos;

        gmtl::Vec3f vec = (gmtl::Vec3f)(focusPoint - wPoint);
        gmtl::normalize(vec);
        gmtl::Point3f newCamPos = focusPoint - (vec * _focusDistance);
        gmtl::Vec3f camPosDist = newCamPos - wCamPos;
        
        return gmtl::Rayf(focusPoint - (vec * _focusDistance), vec);
    } else {
        gmtl::Point2f pixelOffset = halton2D(randomInt(10000), HALTON_SEED_X, HALTON_SEED_Y);
        gmtl::Point3f wPoint = mCameraToWorld(mRasterToCamera(gmtl::Point3f(pixelX + pixelOffset[0], pixelY + pixelOffset[1], 0)));

        gmtl::Vec3f vec = (gmtl::Vec3f)(wPoint - wCamPos);
        gmtl::normalize(vec);

        return gmtl::Rayf(wCamPos, vec);
    }
}


gmtl::Point2ui Camera::getResolution() const
{
    return mResolution;
}

void Camera::setOutputPath(const char* filename)
{
    mOutputPath = filename;
}

const char* Camera::getOutputPath() const
{
    return mOutputPath.c_str();
}
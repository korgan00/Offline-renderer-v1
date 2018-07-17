#include <camera.h>
#include "random_helpers.h"

gmtl::Rayf Camera::generateRay(float pixelX, float pixelY) {
	gmtl::Point3f wPoint = mCameraToWorld(mRasterToCamera(gmtl::Point3f(pixelX, pixelY, 0)));
	gmtl::Point3f wCamPos = mCameraToWorld(gmtl::Point3f(0, 0, 0));
	gmtl::Vec3f vec = (gmtl::Vec3f)(wPoint - wCamPos);
	gmtl::normalize(vec);

    return gmtl::Rayf(wCamPos, vec);
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
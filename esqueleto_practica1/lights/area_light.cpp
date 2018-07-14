#include <lights/arealight.h>
#include "random_helpers.h"


Spectrum AreaLight::Sample(const gmtl::Point3f& position, gmtl::Vec3f& wi, float& pdf, gmtl::Rayf& visibilityRay) const
{
    using namespace gmtl;
#define LIGHT_WIDTH 2.0f
#define LIGHT_HEIGHT 2.0f
#define LIGHT_HALF_W (LIGHT_WIDTH / 2.0f)
#define LIGHT_HALF_H (LIGHT_HEIGHT / 2.0f)

    Point2f offset = halton2D(randomInt(100), HALTON_SEED_X, HALTON_SEED_Y);
    Point3f haltonPosition(mPosition + Point3f((offset[0] * LIGHT_WIDTH) - LIGHT_HALF_W, (offset[1] * LIGHT_HEIGHT) - LIGHT_HALF_H, 0.0f));

    Point3f haltonTransformedPosition = mWorldTransform(haltonPosition);

    wi = haltonTransformedPosition - position;

	float len = normalize(wi);
	visibilityRay.setOrigin(position);
	visibilityRay.setDir(wi);
	visibilityRay.setMaxLength(len);
	pdf = 1.0f / static_cast<float>(LIGHT_WIDTH*LIGHT_HEIGHT);
	float i = mIntensity;
	i /= (len * len);
    i /= static_cast<float>(LIGHT_WIDTH*LIGHT_HEIGHT);

	return mColor * i;
}
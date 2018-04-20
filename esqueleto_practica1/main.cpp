//#include <tbb/tbb.h>
#include <stdio.h>
#include <math.h>
#include <gmtl/gmtl.h>
#include <imageio.h>

#include <world.h>
#include <standard.h>
#include <lights/pointlight.h>

#include <parsers/ass_parser.h>

#include <reporter.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <algorithm>

#include <main.h>



int g_RenderMaxDepth = 12;

extern int g_pixel_samples;

World* ReadFromFile(const char* filename)
{
    World* world;
    if (!ReadAssFile(filename, world))
    {
        Error("Could not read file: %s", filename);
        return NULL;
    }
    return world;
}
gmtl::Vec3f refractedDirection(float ni, float nt, const gmtl::Vec3f& V, const gmtl::Vec3f& N)
{
	gmtl::Vec3f T;
	float eta;

	eta = ni / nt;
	float c1 = -dot(V, N);
	float c2_op = 1.0f - eta * eta*(1.0f - c1 * c1);
	if (c2_op < 0.0f)
		return gmtl::Vec3f(0.0f);

	float c2 = sqrt(c2_op);
	T = eta * V + (eta*c1 - c2)*N;

	return T;
}

gmtl::Vec3f transmittedDirection(bool& entering, const gmtl::Vec3f& N, const gmtl::Vec3f& V, const Standard* mat)
{
	gmtl::Vec3f normal_refraction = N;
	bool exitingObject = dot(N, -V) < 0.0f;
	float ni = 0.0f;
	float nt = 0.0f;

	if (exitingObject)
	{
		ni = mat->refractionIndex;
		nt = 1.0003f; // air refraction index
		normal_refraction = -normal_refraction;
	}
	else
	{
		ni = 1.0003f; // air refraction index
		nt = mat->refractionIndex;
	}

	gmtl::Vec3f T = refractedDirection(ni, nt, V, normal_refraction);

	entering = !exitingObject;
	return T;
}


Spectrum traceRay(World* world, gmtl::Rayf currRay, int callDepth = 0) {
	Spectrum finalColor;
	if (callDepth >= 5) return finalColor;
	callDepth++;

	IntersectInfo info;
	world->intersect(info, currRay);
	if (info.objectID != InvalidObjectID) {
		Point3f collisionPoint = info.position;
		Vector3f v = -currRay.getDir();
		gmtl::normalize(v);
		Vector3f n = info.normal;
		gmtl::normalize(n);

		Standard* mat = ((Standard*)info.material);
		float diffSum = 0.0f, specSum = 0.0f;

		std::vector<Light*>::iterator lightIt = world->mLights.begin();

		while (lightIt != world->mLights.end()) {
			PointLight* currLight = (PointLight*)*lightIt;
			Vector3f l = currLight->getWorldPosition() - collisionPoint;
			float sqrDist = gmtl::lengthSquared(l);
			gmtl::normalize(l);

			IntersectInfo shadowInfo;
			world->intersect(shadowInfo, gmtl::Rayf(collisionPoint + l * 0.01f, l, sqrt(sqrDist)));

			if (shadowInfo.objectID == InvalidObjectID) {
				Vector3f r;
				float intensity = currLight->mIntensity / sqrDist;
				r = -gmtl::reflect(r, l, n);
				gmtl::normalize(r);

				diffSum += intensity * max(gmtl::dot(n, l), 0.0f);
				specSum += intensity * pow(max(gmtl::dot(r, v), 0.0f), mat->Kshi);
			}

			lightIt++;
		}


		Vector3f v_r;
		v_r = -gmtl::reflect(v_r, v, n);
		Spectrum reflection = traceRay(world, gmtl::Rayf(collisionPoint + v_r * 0.01f, v_r), callDepth);


		bool entering;
		Vector3f T = transmittedDirection(entering, info.normal, info.ray.getDir(), mat);
		Spectrum refraction = traceRay(world, gmtl::Rayf(collisionPoint + T * 0.01f, T), callDepth);

		finalColor = mat->Ka_color.GetColor(info) * mat->Ka * 0.01f +
					 mat->Kd.GetColor(info) * diffSum +
					 mat->Ks.GetColor(info) * specSum +
					 mat->Kr.GetColor(info) * reflection +
					 mat->Kt.GetColor(info) * refraction;

	}

	callDepth--;
	return finalColor;
}

void render_image(World* world, unsigned int dimX, unsigned int dimY, float* image, float* alpha)
{
	#define im(x,y,s,a) image[(y * dimX + x) * 3] = s[0];\
						  image[(y * dimX + x) * 3 + 1] = s[1];\
						  image[(y * dimX + x) * 3 + 2] = s[2];\
						  alpha[y * dimX + x] = a;

	Camera* c = world->getCamera();

	unsigned int accum = 0;
	#pragma omp parallel for schedule(dynamic)
	for (int j = 0; j < (int)dimY; j++) {
		gmtl::Rayf currRay;
		for (unsigned int i = 0; i < dimX; i++) {
			currRay = c->generateRay(static_cast<float>(i), static_cast<float>(j));
			Spectrum finalColor = traceRay(world, currRay);
			#pragma omp critical 
			{ im(i, j, finalColor, 1.0f); }
		}
		#pragma omp critical
		{ accum++; }
		printf("\r %f", (float)accum / dimY);
	}
}

unsigned int g_intersectTriangleCalls;
extern "C"
{
    __declspec(dllexport) void renderSceneFromFile(float*& image, float*& alpha, World*& world, const char* filename)
    {
        google::InitGoogleLogging("rendering.dll");
        FLAGS_logtostderr = 1;

        g_intersectTriangleCalls = 0;

        // Create world from file
        world = ReadFromFile(filename);
        if (!world)
        {
            fprintf(stderr, "Error reading file %s. Press enter to exit", filename);
            getc(stdin);
            return;
        }
        INITREPORTER("report.ma", world);
        unsigned int dimX = world->getCamera()->getResolution()[0];
        unsigned int dimY = world->getCamera()->getResolution()[1];

        image = new float[dimX*dimY * 3];
        alpha = new float[dimX*dimY];


        // Compute pixel values
        clock_t tStart = clock();
        render_image(world, dimX, dimY, image, alpha);
        clock_t tEnd = clock();
        LOG(INFO) << "Time taken: " << (double)(tEnd - tStart) / CLOCKS_PER_SEC << "s";
        LOG(INFO) << "Triangles intersected: " << g_intersectTriangleCalls;

        google::ShutdownGoogleLogging();
    }

    __declspec(dllexport) void WriteImg(const char* name, float *pixels, float *alpha, int xRes,
        int yRes, int totalXRes, int totalYRes, int xOffset, int yOffset)
    {
        WriteImage(name, pixels, alpha, xRes, yRes, totalXRes, totalYRes, xOffset, yOffset);
    }
}

// dllmain.cpp : Defines the entry point for the DLL application.

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

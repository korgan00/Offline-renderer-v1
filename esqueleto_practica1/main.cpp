//#include <tbb/tbb.h>
#include <stdio.h>
#include <math.h>
#include <gmtl/gmtl.h>
#include <imageio.h>

#include <world.h>
#include <standard.h>

#include <parsers/ass_parser.h>

#include <reporter.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <algorithm>

#include <main.h>

#define RUSSIAN_ROULLETTE_PROC 1.0f
#include "random_helpers.h"

int g_RenderMaxDepth = 12;

extern int g_pixel_samples;

#define SAMPLES 100
#define INDIRECT_LIGHT_ON true
#define TRUNCATE_FINAL_SPECTRUM_PER_SAMPLE true
#define TRUNCATE_MAX_VALUE (SAMPLES/2.0f)

Spectrum traceRay(World* world, gmtl::Rayf currRay);

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

Spectrum directLight(World* world, const Point3f& collisionPoint, const Vector3f& v, const IntersectInfo& info) {
    int lightCount = world->mLights.size();

    if (lightCount == 0) return Spectrum();

    Light* currLight = *(world->mLights.begin() + randomInt(lightCount));

    Spectrum directContribution;
    gmtl::Vec3f wi;
    float pdf;
    gmtl::Rayf visibilityRay;
    IntersectInfo shadowInfo;
    Vector3f n = info.normal;
    gmtl::normalize(n);

    Spectrum lightSpectrum = currLight->Sample(collisionPoint, wi, pdf, visibilityRay);
    visibilityRay.setOrigin(visibilityRay.getOrigin() + visibilityRay.getDir() * 0.0001f);

    if (!world->shadow(visibilityRay)) {
        directContribution = info.material->BRDF(lightSpectrum, wi, v, info);
        directContribution *= max(gmtl::dot(n, wi), 0.0f);
        directContribution /= pdf;
    }

    return directContribution * static_cast<float>(lightCount);
}

Spectrum indirectLight(World* world, const Point3f& collisionPoint, const Vector3f& v, const IntersectInfo& info) {
    Spectrum indirectContribution;

#if INDIRECT_LIGHT_ON
    if (!russianRoulletteCheck()) return indirectContribution;

    gmtl::Vec3f wi = randomVec3f();
    gmtl::normalize(wi);
    Vector3f n = info.normal;
    gmtl::normalize(n);

    while (gmtl::dot(n, wi) < 0.0001f) {
        wi = -wi;
        if (gmtl::dot(n, wi) < 0.0001f) {
            wi = randomVec3f();
            gmtl::normalize(wi);
        }
    }

    gmtl::Rayf ray = gmtl::Rayf(info.position + wi * 0.001f, wi);
    indirectContribution = traceRay(world, ray);

    indirectContribution = info.material->BRDF(indirectContribution, wi, v, info);
    indirectContribution *= max(gmtl::dot(n, wi), 0.0f);
    indirectContribution *= 2.0f * M_PI; // pdf
    indirectContribution /= RUSSIAN_ROULLETTE_PROC;
#endif

    return indirectContribution;
}

Spectrum traceRay(World* world, gmtl::Rayf currRay) {
	Spectrum finalColor;

	IntersectInfo info;
	world->intersect(info, currRay);
	if (info.objectID != InvalidObjectID) {
		Point3f collisionPoint = info.position;
		Vector3f v = currRay.getDir();
		gmtl::normalize(v);
        Vector3f wi;

        finalColor = directLight(world, collisionPoint, v, info) + indirectLight(world, collisionPoint, v, info);
	}

	return finalColor;
}

void render_image(World* world, unsigned int dimX, unsigned int dimY, float* image, float* alpha)
{
	#define im(x,y,s,a) image[(y * dimX + x) * 3] = s[0];\
						  image[(y * dimX + x) * 3 + 1] = s[1];\
						  image[(y * dimX + x) * 3 + 2] = s[2];\
						  alpha[y * dimX + x] = a;

	Camera* c = world->getCamera();

	static int accum = 0;
	#pragma omp parallel for schedule(dynamic) 
	for (int j = 0; j < (int)dimY; j++) {
		gmtl::Rayf currRay;
        #pragma omp critical
        { 
            accum++;
            srand(accum);
        }
		for (unsigned int i = 0; i < dimX; i++) {
            Spectrum finalColor;
            for (unsigned int k = 0; k < SAMPLES; k++) {
                gmtl::Point2f pixelOffset = halton2D(k, HALTON_SEED_X, HALTON_SEED_Y);
                currRay = c->generateRay(static_cast<float>(i) + pixelOffset[0], static_cast<float>(j) + pixelOffset[1]);
                Spectrum raytraced = traceRay(world, currRay);
#if TRUNCATE_FINAL_SPECTRUM_PER_SAMPLE
                float l = gmtl::length(raytraced);
                if (l > static_cast<float>(TRUNCATE_MAX_VALUE)) {
                    raytraced = (raytraced / l) * static_cast<float>(TRUNCATE_MAX_VALUE);
                }
#endif
                finalColor += raytraced;
            }
            finalColor = finalColor / static_cast<float>(SAMPLES);
			#pragma omp critical 
			{ im(i, j, finalColor, 1.0f); }
		}
	    #pragma omp critical
		{
		    printf("\r %f", (float)accum / dimY);
        }
	}
    printf("\n");
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

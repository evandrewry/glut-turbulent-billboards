#include <vmath.hpp>
#include <cmath>
#include <vector>
#include <noise.h>
#include "Common.hpp"

using namespace vmath;

extern bool ShowStreamlines;
extern bool obstacle;

static Point3 SphereCenter(0, 0, 0);
static const float SphereRadius = 0.75f;
static const float Epsilon = 1e-10f;
static const float NoiseLengthScale[] = {0.4f, 0.23f, 0.11f};
static const float NoiseGain[] = {1.0f, 0.5f, 0.25f};
static const float PlumeCeiling(1);
static const float PlumeBase(-.75);
static const float PlumeHeight(4);
static const float RingRadius(1.25f);
static const float RingSpeed(0.3f);
static const float RingsPerSecond(0.125f);
static const float RingMagnitude(10);
static const float RingFalloff(0.7f);
static const float ParticlesPerSecond(ShowStreamlines ? 1000: 10000 );
static const float SeedRadius(0.125f);
static const float InitialBand(.25f);


static float Time = 0;
static unsigned int Seed(0);

static FlowNoise3 noise;

inline float noise0(Vector3 s) { return noise(s.getX(), s.getY(), s.getZ()); }
inline float noise1(Vector3 s) { return noise(s.getY() + 31.416f, s.getZ() - 47.853f, s.getX() + 12.793f); }
inline float noise2(Vector3 s) { return noise(s.getZ() - 233.145f, s.getX() - 113.408f, s.getY() - 185.31f); }
inline Vector3 noise3d(Vector3 s) { return Vector3(noise0(s), noise1(s), noise2(s)); };

static Vector3 BlendVectors(Vector3 potential, float alpha, Vector3 distanceGradient)
{
    float dp = dot(potential, distanceGradient);
    return alpha * potential + (1-alpha) * dp * distanceGradient;
}


static float SampleDistance(Point3 p)
{
    if (!obstacle) return (0* p.getX() + 1 * p.getY() + 0 * p.getZ() - 0.75);

    float phi = p.getY();
    Vector3 u = p - SphereCenter;
    float d = length(u);
    return d - SphereRadius;
}

static Vector3 ComputeGradient(Point3 p)
{
    const float e = 0.01f;
    Vector3 dx(e, 0, 0);
    Vector3 dy(0, e, 0);
    Vector3 dz(0, 0, e);

    float d =    SampleDistance(p);
    float dfdx = SampleDistance(p + dx) - d;
    float dfdy = SampleDistance(p + dy) - d;
    float dfdz = SampleDistance(p + dz) - d;

    return normalize(Vector3(dfdx, dfdy, dfdz));
}

static Vector3 SamplePotential(Point3 p)
{
   Vector3 psi(0,0,0);
   Vector3 gradient = ComputeGradient(p);

   float obstacleDistance = SampleDistance(p);

   // add turbulence octaves that respect boundaries, increasing upwards
   float height_factor = ramp((p.getY() - PlumeBase) / PlumeHeight);
   for (unsigned int i=0; i < 3; ++i) {
        Vector3 s = Vector3(p) / NoiseLengthScale[i];
        float d = ramp(std::fabs(obstacleDistance) / NoiseLengthScale[i]);
        Vector3 psi_i = BlendVectors(noise3d(s), d, gradient);
        psi += height_factor*NoiseGain[i]*psi_i;
   }

   Vector3 risingForce = Point3(0, 0, 0) - p;
   risingForce = Vector3(-risingForce[2], 0, risingForce[0]);

   // add rising vortex rings
   float ring_y = PlumeCeiling;
   float d = ramp(std::fabs(obstacleDistance) / RingRadius);
   while (ring_y > PlumeBase) {
      float ry = p.getY() - ring_y;
      float rr = std::sqrt(p.getX()*p.getX()+p.getZ()*p.getZ());
      float rmag = RingMagnitude / (sqr(rr-RingRadius)+sqr(rr+RingRadius)+sqr(ry)+RingFalloff);
      Vector3 rpsi = rmag * risingForce;
      psi += BlendVectors(rpsi, d, gradient);
      ring_y -= RingSpeed / RingsPerSecond;
   }

   return psi;
}




static Vector3 ComputeCurl(Point3 p)
{
    const float e = 1e-4f;
    Vector3 dx(e, 0, 0);
    Vector3 dy(0, e, 0);
    Vector3 dz(0, 0, e);

    float x = SamplePotential(p + dy)[2] - SamplePotential(p - dy)[2]
            - SamplePotential(p + dz)[1] + SamplePotential(p - dz)[1];

    float y = SamplePotential(p + dz)[0] - SamplePotential(p - dz)[0]
            - SamplePotential(p + dx)[2] + SamplePotential(p - dx)[2];

    float z = SamplePotential(p + dx)[1] - SamplePotential(p - dx)[1]
            - SamplePotential(p + dy)[0] + SamplePotential(p - dy)[0];

    return Vector3(x, y, z) / (2*e);
}



static void SeedParticles(ParticleList& list, float dt)
{
    static float time = 0;
    time += dt;
    unsigned int num_new = (unsigned int) (time * ParticlesPerSecond);
    if (num_new == 0)
        return;

    time = 0;
    list.reserve(list.size() + num_new);
    for (unsigned int i = 0; i < num_new; ++i) {
        float theta = randhashf(Seed++, 0, M_PI * 2);
        float r = randhashf(Seed++, 0, SeedRadius);
        float y = randhashf(Seed++, 0, InitialBand);
        Particle p;
        p.Px = r*std::cos(theta);
        p.Py = PlumeBase + y;
        p.Pz = r*std::sin(theta) + 0.125f; // Nudge the emitter towards the viewer
        p.ToB = Time;
        list.push_back(p);
    }
}

void AdvanceTime(ParticleList& list, float dt, float timeStep)
{
    Time += dt;

    for (size_t i = 0; i < list.size(); ++i) {
        Point3 p(list[i].Px, list[i].Py, list[i].Pz);
        Vector3 v = ComputeCurl(p);
        Point3 midx = p + 0.5f * timeStep * v;
        p += timeStep * ComputeCurl(midx);
        list[i].Px = p.getX();
        list[i].Py = p.getY();
        list[i].Pz = p.getZ();
        list[i].Vx = v.getX();
        list[i].Vy = v.getY();
        list[i].Vz = v.getZ();
    }

    for (ParticleList::iterator i = list.begin(); i != list.end();) {
        if (i->Py > PlumeCeiling) {
            i = list.erase(i);
        } else {
            ++i;
        }
    }

   noise.set_time(0.5f*NoiseGain[0]/NoiseLengthScale[0]*Time);

   if (!ShowStreamlines || Time < 0.1f)
        SeedParticles(list, dt);
}


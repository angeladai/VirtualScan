#pragma once
#include "GlobalAppState.h"

class TrajectoryGenerator
{
public:
	TrajectoryGenerator(bool partial) {
		mat4f intrinsics(
			570.342163f, 0.0f, 320.0f, 0.0f,
			0.0f, 570.342224f, 240.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			);
		const unsigned int width = 640;
		const unsigned int height = 480;
		const float fx = intrinsics(0, 0);
		m_near = 0.1f;
		m_far = 5.0f;
		m_fov = math::radiansToDegrees((float)2.0 * atan((float)width / ((float)2 * fx)));
		m_aspect = (float)width / (float)height;

		m_flip = mat4f::rotationX(180);

		m_horizAngleRange = vec2f(1.0f, 90.0f);
		m_tiltAngleRange = vec2f(-5.0f, 5.0f);

		m_radiusRange = vec2f(0.85f, 1.3f);
		m_vertAngleRange = vec2f(45.0f, 45.0f); //normal dist about 30 degrees

		m_bGeneratePartial = partial;
	}
	~TrajectoryGenerator() {}

	// debug functions
	void setHorizAngleRange(const vec2f& range) { m_horizAngleRange = range; }
	void setVertAngleRange(const vec2f& range) { m_vertAngleRange = range; }
	void setTiltAngleRange(const vec2f& range) { m_tiltAngleRange = range; }
	void setScanRadiusRange(const vec2f& range) { m_radiusRange = range; } // mult factor against bounding sphere radius

	void generateNoRand(const bbox3f& sceneBbox, std::vector<Cameraf>& trajectory, unsigned int numTransforms) const;

	void generate(const bbox3f& sceneBbox, std::vector<Cameraf>& trajectory, unsigned int numTransforms) const {
		generate(sceneBbox, trajectory, numTransforms, m_horizAngleRange);
	}
	void generate(const bbox3f& sceneBbox, std::vector<Cameraf>& trajectory, unsigned int numTransforms, const vec2f& horizAngleRange) const {
		if (m_bGeneratePartial) {
			float startAngle = 0.0f;
			{
				unsigned int g = ml::RNG::global.uniform(0, 4);
				switch (g) {
				case 0: startAngle = 0.0f;
					break;
				case 1: startAngle = 90.0f;
					break;
				case 2: startAngle = 180.0f;
					break;
				case 3: startAngle = 270.0f;
					break;
				default:
					throw MLIB_EXCEPTION("ERROR");
				}
			}
			startAngle += (float)ml::RNG::global.normal(0.0f, 30.0f);
			const float scanningDir = (ml::RNG::global.uniform(0, 2) == 0) ? 1.0f : -1.0f; //cw or ccw

			const float totalRotation = ml::RNG::global.uniform(horizAngleRange.x, horizAngleRange.y);
			const float rotationIncrement = std::max(15.0f, totalRotation / numTransforms);
			numTransforms = (unsigned int)math::ceil(totalRotation / rotationIncrement);
			const vec3f& center = sceneBbox.getCenter();

			if (numTransforms == 0) {
				std::cerr << "WARNING attempting to generate " << numTransforms << " transforms";
				numTransforms = 1;
			}

			trajectory.resize(numTransforms);
			for (unsigned int i = 0; i < numTransforms; i++) {
				float dist = ml::RNG::global.uniform(m_radiusRange.x, m_radiusRange.y) * sceneBbox.getMaxExtent();

				float horizRot = startAngle + scanningDir * (rotationIncrement * i);
				float vertRot = (float)ml::RNG::global.normal(m_vertAngleRange.x, m_vertAngleRange.y);
				float tiltRot = ml::RNG::global.uniform(m_tiltAngleRange.x, m_tiltAngleRange.y);
				//std::cout << "\t(vert, tilt) = " << vertRot << ", " << tiltRot << std::endl;
				generateCamera(trajectory[i], center, dist, horizRot, vertRot, tiltRot);
			}
		}
		else {
			trajectory.resize(numTransforms);
			for (unsigned int i = 0; i < numTransforms; i++) {
				vec3f dir = vec3f((float)RNG::global.rand_closed01() - 0.5f, (float)RNG::global.rand_closed01() - 0.5f, (float)RNG::global.rand_closed01() - 0.5f).getNormalized();
				vec3f worldUp = (dir ^ vec3f(dir.z, -dir.x, dir.y)).getNormalized();

				float dist = 1.5f*sceneBbox.getMaxExtent();
				vec3f eye = sceneBbox.getCenter() - dist*dir;

				vec3f lookDir = (sceneBbox.getCenter() - eye).getNormalized();
				trajectory[i] = Cameraf(eye, lookDir, worldUp, m_fov, m_aspect, m_near, m_far);
			}
		}
	}

	void generate360(const bbox3f& sceneBbox, std::vector<Cameraf>& trajectory, unsigned int numTransforms) const {
		trajectory.reserve(numTransforms);
		// sample sphere
		const vec3f bboxCenter = sceneBbox.getCenter();
		//const vec3f bboxExtent = sceneBbox.getExtent();
		const float sphereRadius = 1.6f * sceneBbox.getMaxExtent(); 
		std::mt19937 generator(0);
		std::uniform_real_distribution<double> uniform01(0.0, 1.0);
		for (unsigned int i = 0; i < numTransforms; i++) {
			float phi = (float)(2.0 * math::PI * uniform01(generator));
			float costheta = (float)(2.0 * uniform01(generator) - 1.0);
			float theta = std::acos(math::clamp(costheta, -1.0f, 1.0f));

			vec3f eye = bboxCenter + 
				vec3f(std::sin(theta) * std::cos(phi),
				      std::sin(theta) * std::sin(phi),
					  std::cos(theta)).getNormalized() * sphereRadius;

			const vec3f lookDir = (bboxCenter - eye).getNormalized();
			vec3f up = (lookDir ^ vec3f(lookDir.z, -lookDir.x, lookDir.y)).getNormalized();
			if (!isnan(up[0]) && std::fabs(up[0]) != std::numeric_limits<float>::infinity())
				trajectory.push_back(Cameraf(eye, lookDir, up, m_fov, m_aspect, m_near, m_far));
		}
	}

private:
	void generateCamera(Cameraf& camera, const vec3f& center, float dist, float horizRot, float vertRot, float tiltRot) const;

	// parameters
	vec2f m_horizAngleRange;
	vec2f m_vertAngleRange;
	vec2f m_tiltAngleRange;
	vec2f m_radiusRange;

	float m_fov;
	float m_aspect;
	float m_near;
	float m_far;

	mat4f m_flip;

	bool m_bGeneratePartial;
};

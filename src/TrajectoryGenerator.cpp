#include "stdafx.h"
#include "TrajectoryGenerator.h"



void TrajectoryGenerator::generateCamera(Cameraf& camera, const vec3f& center, float dist, float horizRot, float vertRot, float tiltRot) const
{
	vec3f dir = mat3f::rotationY(horizRot) * mat3f::rotationX(vertRot) * vec3f::eZ;
	vec3f eye = center - dist * dir;
	vec3f right = mat3f::rotationY(horizRot) * mat3f::rotationX(vertRot) * mat3f::rotationZ(tiltRot) * vec3f::eX;
	vec3f worldUp = dir ^ right;
	vec3f lookDir = (center - eye).getNormalized();
	MLIB_ASSERT(fabs(worldUp.length() - 1.0f) < 0.0001f);

	camera = Cameraf(eye, lookDir, worldUp, m_fov, m_aspect, m_near, m_far);
}

void TrajectoryGenerator::generateNoRand(const bbox3f& sceneBbox, std::vector<Cameraf>& trajectory, unsigned int numTransforms) const
{
	if (m_bGeneratePartial) {
		const float startAngle = 0.0f;
		const float totalRotation = 180.0f;
		const float rotationIncrement = totalRotation / numTransforms;
		const vec3f& center = sceneBbox.getCenter();

		trajectory.resize(numTransforms);
		for (unsigned int i = 0; i < numTransforms; i++) {
			float dist = 1.5f;

			float horizRot = rotationIncrement * i + startAngle;
			float vertRot = 0.0f;
			float tiltRot = 0.0f;

			generateCamera(trajectory[i], center, dist, horizRot, vertRot, tiltRot);
		}
	}
	else {
		std::cout << "ERROR : generateNoRand not implemented for non-partial" << std::endl;
		getchar();
	}
}

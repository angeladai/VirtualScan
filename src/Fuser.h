#pragma once

#include "mLibInclude.h"
#include "VoxelGrid.h"

template<>
struct std::hash<vec3ui> : public std::unary_function < vec3ui, size_t > {
	size_t operator()(const vec3ui& v) const {
		//TODO larger prime number (64 bit) to match size_t
		const size_t p0 = 73856093;
		const size_t p1 = 19349669;
		const size_t p2 = 83492791;
		const size_t res = ((size_t)v.x * p0) ^ ((size_t)v.y * p1) ^ ((size_t)v.z * p2);
		return res;
	}
};

class Fuser
{
public:
	Fuser(ml::ApplicationData& _app);

	~Fuser();

	bool fuse(const std::string& outputFile, Scene& scene, const std::vector<Cameraf>& cameraTrajectory, bool debugOut = false);

	unsigned int verify(const std::string& filename) {
		VoxelGrid grid(vec3l(30, 30, 30), mat4f::identity(), 0.0f, 0.0f, 0.0f);
		grid.loadFromFile(filename);

		BinaryGrid3 binaryGrid = grid.toBinaryGridOccupied(1, 1.0f);
		size_t numOccupiedEntries = grid.toBinaryGridOccupied(1, 1.0f).getNumOccupiedEntries();
		return (unsigned int)numOccupiedEntries;
	}

	
private:

	const int INF = std::numeric_limits<int>::max();

	void render(Scene& scene, const Cameraf& camera, const SimpleMaterial& material, ColorImageR8G8B8A8& color, DepthImage32& depth);
	void render(Scene& scene, const Cameraf& camera, DepthImage32& depth);

	void includeInBoundingBox(BoundingBox3f& bb, const DepthImage16& depthImage, const Cameraf& camera);
	void boundDepthMap(float minDepth, float maxDepth, DepthImage16& depthImage);

	ml::ApplicationData& m_app;
	D3D11RenderTarget m_renderTarget;
};


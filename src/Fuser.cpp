
#include "stdafx.h"

#include "GlobalAppState.h"
#include "Fuser.h"
#include "MarchingCubes.h"


Fuser::Fuser(ml::ApplicationData& _app) : m_app(_app)
{
	const unsigned int imageWidth = GlobalAppState::get().s_imageWidth;
	const unsigned int imageHeight = GlobalAppState::get().s_imageHeight;
	m_renderTarget.init(m_app.graphics.castD3D11(), imageWidth, imageHeight);
}

Fuser::~Fuser()
{

}


bool Fuser::fuse(const std::string& outputFile, Scene& scene, const std::vector<Cameraf>& cameraTrajectory, bool debugOut /*= false*/)
{
	std::vector<Cameraf> cameras = cameraTrajectory;

	const unsigned int gridDim = GlobalAppState::get().s_gridDim;
	const unsigned int padding = GlobalAppState::get().s_padding;
	const float depthMin = GlobalAppState::get().s_depthMin;
	const float depthMax = GlobalAppState::get().s_depthMax;

	const unsigned int imageWidth = GlobalAppState::get().s_imageWidth;
	const unsigned int imageHeight = GlobalAppState::get().s_imageHeight;

	bool ok = true;

	for (auto& c : cameras) {
		c.updateAspectRatio((float)imageWidth / imageHeight);
	}

	BoundingBox3f bbox = scene.getBoundingBox();
	vec3f bboxDims = vec3f(bbox.getExtentX(), bbox.getExtentY(), bbox.getExtentZ()); // scale of object
	bbox = BoundingBox3f(bbox.getCenter() - 0.5f*bbox.getMaxExtent(), bbox.getCenter() + 0.5f*bbox.getMaxExtent());	//make space isotropic

	mat4f worldToVoxel = bbox.worldToCubeTransform();
	worldToVoxel = mat4f::scale(0.5f*(float)(gridDim - 1)) * mat4f::translation(1.0f) * worldToVoxel;
	worldToVoxel = mat4f::translation((float)padding) * mat4f::scale((float)((gridDim - 1) - 2 * padding) / (float)(gridDim - 1)) * worldToVoxel;
	if (debugOut) std::cout << "gridDim = " << gridDim << ", #cameras = " << cameras.size() << std::endl;

	const float voxelSize = 1.0f / (float)((gridDim - 1) - 2 * padding);
	VoxelGrid grid(vec3l(gridDim, gridDim, gridDim), worldToVoxel, voxelSize, depthMin, depthMax);

	PointCloudf pc;
	for (size_t i = 0; i < cameras.size(); i++) {
		const auto& c = cameras[i];

		DepthImage32 depth;
#pragma omp critical 
		{
			render(scene, c, depth);
		}

		if (debugOut && (i % 50) == 0) {//&& i == 0) {
			FreeImageWrapper::saveImage(util::removeExtensions(outputFile) + "_depth_rep" + std::to_string(i) + ".png", ColorImageR32G32B32(depth), true);
		}

		mat4f projToCamera = c.getProj().getInverse();
		mat4f cameraToWorld = c.getExtrinsic();
		mat4f projToWorld = cameraToWorld * projToCamera;
		mat4f intrinsic = Cameraf::graphicsToVisionProj(c.getProj(), depth.getWidth(), depth.getHeight());

		PointCloudf pcCurrFrame;
		for (auto &p : depth) {
			if (p.value != 0.0f && p.value != 1.0f) {
				vec3f posProj = vec3f(m_app.graphics.castD3D11().pixelToNDC(vec2i((int)p.x, (int)p.y), depth.getWidth(), depth.getHeight()), p.value);
				vec3f posCamera = projToCamera * posProj;
				vec3f posWorld = cameraToWorld * posCamera;

				if (posCamera.z >= depthMin && posCamera.z <= depthMax) {
					if (GlobalAppState::get().s_generatePointClouds) {
						pcCurrFrame.m_points.push_back(posWorld);
						pcCurrFrame.m_colors.push_back(vec4f((float)i));
					}
					p.value = posCamera.z;
				}
				else {
					p.value = 0.0f;
				}
			}
			else {
				p.value = 0.0f;
			}
		}

		grid.integrate(intrinsic, cameraToWorld, depth);

		if (GlobalAppState::get().s_generatePointClouds) {
			for (unsigned int i = 1; i < 5; i++) {
				PointCloudf pcSparse = pcCurrFrame;
				pcSparse.sparsifyUniform(GlobalAppState::get().s_maxPointsPerFrame * i);
				if (pcSparse.m_points.size() > GlobalAppState::get().s_maxPointsPerFrame) {
					pcCurrFrame = pcSparse;
					break;
				}
			}
			pc.merge(pcCurrFrame);
		}
	}

	if (GlobalAppState::get().s_generatePointClouds) {
		//pc.sparsifyUniform(GlobalAppState::get().s_maxPointsPerFrame);
		PointCloudIOf::saveToFile(util::removeExtensions(outputFile) + ".ply" , pc);
	}
	
	grid.normalizeSDFs();
	//grid.improveSDF(gridDim); //don't call this - more representative of real world scans anyways (currently it kills the known space)

	// check for invalid sdfs
	size_t numOcc;
	{
		BinaryGrid3 bg = grid.toBinaryGridOccupied(1, std::numeric_limits<float>::infinity());
		numOcc = bg.getNumOccupiedEntries();
		if (bg.getNumOccupiedEntries() == 0) {
			std::cout << outputFile << ": " << numOcc << std::endl;
			ok = false;
		}
	}
	if (ok) {
		grid.saveToFile(outputFile, bboxDims);
	}

	if (debugOut) {
		BinaryGrid3 bg = grid.toBinaryGridOccupied(1, 1.0f);
		std::cout << "num occupied entries: " << bg.getNumOccupiedEntries() << std::endl;
		MeshDataf meshBg = TriMeshf(bg).computeMeshData();
		MeshIOf::saveToFile(util::removeExtensions(outputFile) + "_BG.ply", meshBg);
		//MeshIOf::saveToFile(util::removeExtensions(outputFile) + "_KNW.ply", TriMeshf(grid.toBinaryGridKnown(1, 1.0f)).computeMeshData());
		MeshIOf::saveToFile(util::removeExtensions(outputFile) + "_MESH.ply", scene.getTriMesh(0).computeMeshData());

		{
			unsigned int width = (unsigned int)gridDim;
			unsigned int height = (unsigned int)gridDim;
			ColorImageR32G32B32 image(width, height);
			for (unsigned int j = 0; j < height; j++) {
				for (unsigned int i = 0; i < width; i++) {
					float dist = (grid)(i, j, grid.getDimZ() / 2).sdf;
					dist += 3;
					image(i, j) = BaseImageHelper::convertDepthToRGB(dist, 0.0f, (float)grid.getDimX() / 3.0f);
				}
			}
			FreeImageWrapper::saveImage(util::removeExtensions(outputFile) + "_test0.png", image, true);
		}

		{
			unsigned int width = (unsigned int)gridDim;
			unsigned int height = (unsigned int)gridDim;
			ColorImageR32G32B32 image(width, height);
			for (unsigned int j = 0; j < height; j++) {
				for (unsigned int i = 0; i < width; i++) {
					float dist = (grid)(i, j, grid.getDimZ() / 2).sdf;
					dist += 3;
					image(i, j) = BaseImageHelper::convertDepthToRGB(dist, 0.0f, (float)grid.getDimX() / 3.0f);
				}
			}
			FreeImageWrapper::saveImage(util::removeExtensions(outputFile) + "_test1.png", image, true);
		}

		{
			unsigned int width = (unsigned int)gridDim;
			unsigned int height = (unsigned int)gridDim;
			ColorImageR32G32B32 image(width, height);
			for (unsigned int j = 0; j < height; j++) {
				for (unsigned int i = 0; i < width; i++) {
					float dist = (grid)(i, j, grid.getDimZ() / 2).sdf;
					if (dist < 0) image(i, j) = vec3f(1.0f, 0.0f, 0.0f);
					else image(i, j) = vec3f(0.0f, 0.0f, 1.0f);
				}
			}
			FreeImageWrapper::saveImage(util::removeExtensions(outputFile) + "_test_negative.png", image, true);
		}
		{
			unsigned int width = (unsigned int)gridDim;
			unsigned int height = (unsigned int)gridDim;
			ColorImageR32G32B32 image(width, height);
			for (unsigned int j = 0; j < height; j++) {
				for (unsigned int i = 0; i < width; i++) {
					unsigned int weight = (grid)(i, j, grid.getDimZ() / 2).weight;
					if (weight == 0) image(i, j) = vec3f(1.0f, 0.0f, 0.0f);
					else image(i, j) = vec3f(0.0f, 0.0f, 1.0f);
				}
			}
			FreeImageWrapper::saveImage(util::removeExtensions(outputFile) + "_test_unknown.png", image, true);
		}

		VoxelGrid re = grid;
		re.setValues(Voxel());
		re.loadFromFile(outputFile);
		for (size_t z = 0; z < grid.getDimZ(); z++) {
			for (size_t y = 0; y < grid.getDimY(); y++) {
				for (size_t x = 0; x < grid.getDimX(); x++) {
					Voxel& v = grid(x, y, z);
					Voxel& r = re(x, y, z);
					if (v.sdf != r.sdf) {
						std::cout << "ERROR MISMATCH IN SDF" << std::endl;
					}
				}
			}
		}
		BinaryGrid3 reBG = re.toBinaryGridOccupied(0, 1.0f);
		if (reBG.getNumOccupiedEntries() != grid.toBinaryGridOccupied(1, 1.0f).getNumOccupiedEntries()) {
			std::cout << " ERROR different number of occupied entires " << std::endl;
		}

		MeshDataf meshWorld = TriMeshf(bg).computeMeshData();
		meshWorld.applyTransform(grid.getGridToWorld());
		MeshIOf::saveToFile(util::removeExtensions(outputFile) + "_BG-WORLD.ply", meshWorld);
	}
	return ok;
}

void Fuser::render(Scene& scene, const Cameraf& camera, const SimpleMaterial& material, ColorImageR8G8B8A8& color, DepthImage32& depth)
{
	auto &g = m_app.graphics.castD3D11();


	m_renderTarget.bind();
	m_renderTarget.clear(ml::vec4f(1.0f, 0.0f, 1.0f, 1.0f));

	scene.render(camera, material);

	m_renderTarget.unbind();

	m_renderTarget.captureColorBuffer(color);
	m_renderTarget.captureDepthBuffer(depth);
}

void Fuser::render(Scene& scene, const Cameraf& camera, DepthImage32& depth)
{
	auto &g = m_app.graphics.castD3D11();

	m_renderTarget.bind();
	m_renderTarget.clear(ml::vec4f(1.0f, 0.0f, 1.0f, 1.0f));

	scene.render(camera, SimpleMaterial::default());

	m_renderTarget.unbind();

	m_renderTarget.captureDepthBuffer(depth);
}

#pragma once 


struct ConstantBuffer
{
	mat4f worldViewProj;
	mat4f world;
	vec4f lightDir;
	vec4f eye;
};

class Fuser;
class TrajectoryGenerator;

class Visualizer : public ApplicationCallback
{
public:
	void init(ApplicationData &app);
	void render(ApplicationData &app);
	void keyDown(ApplicationData &app, UINT key);

	void fuse(ApplicationData & app);

	bool processModel(const MeshInfo& modelInfo, ApplicationData &app, Fuser &fuser,
		const TrajectoryGenerator* generator, const std::vector<std::vector<Cameraf>>& cameras, bool bTransformToBox, bool debugOut = false);

	void keyPressed(ApplicationData &app, UINT key);
	void mouseDown(ApplicationData &app, MouseButtonType button);
	void mouseMove(ApplicationData &app);
	void mouseWheel(ApplicationData &app, int wheelDelta);
	void resize(ApplicationData &app);

	void virtualScan(ApplicationData& app);

private:
	static size_t getNumCameraFiles(std::string cameradir) {
		if (!(cameradir.back() == '/' || cameradir.back() == '\\')) cameradir.push_back('/');
		Directory root(cameradir);
		size_t numFiles = 0;
		numFiles += root.getFilesWithSuffix(".cameras").size();
		if (numFiles == 0) {
			const std::vector<std::string>& subdirs = root.getDirectories();
			for (const std::string& subdir : subdirs)
				numFiles += Directory(cameradir + subdir).getFilesWithSuffix(".cameras").size();
		}
		return numFiles;
	}

	//debugging
	static void visualizeCameras(const std::string& filename, const std::vector<Cameraf>& cameras) {
		if (cameras.empty()) {
			std::cout << "no cameras to visualize!" << std::endl;
			return;
		}
		const float radius = 0.05f;
		MeshDataf meshData;
		for (unsigned int i = 0; i < cameras.size(); i++) {
			const vec3f eye = cameras[i].getEye(); const vec3f look = cameras[i].getLook(); const vec3f up = cameras[i].getUp();
			meshData.merge(Shapesf::sphere(radius, eye, 10, 10, vec4f(0.0f, 1.0f, 0.0f, 1.0f)).computeMeshData());				// eye: green
			meshData.merge(Shapesf::cylinder(eye, eye+look, radius, 10, 10, vec4f(1.0f, 0.0f, 0.0f, 1.0f)).computeMeshData());	// look: red
			meshData.merge(Shapesf::cylinder(eye, eye+up, radius, 10, 10, vec4f(0.0f, 0.0f, 1.0f, 1.0f)).computeMeshData());	// up: blue
		}
		MeshIOf::saveToPLY(filename, meshData);
	}

	MeshDirectory m_meshFiles;
	Scene m_scene;
	SimpleMaterial m_material;

	D3D11Font m_font;
	FrameTimer m_timer;

	D3D11ConstantBuffer<ConstantBuffer> m_constants;
	Cameraf m_camera;


	std::vector<Cameraf> m_recordedCameras;
	bool m_bEnableRecording;
	bool m_bEnableAutoRotate;
};
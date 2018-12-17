
#include "stdafx.h"
#include "GlobalAppState.h"
#include "Fuser.h"
#include "TrajectoryGenerator.h"
#include "omp.h"


void Visualizer::init(ApplicationData &app)
{
	const std::string fileNameDescGlobalApp = "zParametersDefault.txt";

	try{
		std::cout << VAR_NAME(fileNameDescGlobalApp) << " = " << fileNameDescGlobalApp << std::endl;
		ParameterFile parameterFileGlobalApp(fileNameDescGlobalApp);
		GlobalAppState::get().readMembers(parameterFileGlobalApp);
		GlobalAppState::get().print();
		if (GlobalAppState::get().s_scanWholeShape && GlobalAppState::get().s_numTrajectories > 1)
			throw MLIB_EXCEPTION("invalid configuration: cannot have >1 trajectory for scan whole shape");

		m_meshFiles.loadFromList(GlobalAppState::get().s_meshDirectory, GlobalAppState::get().s_meshFileList);
		//m_scene.loadMesh(app.graphics, m_meshFiles.getFiles().front());
	}
	catch (MLibException& e)
	{
		std::stringstream ss;
		ss << "exception caught: " << e.what() << std::endl;
		std::cout << ss.str() << std::endl;
		getchar();
		return;
	}

	vec3f eye = vec3f(
		0.5f*(m_scene.getBoundingBox().getMinX() + m_scene.getBoundingBox().getMaxX()),
		0.5f*(m_scene.getBoundingBox().getMinY() + m_scene.getBoundingBox().getMaxY()),
		m_scene.getBoundingBox().getMinZ()
		);
	eye = eye - 5.0f*(m_scene.getBoundingBox().getCenter() - eye);
	vec3f worldUp = vec3f::eY;

	m_constants.init(app.graphics);
	m_font.init(app.graphics, "Calibri");

	m_material = SimpleMaterial::default();

	m_bEnableRecording = true;
	m_bEnableAutoRotate = false;
	ml::RNG::global.init(0, 1, 2, 3);

	virtualScan(app);
	std::cout << "DONE DONE DONE" << std::endl;
	getchar();
}

void Visualizer::render(ApplicationData &app)
{
	if (m_bEnableAutoRotate) {
		vec3f dir = vec3f((float)RNG::global.rand_closed01() - 0.5f, (float)RNG::global.rand_closed01() - 0.5f, (float)RNG::global.rand_closed01() - 0.5f).getNormalized();
		vec3f worldUp = (dir ^ vec3f(dir.z, -dir.x, dir.y)).getNormalized();

		float dist = 1.5f*m_scene.getBoundingBox().getMaxExtent();
		vec3f eye = m_scene.getBoundingBox().getCenter() - dist*dir;

		vec3f lookDir = (m_scene.getBoundingBox().getCenter() - eye).getNormalized();
		m_camera.reset(eye, lookDir, worldUp);
	}

	m_timer.frame();

	m_scene.render(m_camera, m_material);

	m_font.drawString("FPS: " + convert::toString(m_timer.framesPerSecond()), vec2i(10, 5), 24.0f, RGBColor::Red);

	if (m_bEnableRecording) {
		m_recordedCameras.push_back(m_camera);
		m_font.drawString("RECORDING ON " + std::to_string(m_recordedCameras.size()), vec2i(10, 30), 24.0f, RGBColor::Red);
	}
}


void Visualizer::resize(ApplicationData &app)
{
	m_camera.updateAspectRatio((float)app.window.getWidth() / app.window.getHeight());
}

void Visualizer::keyDown(ApplicationData &app, UINT key)
{
	//if (key == KEY_F) app.graphics.castD3D11().toggleWireframe();

	if (key == KEY_U)
	{
		m_material = SimpleMaterial::randomMaterial(SimpleMaterial::AMBIENT | SimpleMaterial::DIFFUSE | SimpleMaterial::SPECULAR);
		std::cout << m_material << std::endl;
	}

	if (key == KEY_Y) {
		m_bEnableAutoRotate = !m_bEnableAutoRotate;
	}

	//record trajectory
	if (key == KEY_R) {
		if (m_bEnableRecording == false) {
			m_recordedCameras.clear();
			m_bEnableRecording = true;
		}
		else {
			m_bEnableRecording = false;
		}
	}
}

bool Visualizer::processModel(const MeshInfo& modelInfo, ApplicationData &app, Fuser &fuser,
	const TrajectoryGenerator* generator, const std::vector<std::vector<Cameraf>>& cameras, bool bTransformToBox, bool debugOut /*= false*/)
{
	const vec3f upDir = vec3f::eY;
	const unsigned int numTrajectories = GlobalAppState::get().s_numTrajectories;
	const bool bScanWholeShape = GlobalAppState::get().s_scanWholeShape;

	bool ok = true;

	const std::string cameraDir = GlobalAppState::get().s_cameraDir;
	const bool bGenerateCameras = generator != NULL && cameras.empty();
	const std::string outDirectory = GlobalAppState::get().s_outDirectory;

	const auto parts = util::splitPath(modelInfo.path);
	const std::string& outPrefix = outDirectory + "/" + modelInfo.classId + "/" + modelInfo.modelId;
	const std::string outFile = outPrefix + "__" + std::to_string(numTrajectories - 1) + "__" + ".fsdf";
	if (util::fileExists(outFile)) {
		std::stringstream ss;
		ss << "\tfound!\n";
		std::cout << ss.str();
		return ok;
	}

	try {
		MeshDataf meshData = MeshIOf::loadFromFile(modelInfo.path);
		meshData.clearAttributes();
		meshData.mergeCloseVertices(0.004f);
		meshData.removeDegeneratedFaces();
		meshData.removeDuplicateFaces();
		meshData.removeDuplicateVertices();
		meshData.removeIsolatedVertices();
		//MeshIOf::saveToFile("_after_filt.ply", meshData);
		if (meshData.m_Vertices.empty()) {
			std::cout << "skipping empty mesh" << std::endl;
			return false;
		}
		if (bTransformToBox) {
			const bbox3f shapenetBbox(vec3f(-0.5f), vec3f(0.5f)); // TODO MOVE OUT TO PARAMS

			bbox3f meshBox = meshData.computeBoundingBox();
			if (meshBox.getMaxX() > shapenetBbox.getMaxX() || meshBox.getMaxY() > shapenetBbox.getMaxY() || meshBox.getMaxZ() > shapenetBbox.getMaxZ() ||
				meshBox.getMinX() > shapenetBbox.getMinX() || meshBox.getMinY() > shapenetBbox.getMinY() || meshBox.getMinZ() > shapenetBbox.getMinZ()) {
				float maxExtent = std::max(meshBox.getExtentX(), std::max(meshBox.getExtentY(), meshBox.getExtentZ()));
				const float scale = maxExtent > 1.0f ? 1.0f / maxExtent : 1.0f;
				const mat4f transform = mat4f::scale(scale) * mat4f::translation(-meshBox.getCenter());
				meshData.applyTransform(transform);
			}
		}

		if (!meshData.isTriMesh()) meshData.makeTriMesh();
		TriMeshf triMeshOrig(meshData);
		BoundingBox3f bbOrig = triMeshOrig.computeBoundingBox();
#ifdef PRINT_CAMERAS
		std::vector<std::vector<Cameraf>> cameraTrajectories; //for meta-output
#endif
		for (unsigned int r = 0; r < numTrajectories; r++) {

			std::vector<Cameraf> cameraTrajectory;
			if (generator) {
				if (bScanWholeShape) {
					generator->generate360(bbOrig, cameraTrajectory, GlobalAppState::get().s_maxTrajectoryLength);
				}
				else {
					const std::vector<vec2f> angleRanges = { vec2f(1.0f, 1.0f), vec2f(1.0f, 90.0f), vec2f(91.0f, 360.0f) };
					unsigned int angleRangeIdx = std::rand() % 9;
					if (angleRangeIdx == 8) angleRangeIdx = 2;
					else angleRangeIdx = math::floor(0.25f * angleRangeIdx);  //bias towards more incomplete
					generator->generate(bbOrig, cameraTrajectory, GlobalAppState::get().s_maxTrajectoryLength, angleRanges[angleRangeIdx]);
				}
			}
			else {
				cameraTrajectory = cameras[r];
			}
#ifdef PRINT_CAMERAS
			if (bGenerateCameras) cameraTrajectories.push_back(cameraTrajectory);
#endif

			TriMeshf triMesh = triMeshOrig;

			Scene scene(app.graphics, triMesh);
			std::string outFile = outPrefix + "__" + std::to_string(r) + "__" + ".fsdf";
			const std::string outSubDir = util::directoryFromPath(outFile);
			if (!util::directoryExists(outSubDir)) util::makeDirectory(outSubDir);
			{
				bool b = fuser.fuse(outFile, scene, cameraTrajectory, debugOut);
				if (ok) ok = b;
			}
		}
#ifdef PRINT_CAMERAS
		if (bGenerateCameras && !cameraTrajectories.empty()) {
			const std::string camSubDir = cameraDir + "/" + modelInfo.classId + "/"; if (!util::directoryExists(camSubDir)) util::makeDirectory(camSubDir); //always use first camera dir for printing cameras
			const std::string cameraFile = camSubDir + "/" + modelInfo.modelId + ".cameras";
			BinaryDataStreamFile s(cameraFile, true);
			s << cameraTrajectories;
			s.close();
		}
#endif
		return ok;
	}
	catch (MLibException& e)
	{
		std::stringstream ss;
		ss << "exception caught at file " << modelInfo.path << " : " << e.what() << std::endl;
		std::cout << ss.str() << std::endl;
		//getchar();
		return false;
	}
	return false;
}

void Visualizer::keyPressed(ApplicationData &app, UINT key)
{
	const float distance = 0.1f;
	const float theta = 0.1f;

	if (key == KEY_S) m_camera.move(-distance);
	if (key == KEY_W) m_camera.move(distance);
	if (key == KEY_A) m_camera.strafe(-distance);
	if (key == KEY_D) m_camera.strafe(distance);
	if (key == KEY_E) m_camera.jump(distance);
	if (key == KEY_Q) m_camera.jump(-distance);

	if (key == KEY_UP) m_camera.lookUp(theta);
	if (key == KEY_DOWN) m_camera.lookUp(-theta);
	if (key == KEY_LEFT) m_camera.lookRight(theta);
	if (key == KEY_RIGHT) m_camera.lookRight(-theta);

	if (key == KEY_Z) m_camera.roll(theta);
	if (key == KEY_X) m_camera.roll(-theta);
}

void Visualizer::mouseDown(ApplicationData &app, MouseButtonType button)
{

}

void Visualizer::mouseWheel(ApplicationData &app, int wheelDelta)
{
	const float distance = 0.01f;
	m_camera.move(distance * wheelDelta);
}

void Visualizer::mouseMove(ApplicationData &app)
{
	const float distance = 0.05f;
	const float theta = 0.5f;

	vec2i posDelta = app.input.mouse.pos - app.input.prevMouse.pos;

	if (app.input.mouse.buttons[MouseButtonRight])
	{
		m_camera.strafe(-distance * posDelta.x);
		m_camera.jump(distance * posDelta.y);
	}

	if (app.input.mouse.buttons[MouseButtonLeft])
	{
		m_camera.lookRight(-theta * posDelta.x);
		m_camera.lookUp(theta * posDelta.y);
	}

}

void Visualizer::fuse(ApplicationData & app)
{
	if (m_recordedCameras.size() > 0) {

		const bool bDebugOut = GlobalAppState::get().s_debugOutput;
		const bool bTransformToUnitBox = GlobalAppState::get().s_transformToUnitBox;
		const unsigned int maxFrames = GlobalAppState::get().s_maxTrajectoryLength;
		if (m_recordedCameras.size() > maxFrames)  {
			m_recordedCameras.resize(maxFrames);
		}

		unsigned int maxModels = GlobalAppState::get().s_maxModels;
		if (maxModels == 0)	{
			maxModels = (unsigned int)m_meshFiles.getFiles().size();
		}
		else {
			maxModels = std::min(maxModels, (unsigned int)m_meshFiles.getFiles().size());
		}

		const int maxThreads = omp_get_max_threads();	//should be on a quad-core with hyper-threading

		std::vector<Fuser*> fusers(maxThreads);
		for (size_t i = 0; i < maxThreads; i++)	fusers[i] = new Fuser(app);

		if (!util::directoryExists(GlobalAppState::get().s_outDirectory))	util::makeDirectory(GlobalAppState::get().s_outDirectory);

		std::vector< std::vector<std::string> > badFiles(maxThreads);

#pragma omp parallel for
		for (int i = 0; i < (int)maxModels; i++) {
			int thread = omp_get_thread_num();
			Fuser& fuser = *fusers[thread];

			Timer t;
			const MeshInfo& modelInfo = m_meshFiles.getFiles()[i];

			bool ok = processModel(modelInfo, app, fuser, NULL, std::vector<std::vector<Cameraf>>(1, m_recordedCameras), bTransformToUnitBox, bDebugOut);
			if (!ok) {
				std::cout << "bad file: " << modelInfo.path << std::endl;
				badFiles[thread].push_back(modelInfo.path);
			}

			std::stringstream ss;
			ss << "\t[i " << i << "|" << m_meshFiles.getFiles().size() << "]" << "\n";
			ss << "time/model: " << t.getElapsedTimeMS() << " ms" << "\n";

			std::cout << ss.str();
		}
		// log bad files
		unsigned int badCount = 0;
		std::ofstream ofs(GlobalAppState::get().s_outDirectory + "bad.txt");
		for (unsigned int i = 0; i < badFiles.size(); i++) {
			for (unsigned int j = 0; j < badFiles[i].size(); j++) {
				ofs << badFiles[i][j] << std::endl;
				badCount++;
			}
		}
		ofs.close();
		std::cout << "#bad files = " << badCount << std::endl;

		std::cout << std::endl << std::endl << "All finished :)" << std::endl;

		m_bEnableRecording = false;
		m_recordedCameras.clear();

		for (size_t i = 0; i < maxThreads; i++) delete fusers[i];
	}
	else {
		std::cout << "need to record cameras first" << std::endl;
	}
}

void Visualizer::virtualScan(ApplicationData& app)
{
	const std::string outDir = GlobalAppState::get().s_outDirectory;
	const bool bDebugOut = GlobalAppState::get().s_debugOutput;
	const bool bTransformToUnitBox = GlobalAppState::get().s_transformToUnitBox;
	const unsigned int maxFrames = GlobalAppState::get().s_maxTrajectoryLength;
	unsigned int maxModels = GlobalAppState::get().s_maxModels;
	if (maxModels == 0)	{
		maxModels = (unsigned int)m_meshFiles.getFiles().size();
	}
	else {
		maxModels = std::min(maxModels, (unsigned int)m_meshFiles.getFiles().size());
	}
	const std::string& cameraDir = GlobalAppState::get().s_cameraDir;
	const bool bGenerateCameras = getNumCameraFiles(cameraDir) < maxModels;
	if (bGenerateCameras) {
		std::cout << "GENERATING CAMERAS to " << cameraDir << std::endl;
		std::cout << "(press key to continue)" << std::endl; getchar();
		if (!util::directoryExists(cameraDir)) util::makeDirectory(cameraDir);
	}
	else std::cout << "READING CAMERAS" << std::endl;
	const int maxThreads = omp_get_max_threads();	//should be on a quad-core with hyper-threading

	std::vector<Fuser*> fusers(maxThreads);
	std::vector<TrajectoryGenerator*> trajGenerators(maxThreads);
	for (size_t i = 0; i < maxThreads; i++)	{
		fusers[i] = new Fuser(app);
		trajGenerators[i] = bGenerateCameras ? new TrajectoryGenerator(true) : NULL;
	}

	if (!util::directoryExists(outDir))	util::makeDirectory(outDir);
	if (!util::directoryExists(outDir)) { std::cout << "failed to make output directory!" << std::endl; return; }

	std::vector< std::vector<std::string> > badFiles(maxThreads);

#pragma omp parallel for
	for (int i = 0; i < (int)maxModels; i++) {
		int thread = omp_get_thread_num();
		Fuser& fuser = *fusers[thread];
		TrajectoryGenerator* generator = trajGenerators[thread];

		Timer t;
		const MeshInfo& modelInfo = m_meshFiles.getFiles()[i];

		std::vector<std::vector<Cameraf>> cameras;
		if (!bGenerateCameras) {
			const std::string cameraFile = cameraDir + "/" + modelInfo.classId + "/" + modelInfo.modelId + ".cameras";
			if (util::fileExists(cameraFile)) {
				BinaryDataStreamFile s(cameraFile, false);
				s >> cameras; s.close();
			}
			else {
				std::cerr << "warning: camera file for " << modelInfo.path << " does not exist, skipping" << std::endl;
				badFiles[thread].push_back(modelInfo.path);
				continue;
			}
		}
		else {
			const std::string cameraFile = cameraDir + "/" + modelInfo.classId + "/" + modelInfo.modelId + ".cameras";
			if (util::fileExists(cameraFile)) {
				BinaryDataStreamFile s(cameraFile, false);
				s >> cameras; s.close();
			}
		}
		bool ok = processModel(modelInfo, app, fuser, generator, cameras, bTransformToUnitBox, bDebugOut);
		if (!ok) {
			std::cout << "bad file: " << modelInfo.path << std::endl;
			badFiles[thread].push_back(modelInfo.path);
		}

		std::stringstream ss;
		ss << "\t[i " << i << "|" << m_meshFiles.getFiles().size() << "]" << "\n";
		ss << "time/model: " << t.getElapsedTimeMS() << " ms" << "\n";
		std::cout << ss.str();
	}

	// log bad files
	std::string badFile = GlobalAppState::get().s_outDirectory + "/bad";
	if (util::fileExists(badFile + ".txt")) {
		unsigned int num = 0;
		while (util::fileExists(badFile + std::to_string(num) + ".txt"))
			num++;
		badFile = badFile + std::to_string(num);
	}
	unsigned int badCount = 0;
	std::ofstream ofs(badFile + ".txt");
	std::cout << badFile + ".txt" << std::endl;
	for (unsigned int i = 0; i < badFiles.size(); i++) {
		for (unsigned int j = 0; j < badFiles[i].size(); j++) {
			ofs << badFiles[i][j] << std::endl;
			std::cout << badFiles[i][j] << std::endl;
			badCount++;
		}
	}
	ofs.close();
	if (badCount == 0) util::deleteFile(badFile + ".txt");
	std::cout << "#bad files = " << badCount << std::endl;
	std::cout << std::endl << std::endl << "All finished :)" << std::endl;

	m_bEnableRecording = false;
	m_recordedCameras.clear();

	for (size_t i = 0; i < maxThreads; i++) {
		delete fusers[i];
		delete trajGenerators[i];
	}
}

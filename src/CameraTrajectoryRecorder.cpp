
#include "stdafx.h"

#include "CameraTrajectoryRecorder.h"

//
//void CameraTrajectoryRenderMode::extractAllTrajectoriesFromSensors()
//{
//    const string &sensorDirectory = R"(V:\data\scenegrok\scans\sensor\)";
//
//    for (const string &sensorFilename : Directory::enumerateFiles(sensorDirectory, ".sensor"))
//    {
//        const string targetFilename = sensorDirectory + ml::util::replace(sensorFilename, ".sensor", ".trajectory");
//        if (!ml::util::fileExists(targetFilename))
//        {
//            cout << "Creating camera trajectory for " << sensorDirectory << sensorFilename << endl;
//
//            //
//            // To Matthias: we really need input / output streams. If you accidentally make the 2nd parameter true you would delete all our sensor data...
//            //
//            BinaryDataStreamFile sensorFile(sensorDirectory + sensorFilename, false);
//            CalibratedSensorData sensor;
//            sensorFile >> sensor;
//            sensorFile.closeStream();
//            
//            CameraTrajectory trajectory;
//            trajectory.loadSensorFile(sensor);
//            BinaryDataStreamFile trajectoryFile(targetFilename, true);
//            trajectoryFile << trajectory.cameras;
//            trajectoryFile.closeStream();
//        }
//    }
//    
//}
//
//void CameraTrajectoryRenderMode::init(ml::ApplicationData& _app, AppState &_state)
//{
//    app = &_app;
//    state = &_state;
//	bRecordingActive = false;
//
//    trajectoryIndex = -1;
//
//    extractAllTrajectoriesFromSensors();
//}
//
//
//void CameraTrajectoryRenderMode::newScene(const string &filename)
//{
//	activeScene.load(filename, state->database);
//
//    auto &targetObject = ml::util::randomElement(activeScene.objects);
//    trajectory.loadCircleView(targetObject.worldBBox.getCenter());
//}
//
//
//void CameraTrajectoryRenderMode::render()
//{
//    if (trajectoryIndex != -1 && trajectory.cameras.size() > 0)
//    {
//        state->camera = trajectory.cameras[trajectoryIndex];
//        trajectoryIndex = (trajectoryIndex + 1) % trajectory.cameras.size();
//    }
//
//    vector<vec3f> objectColors(activeScene.objects.size(), vec3f(1, 1, 1));
//    state->renderer.renderObjects(state->assets, app->graphics.castD3D11(), state->camera.getCameraPerspective(), activeScene, objectColors, false);
//
//	if (bRecordingActive)
//    {
//        trajectory.cameras.push_back(state->camera);
//	}
//}
//
//void CameraTrajectoryRenderMode::keyDown(UINT key)
//{
//	if (key == KEY_DELETE) {
//		std::cout << "Camera trajectory cleared" << std::endl;
//        trajectory.cameras.clear();
//	}
//
//    if (key == KEY_P)
//    {
//        if (trajectoryIndex == -1)
//            trajectoryIndex = 0;
//        else
//            trajectoryIndex = -1;
//    }
//
//    if (key == KEY_O)
//    {
//        cout << "Begin fusion on trajectory with " << trajectory.cameras.size() << " frames" << endl;
//        Fuser fuser(*app, *state);
//        fuser.fuse(activeScene, trajectory.cameras);
//    }
//
//	if (key == KEY_R) {
//		if (!bRecordingActive) {
//			std::cout << "Recording started" << std::endl;
//			bRecordingActive = true;
//		}
//		else {
//			std::cout << "Recording stopped" << std::endl;
//			bRecordingActive = false;
//		}
//	}
//
//	if (key == KEY_Q) {
//		if (trajectory.cameras.size() > 0) {
//			
//		}
//		else {
//			std::cout << "need to record stuff first -> press 'R'" << std::endl;
//		}
//	}
//}
//
//void CameraTrajectoryRenderMode::mouseDown(ml::MouseButtonType button)
//{
//
//}
//
//string CameraTrajectoryRenderMode::helpText() const
//{
//    string text;
//    text += "R: toggle recording|";
//    text += "P: toggle playback|";
//    text += "O: run fusion|";
//	text += "<del>: reset trajectory";
//    return text;
//}
//
//vector< pair<string, RGBColor> > CameraTrajectoryRenderMode::statusText() const
//{
//    vector< pair<string, RGBColor> > result;
//
//	result.push_back(std::make_pair(string("Camera Trajectory Size: ") + to_string(trajectory.cameras.size()), RGBColor::White));
//
//    return result;
//}
//

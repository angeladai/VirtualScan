
#pragma once

#include "mLibInclude.h"

struct MeshInfo {
	std::string path;
	std::string classId;
	std::string modelId;
};

class MeshDirectory {
public:
	MeshDirectory() {
	}
	~MeshDirectory() {
	}

	void loadFromList(const std::string& dir, const std::string& fileList) {
		std::cerr << "warning: load uses hardcoded relative model paths for ShapeNetV2!" << std::endl;
		clear();
		std::ifstream s(fileList);
		if (!s.is_open()) throw MLIB_EXCEPTION("failed to open " + fileList);
		std::cout << "loading mesh files..." << std::endl;
		std::string meshFile;
		while (std::getline(s, meshFile)) {
			if (util::startsWith(meshFile, "//")) continue; // skip comment
			const auto parts = util::split(util::replace(meshFile, "\\", "/"), '/');
			MLIB_ASSERT(parts.size() == 2);
			std::string file = dir + "/" + meshFile + "/models/model_normalized.obj"; // hardcoded shapenet v2 paths!
			m_infos.push_back(MeshInfo{ file, parts[0], parts[1] });
		}
		std::cout << "found " << m_infos.size() << " mesh files" << std::endl;
	}

	const std::vector<MeshInfo>& getFiles() const {
		return m_infos;
	}

	bool isOrigShapenet() const {
		return m_bIsOrigShapenet;
	}

private:
	void clear() {
		m_infos.clear();
	}

	bool m_bIsOrigShapenet;
	std::vector<MeshInfo> m_infos;
};
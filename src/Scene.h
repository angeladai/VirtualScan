#pragma once

#include "GlobalAppState.h"
#include "SimpleMaterial.h"

class RenderObject {
public:
	RenderObject(GraphicsDevice& g, const TriMeshf& triMesh, const mat4f& modelToWorld) {
		m_triMesh.init(g, triMesh);
		m_modelToWorld = modelToWorld;

		for (const auto& v : triMesh.getVertices())  {
			m_boundingBoxWorld.include(m_modelToWorld * v.position);
		}
	}

	~RenderObject() {

	}

	const mat4f& getModelToWorld() const {
		return m_modelToWorld;
	}

	const D3D11TriMesh& getD3D11TriMesh() const {
		return m_triMesh;
	}

	const BoundingBox3f& getBoundingBoxWorld() const {
		return m_boundingBoxWorld;
	}

private:
	mat4f			m_modelToWorld;
	D3D11TriMesh	m_triMesh;
	BoundingBox3f	m_boundingBoxWorld;
};



class Scene
{
public:
	Scene() {

	}

	Scene(GraphicsDevice& g, const std::string& meshFilename) {
		loadMesh(g, meshFilename);
	}

	Scene(GraphicsDevice& g, const TriMeshf& triMesh) {
		loadMesh(g, triMesh);
	}

	~Scene() {

	}

	void loadMesh(GraphicsDevice& g, const std::string& meshFilename) {
		MeshDataf meshData = MeshIOf::loadFromFile(meshFilename);
		MLIB_ASSERT(meshData.isConsistent());
		unsigned int origNumVerts = (unsigned int)meshData.m_Vertices.size();
		unsigned int newNumVerts = meshData.removeIsolatedVertices();
		if (origNumVerts != newNumVerts) {
			meshData.applyTransform(mat4f::scale(1.0f / meshData.computeBoundingBox().getMaxExtent()));
		}

		TriMeshf triMesh(meshData);		
		if (!triMesh.hasNormals()) 
			triMesh.computeNormals();

		loadMesh(g, triMesh);
	}

	void loadMesh(GraphicsDevice& g, const TriMeshf& triMesh) {
		m_graphics = &g;

		m_cbCamera.init(g);
		m_cbMaterial.init(g);
		m_cbLight.init(g);

		//TODO factor our the shader loading; ideally every object has a shader
		m_shaders.init(g);
		m_shaders.registerShader("shaders/depthOnly.hlsl", "depthOnly");
		m_shaders.registerShader("shaders/specular.hlsl", "specular");
		m_shaders.registerShader("shaders/test.hlsl", "test");

		mat4f modelToWorld = mat4f::identity();
		m_objects.push_back(RenderObject(*m_graphics, triMesh, modelToWorld));

		for (const RenderObject& o : m_objects) {
			m_boundingBox.include(o.getBoundingBoxWorld());
		}
	}


	void render(const Cameraf& camera, const SimpleMaterial& material = SimpleMaterial::default()) {

		ConstantBufferLight cbLight;
		cbLight.lightDir = vec4f(1.0f, 1.0f, 0.0f, 0.0f).getNormalized();
		m_cbLight.updateAndBind(cbLight, 2);

		for (const RenderObject& o : m_objects) {
			ConstantBufferCamera cbCamera;
			cbCamera.worldViewProj = camera.getProj() * camera.getView() * o.getModelToWorld();
			cbCamera.world = o.getModelToWorld();
			cbCamera.eye = vec4f(camera.getEye());
			m_cbCamera.updateAndBind(cbCamera, 0);

			ConstantBufferMaterial cbMaterial;
			cbMaterial.ambient = material.ambient;
			cbMaterial.diffuse = material.diffuse;
			cbMaterial.specular = material.specular;
			cbMaterial.shiny = material.shiny;
			m_cbMaterial.updateAndBind(cbMaterial, 1);

			m_shaders.bindShaders("specular");

			o.getD3D11TriMesh().render();
		}
	} 

	void renderDepth(const Cameraf& camera) {
		m_shaders.bindShaders("depthOnly");

		for (const RenderObject& o : m_objects) {
			ConstantBufferCamera cbCamera;
			cbCamera.worldViewProj = camera.getProj() * camera.getView() * o.getModelToWorld();
			cbCamera.world = o.getModelToWorld();
			cbCamera.eye = vec4f(camera.getEye());
			m_cbCamera.updateAndBind(cbCamera, 0);

			o.getD3D11TriMesh().render();
		}
	}

	const BoundingBox3f& getBoundingBox() const {
		return m_boundingBox;
	}

	const TriMeshf& getTriMesh(unsigned int idx) {
		MLIB_ASSERT(idx < m_objects.size());
		return m_objects[idx].getD3D11TriMesh().getTriMesh();
	}

private:

	struct ConstantBufferCamera {
		mat4f worldViewProj;
		mat4f world;
		vec4f eye;
	};

	struct ConstantBufferMaterial {
		vec4f ambient;
		vec4f diffuse;
		vec4f specular;
		float shiny;
		vec3f dummy;
	};

	struct ConstantBufferLight {
		vec4f lightDir;
	};

	GraphicsDevice* m_graphics;

	D3D11ShaderManager m_shaders;

	D3D11ConstantBuffer<ConstantBufferCamera>	m_cbCamera;
	D3D11ConstantBuffer<ConstantBufferLight>	m_cbLight;
	D3D11ConstantBuffer<ConstantBufferMaterial> m_cbMaterial;

	std::vector<RenderObject> m_objects;
	BoundingBox3f m_boundingBox;

};


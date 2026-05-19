#pragma once
#ifndef LODMODEL_H
#define LODMODEL_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "shader.h"
#include "simplyMesh.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

#pragma region 函数声明
unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);
#pragma endregion

#pragma region 基本数据类型
class LODModel {
public:
	LODModel(string path);
	void Draw(Shader& shader, float LODratio);
private:
	vector<SimplyMesh> simplyMeshLOD0; // 100% 保有率
	vector<SimplyMesh> simplyMeshLOD1; // 90%  保有率
	vector<SimplyMesh> simplyMeshLOD2; // 80%  保有率
	vector<SimplyMesh> simplyMeshLOD3; // 70%  保有率
	vector<SimplyMesh> simplyMeshLOD4; // 60%  保有率
	vector<SimplyMesh> simplyMeshLOD5; // 50%  保有率
	string directory;
	vector<Texture> textures_loaded; // 全局读取过的材质缓存 只供简化计算使用

	void LoadModel(string path);
	void ProcessNode(aiNode* node, const aiScene* scene);
	SimplyMesh ProcessMesh(aiMesh* mesh, const aiScene* scene, float LODratio);
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
};
#pragma endregion

#pragma region 关键函数实现
LODModel::LODModel(string path) {
	LoadModel(path);
}

void LODModel::LoadModel(string path) {
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	// 安全校验 场景读取是否完整
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
		return;
	}

	// 处理目录 保留文件名前的路径
	directory = path.substr(0, path.find_last_of('/'));

	ProcessNode(scene->mRootNode, scene);
}

void LODModel::ProcessNode(aiNode* node, const aiScene* scene) {
	for (unsigned int i = 0; i < node -> mNumMeshes; i++) {
		aiMesh* mesh = scene -> mMeshes[node->mMeshes[i]];
		simplyMeshLOD0.push_back(ProcessMesh(mesh, scene, 1.0f));
		simplyMeshLOD1.push_back(ProcessMesh(mesh, scene, 0.9f));
		simplyMeshLOD2.push_back(ProcessMesh(mesh, scene, 0.8f));
		simplyMeshLOD3.push_back(ProcessMesh(mesh, scene, 0.7f));
		simplyMeshLOD4.push_back(ProcessMesh(mesh, scene, 0.6f));
		simplyMeshLOD5.push_back(ProcessMesh(mesh, scene, 0.5f));
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		ProcessNode(node->mChildren[i], scene);
	}
}

SimplyMesh LODModel::ProcessMesh(aiMesh* mesh, const aiScene* scene, float LODratio) {
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;
	
	// 遍历网格的每一个顶点
	for (unsigned int i = 0; i < mesh -> mNumVertices; i++) {
		// 处理顶点数据集
		Vertex vertex;
		glm::vec3 vector;

		vector.x = mesh -> mVertices[i].x;
		vector.y = mesh -> mVertices[i].y;
		vector.z = mesh -> mVertices[i].z;
		vertex.Position = vector;

		// 法线信息滤过

		// 纹理坐标 我们只用第一个纹理
		if(mesh -> mTextureCoords[0]){
			glm::vec2 vec;
			vec.x = mesh -> mTextureCoords[0][i].x;
			vec.y = mesh -> mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		}

		vertices.push_back(vertex); // 存入一个顶点数据
	}

	// 处理顶点索引
	for (unsigned int i = 0; i < mesh->mNumFaces; i ++) {
		aiFace face = mesh -> mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	// 处理材质
	if(mesh -> mMaterialIndex > 0){
		aiMaterial* material = scene -> mMaterials[mesh -> mMaterialIndex];
		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	}

	return SimplyMesh(vertices, indices, textures, LODratio);
}

vector<Texture> LODModel::loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName) {
	vector<Texture> textures;
	for(unsigned int i = 0; i < mat -> GetTextureCount(type); i++){
		aiString str; // 存储纹理的路径
		mat -> GetTexture(type, i, &str);
		bool skip = false;
		for (unsigned int j = 0; j < textures_loaded.size(); j++) {
			if (strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
				textures.push_back(textures_loaded[j]); // 直接从已经存储的纹理集中取
				skip = true;
				break;
			}
		}

		if (!skip) {
			Texture texture;
			texture.ID = TextureFromFile(str.C_Str(), directory);
			texture.Type = typeName;
			texture.path = str.C_Str();

			textures.push_back(texture);
			textures_loaded.push_back(texture);
		}
	}

	return textures;
}

void LODModel::Draw(Shader& shader, float LODratio) {
	if (LODratio == 0.8f) {
		for (unsigned int i = 0; i < simplyMeshLOD2.size(); i++) {
			simplyMeshLOD2[i].Draw(shader);
		}
	}
	else if (LODratio == 0.9f) {
		for (unsigned int i = 0; i < simplyMeshLOD1.size(); i++) {
			simplyMeshLOD1[i].Draw(shader);
		}
	}
	else if(LODratio == 0.7f){
		for (unsigned int i = 0; i < simplyMeshLOD3.size(); i++) {
			simplyMeshLOD3[i].Draw(shader);
		}
	}
	else if (LODratio == 0.6f) {
		for (unsigned int i = 0; i < simplyMeshLOD4.size(); i++) {
			simplyMeshLOD4[i].Draw(shader);
		}
	}
	else if (LODratio == 0.5f) {
		for (unsigned int i = 0; i < simplyMeshLOD5.size(); i++) {
			simplyMeshLOD5[i].Draw(shader);
		}
	}
	else {
		for (unsigned int i = 0; i < simplyMeshLOD0.size(); i++) {
			simplyMeshLOD0[i].Draw(shader);
		}
	}
}
#pragma endregion

#pragma region 辅助函数
unsigned int TextureFromFile(const char* path, const string& directory, bool gamma) {
	string fileName = string(path);
	fileName = directory + '/' + fileName; // 找到材质的绝对路径

	unsigned int TextureID;
	glGenTextures(1, &TextureID); // 申请一份GPU材质存储地址

	int width, height, nrComponents;
	unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &nrComponents, 0); // 从绝对路径中读出数据
	if (data) {
		GLenum format;
		if (nrComponents == 1) {
			format = GL_RED;
		}
		else if (nrComponents == 3) {
			format = GL_RGB;
		}
		else if (nrComponents == 4) {
			format = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, TextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}

	return TextureID;
}

#pragma endregion

#endif

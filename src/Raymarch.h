#ifndef RAY_MARCH_H
#define RAY_MARCH_H

#include <string>
#include "scene.h"
#include "shader.h"

#include <nlohmann/json.hpp>

class Raymarch : public Scene
{
public:
	Raymarch();
	void setRootDir(const std::string& rootDir);
	void setSize(int width, int height);
	void setNearPlane(float near);
	void setFovy(float vfy);
	void setCameraMatrix(glm::mat4 world_T_camera);
	bool initScene() override;
	void update(float t) override;
	void render() override;
	void resize(int, int) override;
	void releaseScene() override;

	nlohmann::json gSceneParams;
private:
	// rendering variables
	Shader shader;
	unsigned int VBO, VAO, EBO;
	std::string vertexShaderSrc;
	std::string fragmentShaderSrc;
	unsigned int screenWidth;
	unsigned int screenHeight;
	float nearPlane;
	float vfovy;

	// modeling variables
	std::string m_rootDir;
	GLuint rgbVolumeTexture;
	GLuint alphaVolumeTexture;
	GLuint featureVolumeTexture;
	GLuint atlasIndexTexture;
	GLuint weightsTexZero;
	GLuint weightsTexOne;
	GLuint weightsTexTwo;
	unsigned int indices[6] = { 0, 2, 1, 2, 3, 1 };
	std::vector<float> vertices;

	// scene functions
	bool loadScene(const std::string& dirUrl, int width, int height);
	void createRayMarchMaterial();
	void createPlaneBufferGeometry(float width, float height, float z = 0.0f);
	
	static GLuint createDummyNetworkWeightTexture();
	static GLuint createNetworkWeightTexture(nlohmann::json network_weights);
    static GLuint createFloatTextureFromData(int width, int height, float* data);
	std::string createViewDependenceFunctions() const;

	// utils
	static std::string digits(int d, int w);
	static std::string replaceAll(std::string str, const std::string& from, const std::string& to);
};


#endif // !RAY_MARCH_H

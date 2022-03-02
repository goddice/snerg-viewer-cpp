#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glad/gl.h>
#include "Raymarch.h"
#include "constants.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "PerspectiveCamera.h"
#include "TrackballControls.h"

Raymarch::Raymarch():
    shader(),
    VBO(0),
    VAO(0),
    EBO(0),
    vertexShaderSrc(""),
    fragmentShaderSrc(""),
    nearPlane(0.33f),
    vfovy(35.0f),
    m_rootDir(""),
    rgbVolumeTexture(0),
    alphaVolumeTexture(0),
    featureVolumeTexture(0),
    atlasIndexTexture(0),
    weightsTexZero(0),
    weightsTexOne(0),
    weightsTexTwo(0),
    screenWidth(1280),
    screenHeight(720) {}

void Raymarch::setRootDir(const std::string& rootDir) {
    m_rootDir = rootDir;
}

void Raymarch::setSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void Raymarch::setNearPlane(float near) {
    nearPlane = near;
}

void Raymarch::setFovy(float vfy) {
    vfovy = vfy;
}

bool Raymarch::initScene() {

    if (m_rootDir == "") {
        std::cout << "Please set the root directory of the scene\n";
        return false;
    }

    if (!loadScene(m_rootDir, screenWidth, screenHeight)) {
        return false;
    }

    vertexShaderSrc = rayMarchVertexShader;
	shader.initialize(vertexShaderSrc, fragmentShaderSrc);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    // Now pass all the 3D textures as uniforms to the shader
    nlohmann::json M_dict = gSceneParams["worldspace_T_opengl"];
    float worldspace_R_opengl[] = {
        M_dict[0][0], M_dict[0][1], M_dict[0][2],
        M_dict[1][0], M_dict[1][1], M_dict[1][2],
        M_dict[2][0], M_dict[2][1], M_dict[2][2]
    };

    float ndc_f = 755.644059435f;
    float ndc_w = 1006.0f;
    float ndc_h = 756.0f;
    if (gSceneParams.contains("input_focal")) {
        ndc_f = gSceneParams["input_focal"];
        ndc_w = gSceneParams["input_width"];
        ndc_h = gSceneParams["input_height"];
    }

    float blockSize = gSceneParams["block_size"];
    float voxelSize = gSceneParams["voxel_size"];
    float minPosition[3] = { gSceneParams["min_x"], gSceneParams["min_y"] , gSceneParams["min_z"] };

    float modelViewMatrix[] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -100, 1 };
    float z_far = 10000.0f;
    float z_near = -10000.0f;
    float projectionMatrix[] = { 
        2.0f / screenWidth, 0, 0, 0,
        0, 2.0f / screenHeight, 0, 0,
        0, 0, -2.0f / (z_far - z_near), 0,
        0, 0, 0, 1 };

    float gridWidth = gSceneParams["grid_width"];
    float gridHeight = gSceneParams["grid_height"];
    float gridDepth = gSceneParams["grid_depth"];
    float atlasWidth = gSceneParams["atlas_width"];
    float atlasHeight = gSceneParams["atlas_height"];
    float atlasDepth = gSceneParams["atlas_depth"];

    shader.use();
    shader.setInt("mapAlpha", 0);
    shader.setInt("mapColor", 1);
    shader.setInt("mapFeatures", 2);
    shader.setInt("mapIndex", 3);
    shader.setInt("weightsZero", 4);
    shader.setInt("weightsOne", 5);
    shader.setInt("weightsTwo", 6);
    shader.setInt("displayMode", DISPLAY_NORMAL);
    shader.setInt("ndc", 0);
    shader.setFloat("nearPlane", nearPlane);
    shader.setFloat("blockSize", blockSize);
    shader.setFloat("voxelSize", voxelSize);
    shader.setVec3("minPosition", minPosition[0], minPosition[1], minPosition[2]);
    shader.setFloat("ndc_f", ndc_f);
    shader.setFloat("ndc_w", ndc_w);
    shader.setFloat("ndc_h", ndc_h);
    glUniformMatrix3fv(glGetUniformLocation(shader.ID, "worldspace_R_opengl"), 1, GL_FALSE, worldspace_R_opengl);
    shader.setVec3("gridSize", gridWidth, gridHeight, gridDepth);
    shader.setVec3("atlasSize", atlasWidth, atlasHeight, atlasDepth);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "modelViewMatrix"), 1, GL_FALSE, modelViewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projectionMatrix"), 1, GL_FALSE, projectionMatrix);
    shader.turnOff();

    return true;
}

void Raymarch::setCameraMatrix(glm::mat4 world_T_camera) {
    PerspectiveCamera cam(35.0f, 1.0f * screenWidth / screenHeight, 0.33f, 100.0f);
    world_T_camera[3][1] = 1.0f;
    cam.updateProjectionMatrix();
    glm::mat4 camera_T_clip = glm::make_mat4(cam.projectionInv);
    glm::mat4 world_T_clip = world_T_camera * camera_T_clip;
    shader.use();
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "world_T_clip"), 1, GL_FALSE, &world_T_clip[0][0]);
    shader.turnOff();
}

void Raymarch::update(float t) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Raymarch::render() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, alphaVolumeTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, rgbVolumeTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, featureVolumeTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, atlasIndexTexture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, weightsTexZero);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, weightsTexOne);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, weightsTexTwo);

    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Raymarch::resize(int width, int height) {
    glViewport(0, 0, width, height);
}

void Raymarch::releaseScene() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shader.ID);
}

bool Raymarch::loadScene(const std::string& dirUrl, int width, int height) {
    int gNumTextures = 0;

    int image_width;
    int image_height;
    int image_channel;

    std::string sceneParamsUrl = dirUrl + "/" + "scene_params.json";

    std::ifstream sceneParamsInFileStream(sceneParamsUrl);
    nlohmann::json sceneParamsRaw = nlohmann::json::parse(sceneParamsInFileStream);
    sceneParamsInFileStream.close();
    std::cout << "Loaded " << sceneParamsUrl << "\n";

    gSceneParams = sceneParamsRaw;
    gSceneParams["dirUrl"] = dirUrl;
    gSceneParams["loadingTextures"] = false;
    gSceneParams["diffuse"] = true;

    // If we have a view-dependence network in the json file, turn on view
    // dependence.
    if (gSceneParams.contains("0_bias")) {
        gSceneParams["diffuse"] = false;
    }
    gNumTextures = gSceneParams["num_slices"];
    std::cout << "num_slices: " << gNumTextures << "\n";

    // generate texture 3d data

    unsigned long long atlas_width = gSceneParams["atlas_width"];
    unsigned long long atlas_height = gSceneParams["atlas_height"];
    unsigned long long atlas_depth = gSceneParams["atlas_depth"];
    unsigned long long slice_depth = 4;

    // generate rgbVolumeTexture
    glGenTextures(1, &rgbVolumeTexture);
    glBindTexture(GL_TEXTURE_3D, rgbVolumeTexture);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGB8, (GLsizei)atlas_width, (GLsizei)atlas_height, (GLsizei)atlas_depth);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // generate alphaVolumeTexture [TODO mipmaps]
    glGenTextures(1, &alphaVolumeTexture);
    glBindTexture(GL_TEXTURE_3D, alphaVolumeTexture);
    glTexStorage3D(GL_TEXTURE_3D, 8, GL_R8, (GLsizei)atlas_width, (GLsizei)atlas_height, (GLsizei)atlas_depth);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // feed data

    for (int i = 0; i < gNumTextures; i++) {
        std::string rgbaUrl = dirUrl + "/rgba_" + digits(i, 3) + ".png";
        unsigned char* rgbaPixels = stbi_load(rgbaUrl.c_str(), &image_width, &image_height, &image_channel, 4);
        if (rgbaPixels == nullptr) {
            std::cout << "Error in loading " << rgbaUrl << "\n";
            return false;
        }
        std::cout << "Loaded " << rgbaUrl << " with w=" << image_width << ", h=" << image_height << ", c=" << image_channel << "\n";

        unsigned char* rgbPixels = new unsigned char[atlas_width * atlas_height * slice_depth * 3];
        unsigned char* alphaPixels = new unsigned char[atlas_width * atlas_height * slice_depth * 1];

        for (int j = 0; j < atlas_height * atlas_height * slice_depth; j++) {
            rgbPixels[j * 3 + 0] = rgbaPixels[j * 4 + 0];
            rgbPixels[j * 3 + 1] = rgbaPixels[j * 4 + 1];
            rgbPixels[j * 3 + 2] = rgbaPixels[j * 4 + 2];
            alphaPixels[j] = rgbaPixels[j * 4 + 3];
        }

        glBindTexture(GL_TEXTURE_3D, rgbVolumeTexture);
        for (int z = 0; z < slice_depth; ++z) {
            for (int y = 0; y < atlas_height; ++y) {
                glTexSubImage3D(
                    GL_TEXTURE_3D, 0, 0, y, z + i * (GLsizei)slice_depth,
                    (GLsizei)atlas_width, 1, 1, GL_RGB, GL_UNSIGNED_BYTE,
                    rgbPixels + 3 * atlas_width * (y + atlas_height * z));
            }
        }

        glBindTexture(GL_TEXTURE_3D, alphaVolumeTexture);
        for (int z = 0; z < slice_depth; ++z) {
            for (int y = 0; y < atlas_height; ++y) {
                glTexSubImage3D(
                    GL_TEXTURE_3D, 0, 0, y, z + i * (GLsizei)slice_depth,
                    (GLsizei)atlas_width, 1, 1, GL_RED, GL_UNSIGNED_BYTE,
                    alphaPixels + atlas_width * (y + atlas_height * z));
            }
        }

        delete[] rgbPixels;
        delete[] alphaPixels;
        stbi_image_free(rgbaPixels);
    }

    glBindTexture(GL_TEXTURE_3D, alphaVolumeTexture);
    glGenerateMipmap(GL_TEXTURE_3D);

    std::cout << "rgbVolumeTexture " << rgbVolumeTexture << " creation done\n";
    std::cout << "alphaVolumeTexture " << alphaVolumeTexture << " creation done\n";

    // generate featureVolumeTexture
    featureVolumeTexture = 0;
    if (!gSceneParams["diffuse"]) {
        glGenTextures(1, &featureVolumeTexture);
        glBindTexture(GL_TEXTURE_3D, featureVolumeTexture);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, (GLsizei)atlas_width, (GLsizei)atlas_height, (GLsizei)atlas_depth);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // feed data
        for (int i = 0; i < gNumTextures; i++) {
            std::string rgbaUrl = dirUrl + "/feature_" + digits(i, 3) + ".png";
            unsigned char* rgbaImage = stbi_load(rgbaUrl.c_str(), &image_width, &image_height, &image_channel, 4);
            if (rgbaImage == nullptr) {
                std::cout << "Error in loading " << rgbaUrl << "\n";
                return false;
            }
            std::cout << "Loaded " << rgbaUrl << " with w=" << image_width << ", h=" << image_height << ", c=" << image_channel << "\n";

            glBindTexture(GL_TEXTURE_3D, featureVolumeTexture);
            for (int z = 0; z < slice_depth; ++z) {
                for (int y = 0; y < atlas_height; ++y) {
                    glTexSubImage3D(
                        GL_TEXTURE_3D, 0, 0, y, z + i * (GLsizei)slice_depth,
                        (GLsizei)atlas_width, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                        rgbaImage + 4 * atlas_width * (y + atlas_height * z));
                }
            }

            stbi_image_free(rgbaImage);
        }
        std::cout << "featureVolumeTexture " << featureVolumeTexture << " creation done\n";
    }

    // generate atlasIndexTexture
    glGenTextures(1, &atlasIndexTexture);
    glBindTexture(GL_TEXTURE_3D, atlasIndexTexture);
    int grid_width = gSceneParams["grid_width"];
    int grid_height = gSceneParams["grid_height"];
    int grid_depth = gSceneParams["grid_depth"];
    int block_size = gSceneParams["block_size"];
    std::string atlasIndexUrl = dirUrl + "/" + "atlas_indices.png";
    unsigned char* atlasIndexImage = stbi_load(atlasIndexUrl.c_str(), &image_width, &image_height, &image_channel, 0);
    if (atlasIndexImage == nullptr) {
        std::cout << "Error in loading " << atlasIndexUrl << "\n";
        return false;
    }
    std::cout << "Loaded " << atlasIndexUrl << " with w=" << image_width << ", h=" << image_height << ", c=" << image_channel << "\n";

    unsigned char* atlasIndexImageC4Ptr = new unsigned char[image_width * image_height * 4];
    for (int i = 0; i < image_width * image_height; i++) {
        atlasIndexImageC4Ptr[i * 4] = atlasIndexImage[i * image_channel];
        atlasIndexImageC4Ptr[i * 4 + 1] = atlasIndexImage[i * image_channel + 1];
        atlasIndexImageC4Ptr[i * 4 + 2] = atlasIndexImage[i * image_channel + 2];
        atlasIndexImageC4Ptr[i * 4 + 3] = 255;
    }

    unsigned long long indexImageWidth = (unsigned long long)std::ceil(1.0 * grid_width / block_size);
    unsigned long long indexImageHeight = (unsigned long long)std::ceil(1.0 * grid_height / block_size);
    unsigned long long indexImageDepth = (unsigned long long)std::ceil(1.0 * grid_depth / block_size);
    std::cout << "Index Image with w=" << indexImageWidth << ", h=" << indexImageHeight << ", d=" << indexImageDepth << "\n";
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, (GLsizei)indexImageWidth, (GLsizei)indexImageHeight, (GLsizei)indexImageDepth);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexSubImage3D(
        GL_TEXTURE_3D, 0,
        0, 0, 0,
        (GLsizei)indexImageWidth, (GLsizei)indexImageHeight, (GLsizei)indexImageDepth, GL_RGBA, GL_UNSIGNED_BYTE,
        atlasIndexImageC4Ptr);
    // free image
    stbi_image_free(atlasIndexImage);
    delete[] atlasIndexImageC4Ptr;
    std::cout << "atlasIndexTexture " << atlasIndexTexture << " creation done\n";

    createPlaneBufferGeometry((float)width, (float)height, -100.0f);
    createRayMarchMaterial();

    return true;
}

void Raymarch::createRayMarchMaterial() {
    weightsTexZero = 0;
    weightsTexOne = 0;
    weightsTexTwo = 0;

    fragmentShaderSrc = rayMarchFragmentShaderHeader;
    if (gSceneParams["diffuse"]) {
        fragmentShaderSrc += dummyViewDependenceShaderFunctions;
        weightsTexZero = createDummyNetworkWeightTexture();
        weightsTexOne = createDummyNetworkWeightTexture();
        weightsTexTwo = createDummyNetworkWeightTexture();
    }
    else {
        fragmentShaderSrc += createViewDependenceFunctions();
        weightsTexZero = createNetworkWeightTexture(gSceneParams["0_weights"]);
        weightsTexOne = createNetworkWeightTexture(gSceneParams["1_weights"]);
        weightsTexTwo = createNetworkWeightTexture(gSceneParams["2_weights"]);
    }
    fragmentShaderSrc += rayMarchFragmentShaderBody;

    std::cout << "weightsTexZero " << weightsTexZero << " creation done\n";
    std::cout << "weightsTexOne " << weightsTexOne << " creation done\n";
    std::cout << "weightsTexTwo " << weightsTexTwo << " creation done\n";
}

std::string Raymarch::digits(int d, int w) {
    std::stringstream ss;
    ss << std::setw(w) << std::setfill('0') << d;
    return ss.str();
}

void Raymarch::createPlaneBufferGeometry(float width, float height, float z) {
    float width_half = width / 2.0f;
    float height_half = height / 2.0f;

    float segment_width = width;
    float segment_height = height;

    for (int iy = 0; iy < 2; iy++) {
        float y = iy * segment_height - height_half;
        for (int ix = 0; ix < 2; ix++) {
            float x = ix * segment_width - width_half;
            vertices.push_back(x);
            vertices.push_back(-y);
            vertices.push_back(z);
        }
    }
}

GLuint Raymarch::createDummyNetworkWeightTexture() {
    GLuint texture;
    float* weightsData = new float[1];
    weightsData[0] = 0.0f;
    texture = createFloatTextureFromData(1, 1, weightsData);
    delete[] weightsData;
    return texture;
}

GLuint Raymarch::createNetworkWeightTexture(nlohmann::json network_weights) {
    size_t width = network_weights.size();
    size_t height = network_weights[0].size();
    std::vector<float> weightsData(width * height);
    for (int co = 0; co < height; co++) {
        for (int ci = 0; ci < width; ci++) {
            size_t index = co * width + ci;
            float weight = network_weights[ci][co];
            weightsData[index] = weight;
        }
    }
    return createFloatTextureFromData((int)width, (int)height, weightsData.data());
}

GLuint Raymarch::createFloatTextureFromData(int width, int height, float* data) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data);
    return texture;
}

std::string Raymarch::createViewDependenceFunctions() {
    nlohmann::json network_weights = gSceneParams;
    
    size_t width = network_weights["0_bias"].size();
    std::string biasListZero = "";
    for (int i = 0; i < width; i++) {
        float bias = network_weights["0_bias"][i];
        biasListZero += std::to_string(bias);
        if (i + 1 < width) {
            biasListZero += ", ";
        }
    }

    width = network_weights["1_bias"].size();
    std::string biasListOne = "";
    for (int i = 0; i < width; i++) {
        float bias = network_weights["1_bias"][i];
        biasListOne += std::to_string(bias);
        if (i + 1 < width) {
            biasListOne += ", ";
        }
    }

    width = network_weights["2_bias"].size();
    std::string biasListTwo = "";
    for (int i = 0; i < width; i++) {
        float bias = network_weights["2_bias"][i];
        biasListTwo += std::to_string(bias);
        if (i + 1 < width) {
            biasListTwo += ", ";
        }
    }

    size_t channelsZero = network_weights["0_weights"].size();
    size_t channelsOne = network_weights["0_bias"].size();
    size_t channelsTwo = network_weights["1_bias"].size();
    size_t channelsThree = network_weights["2_bias"].size();
    int posEncScales = 4;

    std::string fragmentShaderSource = replaceAll(viewDependenceNetworkShaderFunctions, "NUM_CHANNELS_ZERO", std::to_string(channelsZero));
    fragmentShaderSource = replaceAll(fragmentShaderSource, "NUM_POSENC_SCALES", std::to_string(posEncScales));
    fragmentShaderSource = replaceAll(fragmentShaderSource, "NUM_CHANNELS_ONE", std::to_string(channelsOne));
    fragmentShaderSource = replaceAll(fragmentShaderSource, "NUM_CHANNELS_TWO", std::to_string(channelsTwo));
    fragmentShaderSource = replaceAll(fragmentShaderSource, "NUM_CHANNELS_THREE", std::to_string(channelsThree));

    fragmentShaderSource = replaceAll(fragmentShaderSource, "BIAS_LIST_ZERO", biasListZero);
    fragmentShaderSource = replaceAll(fragmentShaderSource, "BIAS_LIST_ONE", biasListOne);
    fragmentShaderSource = replaceAll(fragmentShaderSource, "BIAS_LIST_TWO", biasListTwo);

    return fragmentShaderSource;
}

std::string Raymarch::replaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}
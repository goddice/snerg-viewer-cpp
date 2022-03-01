#pragma once
#include <string>

static float gNearPlane = 0.33f;

const int DISPLAY_NORMAL = 0;
const int DISPLAY_DIFFUSE = 1;
const int DISPLAY_FEATURES = 2;
const int DISPLAY_VIEW_DEPENDENT = 3;
const int DISPLAY_COARSE_GRID = 4;
const int DISPLAY_3D_ATLAS = 5;

static std::string rayMarchVertexShader = R"(  
  #version 330 core

  layout (location = 0) in vec3 position;

  out vec3 vOrigin;
  out vec3 vDirection;
  out vec2 uv;
  
  uniform mat4 modelViewMatrix;
  uniform mat4 projectionMatrix;
  uniform mat4 world_T_clip;

  void main() {
    vec4 positionClip = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
    gl_Position = positionClip;

    positionClip /= positionClip.w;
    vec4 nearPoint = world_T_clip * vec4(positionClip.x, positionClip.y, -1.0, 1.0);
    vec4 farPoint = world_T_clip * vec4(positionClip.x, positionClip.y, 1.0, 1.0);

    vOrigin = nearPoint.xyz / nearPoint.w;
    vDirection = normalize(farPoint.xyz / farPoint.w - vOrigin);
    uv = vec2(position.x/640.0, position.y/360.0);
  }
)";

static std::string rayMarchFragmentShaderHeader = R"(
  #version 330 core
  out vec4 FragColor;

  in vec3 vOrigin;
  in vec3 vDirection;
  in vec2 uv;

  uniform int displayMode;
  uniform int ndc;

  uniform vec3 minPosition;
  uniform vec3 gridSize;
  uniform vec3 atlasSize;
  uniform float voxelSize;
  uniform float blockSize;
  uniform mat3 worldspace_R_opengl;
  uniform float nearPlane;

  uniform float ndc_h;
  uniform float ndc_w;
  uniform float ndc_f;

  uniform sampler3D mapAlpha;
  uniform sampler3D mapColor;
  uniform sampler3D mapFeatures;
  uniform sampler3D mapIndex;

  uniform sampler2D weightsZero;
  uniform sampler2D weightsOne;
  uniform sampler2D weightsTwo;
)";

static std::string viewDependenceNetworkShaderFunctions = R"(
  float indexToPosEnc(vec3 dir, int index) {
    float coordinate =
      (index % 3 == 0) ? dir.x : (
      (index % 3 == 1) ? dir.y : dir.z);
    if (index < 3) {
      return coordinate;
    }
    int scaleExponent = ((index - 3) % (3 * 4)) / 3;
    coordinate *= pow(2.0, float(scaleExponent));
    if ((index - 3) >= 3 * 4) {
      const float kHalfPi = 1.57079632679489661923;
      coordinate += kHalfPi;
    }
    return sin(coordinate);
  }

  vec3 evaluateNetwork(vec3 color, vec4 features, vec3 viewdir) {

    float intermediate_one[NUM_CHANNELS_ONE] = float[](
      BIAS_LIST_ZERO
    );
    for (int j = 0; j < NUM_CHANNELS_ZERO; ++j) {
      float input_value = 0.0;
      if (j < 27) {
        input_value = indexToPosEnc(viewdir, j);
      } else if (j < 30) {
        input_value =
          (j % 3 == 0) ? color.r : (
          (j % 3 == 1) ? color.g : color.b);
      } else {
        input_value =
          (j == 30) ? features.r : (
          (j == 31) ? features.g : (
          (j == 32) ? features.b : features.a));
      }
      if (abs(input_value) < 0.1 / 255.0) {
        continue;
      }
      for (int i = 0; i < NUM_CHANNELS_ONE; ++i) {
        intermediate_one[i] += input_value *
          texelFetch(weightsZero, ivec2(j, i), 0).x;
      }
    }
    
    float intermediate_two[NUM_CHANNELS_TWO] = float[](
      BIAS_LIST_ONE
    );
    for (int j = 0; j < NUM_CHANNELS_ONE; ++j) {
      if (intermediate_one[j] <= 0.0) {
        continue;
      }
      for (int i = 0; i < NUM_CHANNELS_TWO; ++i) {
        intermediate_two[i] += intermediate_one[j] *
          texelFetch(weightsOne, ivec2(j, i), 0).x;
      }
    }

    float result[NUM_CHANNELS_THREE] = float[](
      BIAS_LIST_TWO
    );
    for (int j = 0; j < NUM_CHANNELS_TWO; ++j) {
      if (intermediate_two[j] <= 0.0) {
        continue;
      }
      for (int i = 0; i < NUM_CHANNELS_THREE; ++i) {
        result[i] += intermediate_two[j] *
          texelFetch(weightsTwo, ivec2(j, i), 0).x;
      }
    }
    for (int i = 0; i < NUM_CHANNELS_THREE; ++i) {
      result[i] = 1.0 / (1.0 + exp(-result[i]));
    }

    return vec3(result[0], result[1], result[2]);
  }
)";

static std::string dummyViewDependenceShaderFunctions = R"(
  vec3 evaluateNetwork(
      vec3 color, vec4 features, vec3 viewdir) {
    return vec3(0.0, 0.0, 0.0);
  }
)";

static std::string rayMarchFragmentShaderBody = R"(
  vec3 convertOriginToNDC(vec3 origin, vec3 direction) {
    // We store the NDC scenes flipped, so flip back.
    origin.z *= -1.0;
    direction.z *= -1.0;

    const float near = 1.0;
    float t = -(near + origin.z) / direction.z;
    origin = origin * t + direction;

    // Hardcoded, worked out using approximate iPhone FOV of 67.3 degrees
    // and an image width of 1006 px.
    float focal = ndc_f;
    float W = ndc_w;
    float H = ndc_h;
    float o0 = 1.0 / (W / (2.0 * focal)) * origin.x / origin.z;
    float o1 = -1.0 / (H / (2.0 * focal)) * origin.y / origin.z;
    float o2 = 1.0 + 2.0 * near / origin.z;

    origin = vec3(o0, o1, o2);
    origin.z *= -1.0;
    return origin;
  }

  vec3 convertDirectionToNDC(vec3 origin, vec3 direction) {
    // We store the NDC scenes flipped, so flip back.
    origin.z *= -1.0;
    direction.z *= -1.0;

    const float near = 1.0;
    float t = -(near + origin.z) / direction.z;
    origin = origin * t + direction;

    // Hardcoded, worked out using approximate iPhone FOV of 67.3 degrees
    // and an image width of 1006 px.
    float focal = ndc_f;
    float W = ndc_w;
    float H = ndc_h;

    float d0 = 1.0 / (W / (2.0 * focal)) *
      (direction.x / direction.z - origin.x / origin.z);
    float d1 = -1.0 / (H / (2.0 * focal)) *
      (direction.y / direction.z - origin.y / origin.z);
    float d2 = -2.0 * near / origin.z;

    direction = normalize(vec3(d0, d1, d2));
    direction.z *= -1.0;
    return direction;
  }

  // Compute the atlas block index for a point in the scene using pancake
  // 3D atlas packing.
  vec3 pancakeBlockIndex(
      vec3 posGrid, float blockSize, ivec3 iBlockGridBlocks) {
    ivec3 iBlockIndex = ivec3(floor(posGrid / blockSize));
    ivec3 iAtlasBlocks = ivec3(atlasSize) / ivec3(blockSize + 2.0);
    int linearIndex = iBlockIndex.x + iBlockGridBlocks.x *
      (iBlockIndex.z + iBlockGridBlocks.z * iBlockIndex.y);

    vec3 atlasBlockIndex = vec3(
      float(linearIndex % iAtlasBlocks.x),
      float((linearIndex / iAtlasBlocks.x) % iAtlasBlocks.y),
      float(linearIndex / (iAtlasBlocks.x * iAtlasBlocks.y)));

    // If we exceed the size of the atlas, indicate an empty voxel block.
    if (atlasBlockIndex.z >= float(iAtlasBlocks.z)) {
      atlasBlockIndex = vec3(-1.0, -1.0, -1.0);
    }

    return atlasBlockIndex;
  }

  vec2 rayAabbIntersection(vec3 aabbMin,
                                   vec3 aabbMax,
                                   vec3 origin,
                                   vec3 invDirection) {
    vec3 t1 = (aabbMin - origin) * invDirection;
    vec3 t2 = (aabbMax - origin) * invDirection;
    vec3 tMin = min(t1, t2);
    vec3 tMax = max(t1, t2);
    return vec2(max(tMin.x, max(tMin.y, tMin.z)),
                min(tMax.x, min(tMax.y, tMax.z)));
  }

  void main() {
    float x = (uv.x + 1.0) * 0.5;
    float y = (uv.y + 1.0) * 0.5;
    vec3 texCoord3d = vec3(x, y, 0.37);
    // FragColor = vec4(texture(mapIndex, texCoord3d).rgb, 1.0);
    // return;
    //vec2 texCoord2d = vec2(x,y);

    // See the DisplayMode enum at the top of this file.
    // Runs the full model with view dependence.
    const int DISPLAY_NORMAL = 0;
    // Disables the view-dependence network.
    const int DISPLAY_DIFFUSE = 1;
    // Only shows the latent features.
    const int DISPLAY_FEATURES = 2;
    // Only shows the view dependent component.
    const int DISPLAY_VIEW_DEPENDENT = 3;
    // Only shows the coarse block grid.
    const int DISPLAY_COARSE_GRID = 4;
    // Only shows the 3D texture atlas.
    const int DISPLAY_3D_ATLAS = 5;

    // Set up the ray parameters in world space..
    float nearWorld = nearPlane;
    vec3 originWorld = vOrigin;
    vec3 directionWorld = normalize(vDirection);
    if (ndc != 0) {
      nearWorld = 0.0;
      originWorld = convertOriginToNDC(vOrigin, normalize(vDirection));
      directionWorld = convertDirectionToNDC(vOrigin, normalize(vDirection));
    }

    // Now transform them to the voxel grid coordinate system.
    vec3 originGrid = (originWorld - minPosition) / voxelSize;
    vec3 directionGrid = directionWorld;
    vec3 invDirectionGrid = 1.0 / directionGrid;


    ivec3 iGridSize = ivec3(round(gridSize));
    int iBlockSize = int(round(blockSize));
    ivec3 iBlockGridBlocks = (iGridSize + iBlockSize - 1) / iBlockSize;
    ivec3 iBlockGridSize = iBlockGridBlocks * iBlockSize;
    vec3 blockGridSize = vec3(iBlockGridSize);
    vec2 tMinMax = rayAabbIntersection(
      vec3(0.0, 0.0, 0.0), gridSize, originGrid, invDirectionGrid);


    // Skip any rays that miss the scene bounding box.
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    if (tMinMax.x > tMinMax.y) {
      return;
    }

    float t = max(nearWorld / voxelSize, tMinMax.x) + 0.5;

    vec3 posGrid = originGrid + directionGrid * t;

    vec3 blockMin = floor(posGrid / blockSize) * blockSize;
    vec3 blockMax = blockMin + blockSize;
    vec2 tBlockMinMax = rayAabbIntersection(
          blockMin, blockMax, originGrid, invDirectionGrid);
    vec3 atlasBlockIndex;

    if (displayMode == DISPLAY_3D_ATLAS) {
      atlasBlockIndex = pancakeBlockIndex(posGrid, blockSize, iBlockGridBlocks);
    } else {
      atlasBlockIndex = 255.0 * texture(
        mapIndex, (blockMin + blockMax) / (2.0 * blockGridSize)).rgb;
    }

    float visibility = 1.0;
    vec3 color = vec3(0.0, 0.0, 0.0);
    vec4 features = vec4(0.0, 0.0, 0.0, 0.0);
    int step = 0;
    int maxStep = int(ceil(length(gridSize)));

    while (step < maxStep && t < tMinMax.y && visibility > 1.0 / 255.0) {
      // Skip empty macroblocks.
      if (atlasBlockIndex.x > 254.0) {
        t = 0.5 + tBlockMinMax.y;
      } else { // Otherwise step through them and fetch RGBA and Features.
        vec3 posAtlas = clamp(posGrid - blockMin, 0.0, blockSize);
        posAtlas += atlasBlockIndex * (blockSize + 2.0);
        posAtlas += 1.0; // Account for the one voxel padding in the atlas.

        if (displayMode == DISPLAY_COARSE_GRID) {
          color = atlasBlockIndex * (blockSize + 2.0) / atlasSize;
          features.rgb = atlasBlockIndex * (blockSize + 2.0) / atlasSize;
          features.a = 1.0;
          visibility = 0.0;
          continue;
        }

        // Do a conservative fetch for alpha!=0 at a lower resolution,
        // and skip any voxels which are empty. First, this saves bandwidth
        // since we only fetch one byte instead of 8 (trilinear) and most
        // fetches hit cache due to low res. Second, this is conservative,
        // and accounts for any possible alpha mass that the high resolution
        // trilinear would find.
        const int skipMipLevel = 2;
        const float miniBlockSize = float(1 << skipMipLevel);

        // Only fetch one byte at first, to conserve memory bandwidth in
        // empty space.
        float atlasAlpha = texelFetch(mapAlpha, ivec3(posAtlas / miniBlockSize), skipMipLevel).x;


        if (atlasAlpha > 0.0) {
          // OK, we hit something, do a proper trilinear fetch at high res.
          vec3 atlasUvw = posAtlas / atlasSize;
          atlasAlpha = textureLod(mapAlpha, atlasUvw, 0.0).x;

          // Only worth fetching the content if high res alpha is non-zero.
          if (atlasAlpha > 0.5 / 255.0) {
            vec4 atlasRgba = vec4(0.0, 0.0, 0.0, atlasAlpha);
            atlasRgba.rgb = texture(mapColor, atlasUvw).rgb;
            if (displayMode != DISPLAY_DIFFUSE) {
              vec4 atlasFeatures = texture(mapFeatures, atlasUvw);
              features += visibility * atlasFeatures;
            }
            color += visibility * atlasRgba.rgb;
            visibility *= 1.0 - atlasRgba.a;
          }
        }
        t += 1.0;
      }

      posGrid = originGrid + directionGrid * t;
      if (t > tBlockMinMax.y) {
        blockMin = floor(posGrid / blockSize) * blockSize;
        blockMax = blockMin + blockSize;
        tBlockMinMax = rayAabbIntersection(blockMin, blockMax, originGrid, invDirectionGrid);
        if (displayMode == DISPLAY_3D_ATLAS) {
          atlasBlockIndex = pancakeBlockIndex(posGrid, blockSize, iBlockGridBlocks);
        } else {
          atlasBlockIndex = 255.0 * texture(mapIndex, (blockMin + blockMax) / (2.0 * blockGridSize)).rgb;
        }
      }
      step++;
    }

    if (displayMode == DISPLAY_VIEW_DEPENDENT) {
      color = vec3(0.0, 0.0, 0.0) * visibility;
    } else if (displayMode == DISPLAY_FEATURES) {
      color = features.rgb;
    }

    // Compute the final color, to save compute only compute view-depdence
    // for rays that intersected something in the scene.
    color = vec3(1.0, 1.0, 1.0) * visibility + color;
    const float kVisibilityThreshold = 254.0 / 255.0;
    if (visibility <= kVisibilityThreshold && (displayMode == DISPLAY_NORMAL ||displayMode == DISPLAY_VIEW_DEPENDENT)) {
      color += evaluateNetwork(color, features, worldspace_R_opengl * normalize(vDirection));
    }

    FragColor = vec4(color, 1.0);
}
)";
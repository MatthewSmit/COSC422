//  ========================================================================
//  COSC422: Advanced Computer Graphics;  University of Canterbury (2019)
//  ========================================================================

// TODO: Cache vertex/normals - speedup
// TODO: SceneMin/Max per frame
// TODO: Track movement and move floor plane accordingly
// TODO: Remap properly

#include <GL/freeglut.h>

#include <IL/il.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>

//#include "assimp_extras.h"

#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>

//----------Globals----------------------------
const aiScene* scene = NULL;
const aiScene* animationScene = NULL;
int oldTimeSinceStart;
float angle = 135;
float distance = 5;
std::unordered_map<int, int> texIdMap;
GLuint floorTexture;
bool keyState[256] = {};
bool specialKeyState[GLUT_KEY_INSERT + 1] = {};

//------------Modify the following as needed----------------------
float materialCol[4] = {0.9, 0.9, 0.9, 1}; //Default material colour (not used if model's colour is available)
float shadowColour[4] = {0, 0, 0, 1}; //Default material colour (not used if model's colour is available)
float lightPosn[4] = {2, 10, 5, 0}; //Default light's position
bool twoSidedLight = false; //Change to 'true' to enable two-sided lighting

int tDuration; //Animation duration in ticks.
int currTick = 0; //current tick
int timeStep = 50; //Animation time step = 50 m.sec

bool dwarfSpecial = false;
int currentSceneId = 0;
const int maxSceneId = 3;

#if !defined(_MSC_VER)
#define __debugbreak() asm("int3")
#endif

struct BoneInfo
{
	aiMatrix4x4 offsetMatrix;
	std::vector<aiMatrix4x4> matrix;
	std::vector<aiMatrix4x4> invmatrix;
	int parentIndex;
};

struct ScenePositions
{
    aiVector3D min;
    aiVector3D max;
    aiVector3D center;
    float scale;
};

ScenePositions positions{};
std::vector<BoneInfo> bones{};
std::vector<std::vector<aiMatrix4x4>> animationMatrices{};
std::vector<std::vector<std::vector<std::pair<float, int>>>> vertexWeights{};

//-------------Loads texture files using DevIL library-------------------------------
void loadGLTextures(const std::string& path, const aiScene* scene)
{
	if (scene->HasTextures())
	{
		std::cout << "Support for meshes with embedded textures is not implemented" << std::endl;
		return;
	}

	texIdMap.clear();

	/* scan scene's materials for textures */
	/* Simplified version: Retrieves only the first texture with index 0 if present*/
	for (unsigned int m = 0; m < scene->mNumMaterials; ++m)
	{
		aiString fileName; // filename

		if (scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, 0, &fileName) == AI_SUCCESS)
		{
			ILuint imageId;
			GLuint texId;
			ilGenImages(1, &imageId);
			glGenTextures(1, &texId);
			texIdMap[m] = texId; //store tex ID against material id in a hash map
			ilBindImage(imageId); /* Binding of DevIL image name */
			ilEnable(IL_ORIGIN_SET);
			ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

			std::string realFileName = fileName.data;

			auto lastIndex = realFileName.rfind('/');
			auto realLastIndex = -1;
			if (lastIndex != std::string::npos)
			{
				realLastIndex = lastIndex;
			}

			lastIndex = realFileName.rfind('\\');
			if (lastIndex != std::string::npos)
			{
				realLastIndex = std::max(static_cast<int>(lastIndex), realLastIndex);
			}

			if (realLastIndex >= 0)
			{
				realFileName = realFileName.substr(realLastIndex + 1);
			}

			realFileName = path + realFileName;

			if (ilLoadImage(realFileName.c_str())) //if success
			{
				/* Convert image to RGBA */
				ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

				/* Create and load textures to OpenGL */
				glBindTexture(GL_TEXTURE_2D, texId);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH),
				             ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE,
				             ilGetData());
				// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				std::cout << "Texture:" << realFileName << " successfully loaded." << std::endl;
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
				glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
			}
			else
			{
				std::cout << "Couldn't load Image: " << realFileName << std::endl;
			}
		}
	} //loop for material
}

//-------Loads model data from file and creates a scene object----------
bool loadModel(const std::string& fileName, const std::string& animationFileName)
{
	if (scene)
	{
		aiReleaseImport(scene);
	}
	
	if (animationScene)
	{
		aiReleaseImport(animationScene);
	}

	auto flags = aiProcessPreset_TargetRealtime_MaxQuality;
	if (fileName.compare(fileName.size() - 4, 4, ".bvh") == 0)
	{
		flags |= aiProcess_Debone;
	}
	scene = aiImportFile(fileName.c_str(), flags);
	if (scene == nullptr)
	{
		throw std::exception();
	}

	if (!animationFileName.empty())
	{
		animationScene = aiImportFile(animationFileName.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
		if (animationScene == nullptr)
		{
			throw std::exception();
		}
	}
	else
	{
		animationScene = nullptr;
	}
	
	//printSceneInfo(scene);
	//printMeshInfo(scene);
	//printTreeInfo(scene->mRootNode);
	//printBoneInfo(scene);
	//printAnimInfo(scene);  //WARNING:  This may generate a lengthy output if the model has animation data
	loadGLTextures(fileName.substr(0, fileName.rfind('/') + 1), scene);

	if (animationScene)
	{
		if (animationScene->mAnimations)
		{
			tDuration = animationScene->mAnimations[0]->mDuration;
		}
		else
		{
			tDuration = 0;
		}
	}
	else
	{
		if (scene->mAnimations)
		{
			tDuration = scene->mAnimations[0]->mDuration;
		}
		else
		{
			tDuration = 0;
		}
	}
	return true;
}

aiMatrix4x4 FindMatrix(std::unordered_map<std::string, int>& boneMapping, aiNode* node, int tick)
{
	const auto mapping = boneMapping.find(node->mName.C_Str());
	aiMatrix4x4 transformation;
	if (mapping == boneMapping.end())
	{
		transformation = node->mTransformation;
	}
	else
	{
		transformation = animationMatrices.at(mapping->second).at(tick);
	}
	
	if (node->mParent == NULL)
	{
		return transformation;
	}

	return FindMatrix(boneMapping, node->mParent, tick) * transformation;
}

// ------A recursive function to traverse scene graph and render each mesh----------
void render(const aiScene* sc, bool shadow)
{
	for (auto j = 0u; j < sc->mNumMeshes; j++)
	{
        const auto& meshWeights = vertexWeights.at(j);
		auto mesh = scene->mMeshes[j];
		
		if (mesh->HasTextureCoords(0) && !shadow)
		{
			glEnable(GL_TEXTURE_2D);
		}
		else
		{
			glDisable(GL_TEXTURE_2D);
		}

		aiColor4D diffuse;
		int materialIndex = mesh->mMaterialIndex; //Get material index attached to the mesh
		auto mtl = sc->mMaterials[materialIndex];
		if (shadow)
		{
			glColor4fv(shadowColour); //User-defined colour
		}
		else if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
		{
			//Get material colour from model
			glColor4f(diffuse.r, diffuse.g, diffuse.b, 1.0);
		}
		else
		{
			glColor4fv(materialCol); //Default material colour
		}
		
		if (mesh->HasTextureCoords(0) && !shadow)
		{
			glBindTexture(GL_TEXTURE_2D, texIdMap[materialIndex]);
		}

		//Get the polygons from each mesh and draw them
		for (auto k = 0u; k < mesh->mNumFaces; k++)
		{
			const auto face = &mesh->mFaces[k];
			GLenum face_mode;
			
			switch (face->mNumIndices)
			{
			case 1: face_mode = GL_POINTS;
				break;
			case 2: face_mode = GL_LINES;
				break;
			case 3: face_mode = GL_TRIANGLES;
				break;
			default: face_mode = GL_POLYGON;
				break;
			}
			
			glBegin(face_mode);
			
			for (auto i = 0u; i < face->mNumIndices; i++)
			{
				const int vertexIndex = face->mIndices[i];
				const auto& vertexWeight = meshWeights.at(vertexIndex);
				
				if (shadow)
				{
				}
				else if (mesh->HasTextureCoords(0))
				{
					glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x, mesh->mTextureCoords[0][vertexIndex].y);
				}
				else if (mesh->HasVertexColors(0))
				{
					glColor4fv(reinterpret_cast<GLfloat*>(&mesh->mColors[0][vertexIndex]));
				}
				
				if (mesh->HasNormals())
				{
                    aiVector3D normal = aiVector3D();
                    for (auto weight : vertexWeight)
                    {
                        normal += bones.at(weight.second).invmatrix.at(currTick) * mesh->mNormals[vertexIndex] * weight.first;
                    }
					glNormal3fv(&normal.x);
				}

                aiVector3D vertex = aiVector3D();
                for (auto weight : vertexWeight)
                {
                    vertex += bones.at(weight.second).matrix.at(currTick) * mesh->mVertices[vertexIndex] * weight.first;
                }
				glVertex3fv(&vertex.x);
			}
			
			glEnd();
		}
	}
}

aiMatrix4x4 GetKeyframe(aiNodeAnim* pAnim, int j)
{
	aiVector3D position;
	auto foundPosition = false;
	if (pAnim->mNumPositionKeys == 1)
	{
		position = pAnim->mPositionKeys[0].mValue;
		foundPosition = true;
	}
	else
	{
		for (auto i = 0u; i < pAnim->mNumPositionKeys; i++)
		{
			if (pAnim->mPositionKeys[i].mTime == j)
			{
				position = pAnim->mPositionKeys[i].mValue;
				foundPosition = true;
				break;
			}

			if (i > 0 && pAnim->mPositionKeys[i - 1].mTime < j && pAnim->mPositionKeys[i].mTime >= j)
			{
				const auto previous = pAnim->mPositionKeys[i - 1].mValue;
				const auto current = pAnim->mPositionKeys[i].mValue;
				const auto delta = (j - pAnim->mPositionKeys[i - 1].mTime) / (pAnim->mPositionKeys[i].mTime - pAnim->mPositionKeys[i - 1].mTime);
				position = (current - previous).SymMul(aiVector3D(delta)) + previous;
				foundPosition = true;
				break;
			}
		}
	}

	if (!foundPosition)
	{
		__debugbreak();
		throw std::exception{};
	}

	aiQuaternion rotation;
	auto foundRotation = false;
	if (pAnim->mNumRotationKeys == 1)
	{
		rotation = pAnim->mRotationKeys[0].mValue;
		foundRotation = true;
	}
	else
	{
		for (auto i = 0u; i < pAnim->mNumRotationKeys; i++)
		{
			if (pAnim->mRotationKeys[i].mTime == j)
			{
				rotation = pAnim->mRotationKeys[i].mValue;
				foundRotation = true;
				break;
			}

			if (i > 0 && pAnim->mRotationKeys[i - 1].mTime < j && pAnim->mRotationKeys[i].mTime >= j)
			{
				const auto previous = pAnim->mRotationKeys[i - 1].mValue;
				const auto current = pAnim->mRotationKeys[i].mValue;
				const auto delta = (j - pAnim->mRotationKeys[i - 1].mTime) / (pAnim->mRotationKeys[i].mTime - pAnim->mRotationKeys[i - 1].mTime);
				aiQuaternion::Interpolate(rotation, previous, current, delta);
				foundRotation = true;
				break;
			}
		}
	}

	if (!foundRotation)
	{
		__debugbreak();
		throw std::exception{};
	}

	// TODO: Scale?

	aiMatrix4x4 positionMatrix;
	aiMatrix4x4::Translation(position, positionMatrix);

	const auto rotationMatrix = aiMatrix4x4(rotation.GetMatrix());

	return positionMatrix * rotationMatrix;
}

void FindBones(const aiNode* node, std::unordered_map<std::string, int>& boneMapping)
{
	if (boneMapping.find(node->mName.C_Str()) == boneMapping.end())
	{
		boneMapping.insert(std::make_pair(node->mName.C_Str(), bones.size()));
		bones.push_back({aiMatrix4x4(), {}, {}, -1});
	}
	
	for (auto i = 0u; i < node->mNumChildren; i++)
	{
		FindBones(node->mChildren[i], boneMapping);
	}
}

void FindBones(const aiScene* scene, std::unordered_map<std::string, int>& boneMapping)
{
	for (auto i = 0u; i < scene->mNumMeshes; i++)
	{
		for (auto j = 0u; j < scene->mMeshes[i]->mNumBones; j++)
		{
			const auto& bone = scene->mMeshes[i]->mBones[j];
			if (boneMapping.find(bone->mName.C_Str()) == boneMapping.end())
			{
				boneMapping.insert(std::make_pair(bone->mName.C_Str(), bones.size()));
				bones.push_back({bone->mOffsetMatrix, {}, {}, -1});
			}
		}
	}

	FindBones(scene->mRootNode, boneMapping);

	for (const auto& mapping : boneMapping)
	{
		const auto node = scene->mRootNode->FindNode(mapping.first.data());
		auto parent = node->mParent;
		while (parent)
		{
			auto parentIndex = boneMapping.find(parent->mName.C_Str());
			if (parentIndex == boneMapping.end())
			{
				parent = parent->mParent;
			}
			else
			{
				bones.at(mapping.second).parentIndex = parentIndex->second;
				break;
			}
		}
	}
}

void UpdateAnimationMatrices()
{
	bones.clear();
	animationMatrices.clear();
	vertexWeights.clear();

	const auto sourceScene = animationScene ? animationScene : scene;

	if (!sourceScene->mAnimations)
	{
		throw std::exception{};
	}

	const auto anim = sourceScene->mAnimations[0];

	if (round(anim->mDuration) != anim->mDuration)
	{
		throw std::exception{};
	}

	auto boneMapping = std::unordered_map<std::string, int>();
	FindBones(scene, boneMapping);

	vertexWeights.resize(scene->mNumMeshes);
	for (auto i = 0u; i < scene->mNumMeshes; i++)
	{
		auto& meshVertexWeights = vertexWeights.at(i);
		meshVertexWeights.resize(scene->mMeshes[i]->mNumVertices);
		
		for (auto j = 0u; j < scene->mMeshes[i]->mNumBones; j++)
		{
			const auto& bone = scene->mMeshes[i]->mBones[j];
			auto boneIndex = boneMapping.find(bone->mName.C_Str())->second;
			for (auto k = 0u; k < bone->mNumWeights; k++)
			{
				const auto& weight = bone->mWeights[k];
				meshVertexWeights.at(weight.mVertexId).push_back(std::make_pair(weight.mWeight, boneIndex));
			}
		}
	}

	animationMatrices.resize(boneMapping.size());
	for (const auto& mapping : boneMapping)
	{
		const auto node = scene->mRootNode->FindNode(mapping.first.data());
		animationMatrices.at(mapping.second) = std::vector<aiMatrix4x4>(static_cast<int>(anim->mDuration), node->mTransformation);
	}
	
	for (auto i = 0u; i < anim->mNumChannels; i++)
	{
		const auto ndAnim = anim->mChannels[i];
		// const auto node = scene->mRootNode->FindNode(ndAnim->mNodeName);
		const auto mapping = boneMapping.find(ndAnim->mNodeName.C_Str());
		if (mapping == boneMapping.end())
		{
			__debugbreak();
		}

		for (auto j = 0; j < anim->mDuration; j++)
		{
			animationMatrices.at(mapping->second).at(j) = GetKeyframe(anim->mChannels[i], j);
		}
	}

	for (const auto& mapping : boneMapping)
	{
		const auto node = scene->mRootNode->FindNode(mapping.first.data());
		bones.at(mapping.second).matrix.resize(static_cast<int>(anim->mDuration));
		bones.at(mapping.second).invmatrix.resize(static_cast<int>(anim->mDuration));
		for (auto i = 0; i < static_cast<int>(anim->mDuration); i++)
		{
			bones.at(mapping.second).matrix.at(i) = FindMatrix(boneMapping, node, i) * bones.at(mapping.second).offsetMatrix;
			bones.at(mapping.second).invmatrix.at(i) = bones.at(mapping.second).matrix.at(i);
            bones.at(mapping.second).invmatrix.at(i).Transpose().Inverse();
		}
	}
}

void get_bounding_box()
{
    positions.min = aiVector3D(+1e10f);
    positions.max = aiVector3D(-1e10f);

    for (auto n = 0u; n < scene->mNumMeshes; ++n)
    {
        const aiMesh* mesh = scene->mMeshes[n];
        const auto& meshWeights = vertexWeights.at(n);

        for (auto k = 0u; k < mesh->mNumFaces; k++) {
            const auto face = &mesh->mFaces[k];
            for (auto i = 0u; i < face->mNumIndices; i++) {
                const int vertexIndex = face->mIndices[i];
                const auto &vertexWeight = meshWeights.at(vertexIndex);
                aiVector3D tmp = aiVector3D();
                for (auto weight : vertexWeight)
                {
                    tmp += bones.at(weight.second).matrix.at(currTick) * mesh->mVertices[vertexIndex] * weight.first;
                }

                positions.min.x = std::min(positions.min.x, tmp.x);
                positions.min.y = std::min(positions.min.y, tmp.y);
                positions.min.z = std::min(positions.min.z, tmp.z);

                positions.max.x = std::max(positions.max.x, tmp.x);
                positions.max.y = std::max(positions.max.y, tmp.y);
                positions.max.z = std::max(positions.max.z, tmp.z);
            }
        }
    }

	auto tmp = positions.max.x - positions.min.x;
	tmp = std::max(positions.max.y - positions.min.y, tmp);
	tmp = std::max(positions.max.z - positions.min.z, tmp);
	positions.scale = 1.0f / tmp;

    positions.center = ((positions.max - positions.min) * 0.5f + positions.min) * positions.scale;
}

void loadScene(int newSceneId)
{
    currTick = 0;
	currentSceneId = newSceneId;
	if (currentSceneId < 0)
	{
		currentSceneId = maxSceneId - 1;
	}
	if (currentSceneId >= maxSceneId)
	{
		currentSceneId = 0;
	}

	switch (currentSceneId)
	{
	case 0:
		loadModel("data2/ArmyPilot/ArmyPilot.x", "");
		break;
	case 1:
		loadModel("data2/Mannequin/mannequin.fbx", "data2/Mannequin/run.fbx");
		break;
	case 2:
		if (dwarfSpecial)
		{
			loadModel("data2/Dwarf/dwarf.x", "data2/Dwarf/avatar_walk.bvh");
		}
		else
		{
			loadModel("data2/Dwarf/dwarf.x", "");
		}
		break;

	default:
		throw std::exception{};
	}

	UpdateAnimationMatrices();

    get_bounding_box();
}

void initialise()
{
	float ambient[4] = {0.2, 0.2, 0.2, 1.0}; //Ambient light
	float white[4] = {1, 1, 1, 1}; //Light's colour
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white);
	if (twoSidedLight)
	{
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
	}

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50);
	glColor4fv(materialCol);
	
	loadScene(currentSceneId);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(35, 800.0f / 600.0f, 1.0, 1000.0);

	uint32_t colour[]
	{
		0xFF00FFFF,
		0xFFFFFF00,
		0xFFFFFF00,
		0xFF00FFFF,
	};

	glGenTextures(1, &floorTexture);
	glBindTexture(GL_TEXTURE_2D, floorTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, colour);
}

void update(int)
{
	const auto timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	const auto deltaTime = timeSinceStart - oldTimeSinceStart;
	oldTimeSinceStart = timeSinceStart;

	const auto delta = deltaTime * 0.001f;
	
	if (specialKeyState[GLUT_KEY_LEFT])
	{
		angle -= delta * 100;
		if (angle < 0)
		{
			angle += 360;
		}
	}

	if (specialKeyState[GLUT_KEY_RIGHT])
	{
		angle += delta * 100;
		if (angle > 0)
		{
			angle -= 360;
		}
	}

	if (specialKeyState[GLUT_KEY_UP])
	{
		distance -= delta * 2;
		if (distance < 1.5)
		{
			distance = 1.5;
		}
	}

	if (specialKeyState[GLUT_KEY_DOWN])
	{
		distance += delta * 2;
		if (distance > 100)
		{
			distance = 100;
		}
	}

	currTick++;
	if (currTick >= tDuration)
	{
		currTick = 0;
	}
	
	glutPostRedisplay();
	glutTimerFunc(timeStep, update, 0);
}

void keyboardCallback(unsigned char key, int x, int y)
{
	if (key == '1' && currentSceneId == 2)
	{
		dwarfSpecial = false;
		loadScene(2);
	}
	if (key == '2' && currentSceneId == 2)
	{
		dwarfSpecial = true;
		loadScene(2);
	}

	if (key == '-')
	{
		dwarfSpecial = false;
		loadScene(currentSceneId - 1);
	}
	if (key == '=')
	{
		dwarfSpecial = false;
		loadScene(currentSceneId + 1);
	}

	keyState[key] = true;
}

void keyboardUpCallback(unsigned char key, int, int)
{
	keyState[key] = false;
}

void specialCallback(int key, int, int)
{
	if (key >= 0 && key <= GLUT_KEY_INSERT)
	{
		specialKeyState[key] = true;
	}
}

void specialUpCallback(int key, int, int) {
	if (key >= 0 && key <= GLUT_KEY_INSERT)
	{
		specialKeyState[key] = false;
	}
}

void drawPlane()
{
	glColor4f(1, 1, 1, 1);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, floorTexture);
	
	glBegin(GL_QUADS);

	glTexCoord2f(0, 0);
	glVertex3f(-100, 0, -100);
	
	glTexCoord2f(0, 100);
	glVertex3f(-100, 0, 100);
	
	glTexCoord2f(100, 100);
	glVertex3f(100, 0, 100);
	
	glTexCoord2f(100, 0);
	glVertex3f(100, 0, -100);
	
	glEnd();
	glEnable(GL_LIGHTING);
}

void normalise(const float input[4], float output[4])
{
	const auto size = std::sqrt(input[0] * input[0] +
		input[1] * input[1] +
		input[2] * input[2] +
		input[3] * input[3]);
	output[0] = input[0] / size;
	output[1] = input[1] / size;
	output[2] = input[2] / size;
	output[3] = input[3] / size;
}

static void createShadowMatrix(const float ground[4], const float xlight[4])
{
	float shadowMat[4][4]
	{
	};

	float light[4];
	normalise(xlight, light);

	const auto dot = ground[0] * light[0] +
		ground[1] * light[1] +
		ground[2] * light[2] +
		ground[3] * light[3];

	shadowMat[0][0] = dot - light[0] * ground[0];
	shadowMat[1][0] = 0.0 - light[0] * ground[1];
	shadowMat[2][0] = 0.0 - light[0] * ground[2];
	shadowMat[3][0] = 0.0 - light[0] * ground[3];

	shadowMat[0][1] = 0.0 - light[1] * ground[0];
	shadowMat[1][1] = dot - light[1] * ground[1];
	shadowMat[2][1] = 0.0 - light[1] * ground[2];
	shadowMat[3][1] = 0.0 - light[1] * ground[3];

	shadowMat[0][2] = 0.0 - light[2] * ground[0];
	shadowMat[1][2] = 0.0 - light[2] * ground[1];
	shadowMat[2][2] = dot - light[2] * ground[2];
	shadowMat[3][2] = 0.0 - light[2] * ground[3];

	shadowMat[0][3] = 0.0 - light[3] * ground[0];
	shadowMat[1][3] = 0.0 - light[3] * ground[1];
	shadowMat[2][3] = 0.0 - light[3] * ground[2];
	shadowMat[3][3] = dot - light[3] * ground[3];

	glMultMatrixf(reinterpret_cast<const GLfloat*>(shadowMat));
}

void renderScene(bool shadow)
{
	if (shadow)
	{
		glDisable(GL_LIGHTING);
	}
	else
	{
		glEnable(GL_LIGHTING);
	}
	glPushMatrix();

	glTranslatef(0, -positions.min.y * positions.scale, 0);

	// scale the whole asset to fit into our view frustum
	glScalef(positions.scale, positions.scale, positions.scale);

//	const float xc = (scene_min.x + scene_max.x) * 0.5;
//	const float yc = (scene_min.y + scene_max.y) * 0.5;
//	const float zc = (scene_min.z + scene_max.z) * 0.5;
	// center the model
//	glTranslatef(-xc, -yc, -zc);
	glTranslatef(-positions.center.x, -positions.center.y, -positions.center.z);

	render(scene, shadow);
	glPopMatrix();
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(sin(AI_DEG_TO_RAD(angle)) * distance,
              positions.center.y + distance / 2,
	          cos(AI_DEG_TO_RAD(angle)) * distance,
              positions.center.x,
              positions.center.y,
              positions.center.z,
	          0, 1, 0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosn);

	drawPlane();
	renderScene(false);

	glTranslatef(0, 0.0001, 0);
	glScalef(1, 0, 1);

	float ground[4]{0, 1, 0, 0};
	createShadowMatrix(ground, lightPosn);
	renderScene(true);

	glutSwapBuffers();
}

int main(int argc, char** argv)
{
	/* initialization of DevIL */
	ilInit();

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutInitContextVersion(4, 2);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
	glutSetKeyRepeat(false);
	glutCreateWindow("COSC 422 Assignment 2 - MJS351 - Animation");

	initialise();
	glutDisplayFunc(display);
	glutTimerFunc(50, update, 0);
	glutKeyboardFunc(keyboardCallback);
	glutKeyboardUpFunc(keyboardUpCallback);
	glutSpecialFunc(specialCallback);
	glutSpecialUpFunc(specialUpCallback);
	glutMainLoop();

	aiReleaseImport(scene);
}


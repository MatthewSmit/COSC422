//  ========================================================================
//  COSC422: Advanced Computer Graphics;  University of Canterbury (2019)
//  ========================================================================

#include <iostream>
#include <map>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <IL/il.h>

#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "assimp_extras.h"

#include <unordered_map>
#include <vector>

#ifndef NDEBUG
void GLAPIENTRY debugCallback(GLenum source,
                              GLenum type,
                              GLuint id,
                              GLenum severity,
                              GLsizei,
                              const char* message,
                              const void*)
{
	static const char* SOURCE[] = {
		"API",
		"Window System",
		"Shader Compiler",
		"Thirdy Party",
		"Application",
		"Other"
	};

	static const char* TYPE[] = {
		"Error",
		"Deprecated Behaviour",
		"Undefined Behaviour",
		"Portability",
		"Performance",
		"Other"
	};

	static const char* SEVERITY[] = {
		"High",
		"Medium",
		"Low"
	};

	printf("Source: %s; Type: %s; Id: %d; Severity: %s, Message: %s\n",
	       SOURCE[source - GL_DEBUG_SOURCE_API],
	       TYPE[type - GL_DEBUG_TYPE_ERROR],
	       id,
	       severity == GL_DEBUG_SEVERITY_NOTIFICATION ? "Notification" : SEVERITY[severity - GL_DEBUG_SEVERITY_HIGH],
	       message);
}
#endif

//----------Globals----------------------------
const aiScene* scene = NULL;
const aiScene* animationScene = NULL;
int oldTimeSinceStart;
float angle = 0;
float distance = 5;
aiVector3D scene_min;
aiVector3D scene_max;
aiVector3D scene_center;
int modelRotn = 0;
std::map<int, int> texIdMap;
GLuint floorTexture;
bool keyState[256] = {};
bool specialKeyState[GLUT_KEY_INSERT + 1] = {};

//------------Modify the following as needed----------------------
float materialCol[4] = {0.9, 0.9, 0.9, 1}; //Default material colour (not used if model's colour is available)
float shadowColour[4] = {0, 0, 0, 1}; //Default material colour (not used if model's colour is available)
float lightPosn[4] = {2, 10, 5, 0}; //Default light's position
bool twoSidedLight = false; //Change to 'true' to enable two-sided lighting

int tDuration; //Animation duration in ticks.
int currTick = 1; //current tick
int timeStep = 50; //Animation time step = 50 m.sec

bool dwarfSpecial = false;
int currentSceneId = 0;
const int maxSceneId = 3;

std::unordered_map<std::string, std::vector<aiMatrix4x4>> animationMatrices;

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
		if (scene->mAnimations)
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

// ------A recursive function to traverse scene graph and render each mesh----------
void render(const aiScene* sc, const aiNode* nd, bool shadow)
{
	auto m = nd->mTransformation;

	aiColor4D diffuse;

	aiTransposeMatrix4(&m); //Convert to column-major order
	glPushMatrix();
	glMultMatrixf(reinterpret_cast<float*>(&m)); //Multiply by the transformation matrix for this node

	// Draw all meshes assigned to this node
	for (auto n = 0; n < nd->mNumMeshes; n++)
	{
		const int meshIndex = nd->mMeshes[n]; //Get the mesh indices from the current node
		auto mesh = scene->mMeshes[meshIndex]; //Using mesh index, get the mesh object

		if (mesh->HasTextureCoords(0) && !shadow)
		{
			glEnable(GL_TEXTURE_2D);
		}
		else
		{
			glDisable(GL_TEXTURE_2D);
		}
		
		int materialIndex = mesh->mMaterialIndex; //Get material index attached to the mesh
		auto mtl = sc->mMaterials[materialIndex];
		if (shadow)
			glColor4fv(shadowColour);   //User-defined colour
		else if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))  //Get material colour from model
			glColor4f(diffuse.r, diffuse.g, diffuse.b, 1.0);
		else
			glColor4fv(materialCol);   //Default material colour

		if (mesh->HasTextureCoords(0) && !shadow)
		{
			glBindTexture(GL_TEXTURE_2D, texIdMap[materialIndex]);
		}
        
		//Get the polygons from each mesh and draw them
		for (auto k = 0; k < mesh->mNumFaces; k++)
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

			for (auto i = 0; i < face->mNumIndices; i++)
			{
				const int vertexIndex = face->mIndices[i];

				if (shadow)
				{
				}
				else if (mesh->HasTextureCoords(0))
				{
					glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x, mesh->mTextureCoords[0][vertexIndex].y);
				}
				else if(mesh->HasVertexColors(0))
					glColor4fv(reinterpret_cast<GLfloat*>(&mesh->mColors[0][vertexIndex]));

				//Assign texture coordinates here

				if (mesh->HasNormals())
					glNormal3fv(&mesh->mNormals[vertexIndex].x);

				glVertex3fv(&mesh->mVertices[vertexIndex].x);
			}

			glEnd();
		}
	}

	// Draw all children
	for (int i = 0; i < nd->mNumChildren; i++)
		render(sc, nd->mChildren[i], shadow);

	glPopMatrix();
}

aiMatrix4x4 GetKeyframe(aiNodeAnim *pAnim, int j) {
    aiVector3D position;
    auto foundPosition = false;
    if (pAnim->mNumPositionKeys == 1) {
        position = pAnim->mPositionKeys[0].mValue;
        foundPosition = true;
    } else {
        for (auto i = 0u; i < pAnim->mNumPositionKeys; i++) {
            if (pAnim->mPositionKeys[i].mTime == j) {
                position = pAnim->mPositionKeys[i].mValue;
                foundPosition = true;
                break;
            } else if (i > 0 && pAnim->mPositionKeys[i - 1].mTime < j && pAnim->mPositionKeys[i].mTime >= j) {
                auto previous = pAnim->mPositionKeys[i - 1].mValue;
                auto current = pAnim->mPositionKeys[i].mValue;
                auto delta = (j - pAnim->mPositionKeys[i - 1].mTime) / (pAnim->mPositionKeys[i].mTime - pAnim->mPositionKeys[i - 1].mTime);
                position = (current - previous).SymMul(aiVector3D(delta)) + previous;
                foundPosition = true;
                break;
            }
        }
    }

    if (!foundPosition) {
        asm("int3");
        throw std::exception{};
    }

    aiQuaternion rotation;
    auto foundRotation = false;
    if (pAnim->mNumRotationKeys == 1) {
        rotation = pAnim->mRotationKeys[0].mValue;
        foundRotation = true;
    } else {
        for (auto i = 0u; i < pAnim->mNumRotationKeys; i++) {
            if (pAnim->mRotationKeys[i].mTime == j) {
                rotation = pAnim->mRotationKeys[i].mValue;
                foundRotation = true;
                break;
            } else if (i > 0 && pAnim->mRotationKeys[i - 1].mTime < j && pAnim->mRotationKeys[i].mTime >= j) {
                auto previous = pAnim->mRotationKeys[i - 1].mValue;
                auto current = pAnim->mRotationKeys[i].mValue;
                auto delta = (j - pAnim->mRotationKeys[i - 1].mTime) / (pAnim->mRotationKeys[i].mTime - pAnim->mRotationKeys[i - 1].mTime);
                aiQuaternion::Interpolate(rotation, previous, current, delta);
                foundRotation = true;
                break;
            }
        }
    }

    if (!foundRotation) {
        asm("int3");
        throw std::exception{};
    }

    // TODO: Scale?

    aiMatrix4x4 positionMatrix;
    aiMatrix4x4::Translation(position, positionMatrix);

    aiMatrix4x4 rotationMatrix = aiMatrix4x4(rotation.GetMatrix());

    return positionMatrix * rotationMatrix;
}

void UpdateAnimationMatrices()
{
    animationMatrices.clear();

    auto sourceScene = animationScene ? animationScene : scene;

    if (!sourceScene->mAnimations)
    {
        throw std::exception{};
    }

    const auto anim = sourceScene->mAnimations[0];

    if (round(anim->mDuration) != anim->mDuration)
    {
        throw std::exception{};
    }

    for (auto i = 0u; i < anim->mNumChannels; i++)
    {
        const auto ndAnim = anim->mChannels[i]; //Channel
        animationMatrices.insert(std::make_pair(ndAnim->mNodeName.C_Str(), std::vector<aiMatrix4x4>((int)anim->mDuration)));
        auto& vector = animationMatrices.at(ndAnim->mNodeName.C_Str());

        for (auto j = 0; j < anim->mDuration; j++)
        {
            vector[j] = GetKeyframe(anim->mChannels[i], j);
        }
    }
}

void loadScene(int newSceneId)
{
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
            modelRotn = -1;

//        loadModel("data2/Walk.bvh", "");
//            modelRotn = 1;
		break;
	case 1:
		loadModel("data2/Mannequin/mannequin.fbx", "data2/Mannequin/run.fbx");
		modelRotn = 0;
		break;
	case 2:
		if (dwarfSpecial)
		{
			loadModel("data2/Dwarf/dwarf.x", "data2/Walk.bvh");
		}
		else
		{
			loadModel("data2/Dwarf/dwarf.x", "");
		}
		modelRotn = 0;
		break;

		default:
	        throw std::exception{};
	}

    currTick = 1;

	UpdateAnimationMatrices();

	// printAnimInfo(scene);
}
//--------------------OpenGL initialization------------------------
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

//void updateNodeMatrices(int tick)
//{
//	auto sourceScene = animationScene ? animationScene : scene;
//
//	if (!sourceScene->mAnimations)
//	{
//		return;
//	}
//
//	const auto anim = sourceScene->mAnimations[0];
//
//	for (auto i = 0; i < anim->mNumChannels; i++)
//	{
//		auto matPos = aiMatrix4x4(); //Identity
//		auto matRot = aiMatrix4x4();
//		const auto ndAnim = anim->mChannels[i]; //Channel
//
//		auto index = std::min(std::max(tick, 0), static_cast<int>(ndAnim->mNumPositionKeys) - 1);
//
//		auto posn = ndAnim->mPositionKeys[index].mValue;
//		aiMatrix4x4::Translation(posn, matPos);
//
//		index = std::min(std::max(tick, 0), static_cast<int>(ndAnim->mNumRotationKeys) - 1);
//
//		auto rotn = ndAnim->mRotationKeys[index].mValue;
//		auto matRot3 = rotn.GetMatrix();
//		matRot = aiMatrix4x4(matRot3);
//		const auto matProd = matPos * matRot;
//
//		auto nd = scene->mRootNode->FindNode(ndAnim->mNodeName);
//		if (nd)
//		{
//			nd->mTransformation = matProd;
//		}
//	}
//}

//----Timer callback for continuous rotation of the model about y-axis----
void update(int)
{
	int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	int deltaTime = timeSinceStart - oldTimeSinceStart;
	oldTimeSinceStart = timeSinceStart;

	float delta = deltaTime * 0.001f;
	
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

	if (currTick >= tDuration)
	{
		currTick = 0;
	}
//	updateNodeMatrices(currTick);
	get_bounding_box(scene, &scene_min, &scene_max);
	scene_center = (scene_max - scene_min) * 0.5f + scene_min;
	scene_center = scene_center.Normalize();
	currTick++;
	
	glutPostRedisplay();
	glutTimerFunc(timeStep, update, 0);
}

//----Keyboard callback to toggle initial model orientation---
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

void keyboardUpCallback(unsigned char key, int, int) {
	keyState[key] = false;
}

void specialCallback(int key, int, int) {
	if (key >= 0 && key <= GLUT_KEY_INSERT) {
		specialKeyState[key] = true;
	}
}

void specialUpCallback(int key, int, int) {
	if (key >= 0 && key <= GLUT_KEY_INSERT) {
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
	const auto size = sqrt(input[0] * input[0] +
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

	glTranslatef(0, 0.5, 0);
	
	// glRotatef(angle, 0.f, 1.f, 0.0f); //Continuous rotation about the y-axis
	if (modelRotn == -1)
	{
		glRotatef(90, 1, 0, 0); //First, rotate the model about x-axis if needed.
	}
	if (modelRotn == 1)
	{
		glRotatef(-90, 1, 0, 0); //First, rotate the model about x-axis if needed.
	}

	// scale the whole asset to fit into our view frustum
	auto tmp = scene_max.x - scene_min.x;
	tmp = aisgl_max(scene_max.y - scene_min.y, tmp);
	tmp = aisgl_max(scene_max.z - scene_min.z, tmp);
	tmp = 1.0f / tmp;
	glScalef(tmp, tmp, tmp);

	const float xc = (scene_min.x + scene_max.x) * 0.5;
	const float yc = (scene_min.y + scene_max.y) * 0.5;
	const float zc = (scene_min.z + scene_max.z) * 0.5;
	// center the model
	glTranslatef(-xc, -yc, -zc);

	render(scene, scene->mRootNode, shadow);
	glPopMatrix();
}

//------The main display function---------
//----The model is first drawn using a display list so that all GL commands are
//    stored for subsequent display updates.
void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(sin(AI_DEG_TO_RAD(angle)) * distance, 
	          scene_center.y + distance / 2,
	          cos(AI_DEG_TO_RAD(angle)) * distance, 
	          scene_center.x, 
	          scene_center.y, 
	          scene_center.z, 
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
#ifndef NDEBUG
	glutInitContextFlags(GLUT_DEBUG);
#endif
	glutCreateWindow("COSC 422 Assignment 2 - MJS351 - Animation");

#ifndef NDEBUG
	if (glewInit() == GLEW_OK)
	{
		std::cout << "GLEW initialization successful! " << std::endl;
		std::cout << " Using GLEW version " << glewGetString(GLEW_VERSION) << std::endl;
	}
	else
	{
		std::cerr << "Unable to initialize GLEW  ...exiting." << std::endl;
		exit(EXIT_FAILURE);
	}
	glDebugMessageCallback(debugCallback, nullptr);
#endif

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


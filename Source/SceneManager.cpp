///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
 /***********************************************************

  /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	/***************************/
	// loads NAPKIN texture into project from final_project folder //
	// assigns PPaper texture to paper texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/PPAPER.jpg",
		"Paper");
	/***************************/
		/***************************/
	// loads wood texture into project from final_project folder //
	// assigns wood006 texture to wood texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/WoodTab.jpg",
		"Table");
	/***************************/
// loads Metal stainless texture into project from final_project folder //
// assigns Metal stainless texture to Metal_S texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Metalstainless.jpg",
		"Metal_S");
	/***************************/
	/***************************/
// loads plastic texture into project from final_project folder //
// assigns plastic texture to Plastic_P texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/PlasticGray.jpg",
		"Plastic_P");
	/***************************/
	/***************************/
// loads dark Metal texture into project from final_project folder //
// assigns dark Metal texture to Metal_T texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Metal_T.jpg",
		"Metal_T");
	/***************************/
	/***************************/
// loads everything bagel texture into project from final_project folder //
// assigns everything texture to Bagel_B texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Bagel01.jpg",
		"Bagel_B");
	/***************************/
// loads candle texture into project from final_project folder //
// assigns Candle to Candle_C texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Candle.jpg",
		"Candle_C");
	/***************************/
// loads candle light texture into project from final_project folder //
// assigns Candle light to Candle_L texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Candle_L.jpg",
		"Candle_L");
	/***************************/
// loads Mug texture into project from final_project folder //
// assigns Mug texture to Mug_M texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Mug_M.jpg",
		"Mug_M");
	/***************************/
// loads light blue texture into project from final_project folder //
// assigns light blue texture to Lblue_B texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/Lblue_B.jpg",
		"Lblue_B");
	/***************************/
// loads white lid texture into project from final_project folder //
// assigns white lid texture to White_Lid texture slot //

	bReturn = CreateGLTexture(
		"C:/Users/arman/Documents/SNHU/CS 330/CS330Content/CS330Content/Projects/7-1_FinalProjectMilestones/White_Lid.jpg",
		"White_Lid");
	/***************************/

	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}
/***********************************************************
* DefineObjectMaterials()
 *
* This method is used for configuring the various material
* se?ngs for all of the objects in the 3D scene.
***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL steelMaterial;
	steelMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // low ambient
	steelMaterial.ambientStrength = 0.2f; // mild ambient
	steelMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f); //low diffuse
	steelMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f); // mild specular component
	steelMaterial.shininess = 128.0; //High Shininess like stainless steel
	steelMaterial.tag = "steel";
	m_objectMaterials.push_back(steelMaterial);

	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); // low ambient
	ceramicMaterial.ambientStrength = 0.4f; // mild ambient
	ceramicMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // high diffuse
	ceramicMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	ceramicMaterial.shininess = 32.0;  // mild shininess
	ceramicMaterial.tag = "ceramic";
	m_objectMaterials.push_back(ceramicMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // low ambient
	plasticMaterial.ambientStrength = 0.2f; // mile ambient 
	plasticMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f); // low diffuse
	plasticMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f); // high specular
	plasticMaterial.shininess = 64.0; // moderate shine
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL bagelMaterial; 
	bagelMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); // low ambient
	bagelMaterial.ambientStrength = 0.3f; // mild ambient strength
	bagelMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f); // warm, baked color
	bagelMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f); // low specular component
	bagelMaterial.shininess = 0.5; // low shine
	bagelMaterial.tag = "bagel";
	m_objectMaterials.push_back(bagelMaterial);

	OBJECT_MATERIAL paperMaterial;
	paperMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); // Low ambient component
	paperMaterial.ambientStrength = 0.4f; // Moderate ambient strength
	paperMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f); // High diffuse component
	paperMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Low specular component
	paperMaterial.shininess = 40.0; // Low shininess value
	paperMaterial.tag = "paper";
	m_objectMaterials.push_back(paperMaterial);

	OBJECT_MATERIAL waxMaterial;
	waxMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); // Low ambient component
	waxMaterial.ambientStrength = 0.4f; // Moderate ambient strength
	waxMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.6f); // High diffuse component
	waxMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.4f); // Moderate specular component
	waxMaterial.shininess = 32.0f; // Moderate shininess value
	waxMaterial.tag = "wax";
	m_objectMaterials.push_back(waxMaterial);

	OBJECT_MATERIAL candleFlameMaterial;
	candleFlameMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.0f); // No ambient component
	candleFlameMaterial.ambientStrength = 0.0f; // No ambient strength
	candleFlameMaterial.diffuseColor = glm::vec3(1.0f, 0.8f, 0.4f); // Yellow-orange color for the flame
	candleFlameMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f); // No specular component
	candleFlameMaterial.shininess = 0.0f; // No shininess value
	candleFlameMaterial.tag = "candleFlame";
	m_objectMaterials.push_back(candleFlameMaterial);
}

void SceneManager::SetupSceneLights()
{
	// Key Light 
	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 64.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 1.00f);

	// Fill Light
	m_pShaderManager->setVec3Value("lightSources[1].position", 3.0f, 14.0f, -3.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.02f, 0.02f, 0.02f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	// Back light
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.3f);

	// Rim light 1
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.3f);

	// Rim light 2
	m_pShaderManager->setVec3Value("lightSources[4].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[4].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[4].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[4].specularColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setFloatValue("lightSources[4].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[4].specularIntensity", 0.3f);

	m_pShaderManager->setBoolValue("bUseLighting", true);
}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// Table Plane //
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	// assigns wood texture to Table slot to plane mesh //
	SetShaderTexture("Table");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
		//   First Cylinder Jar ******//
		// set the XYZ scale for the jar mesh
	scaleXYZ = glm::vec3(2.0f, 3.95f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the jar mesh
	positionXYZ = glm::vec3(3.0f, 4.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use lightSlateGray //
	SetShaderColor(0.439, 0.502, 0.565, 1);
	// assigns Metal texture to Metal_S slot to cylinder mesh //
	SetShaderTexture("Metal_S");
	SetShaderMaterial("steel");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
			//   Second Cylinder Lid ******//
		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.8f, 1.5f, 1.7f);
	// set the XYZ rotation for the cylinder lid mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 100.0f;

	// set the XYZ position for the cylinder lid mesh
	positionXYZ = glm::vec3(4.0f, 5.2f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use DarkSlateGray instead of Black to differentiate//
	SetShaderColor(0.184, 0.310, 0.310, 1);
	// assigns Metal texture to Metal_S slot to cylinder lid mesh //
	SetShaderTexture("Metal_S");
	SetShaderMaterial("steel");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
		 //   Second Cylinder Lid Seal  ******//
		// set the XYZ scale for the cylinder lid seal mesh
	scaleXYZ = glm::vec3(1.9f, 0.3f, 1.75f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 100.0f;

	// set the XYZ position for the cylinder lid seal mesh
	positionXYZ = glm::vec3(3.5f, 5.35f, -.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use silver for lid seal//
	SetShaderColor(0.753, 0.753, 0.753, 1);
	// assigns Metal texture to Metal_S slot to cylinder mesh //
	SetShaderTexture("Plastic_P");
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
			 //    Cylinder jar lid handle  ******//
		// set the XYZ scale for the cylinder jar lid handle mesh
	scaleXYZ = glm::vec3(0.5f, 0.8f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 100.0f;

	// set the XYZ position for the cylinder jar lid handle mesh
	positionXYZ = glm::vec3(2.7f, 5.35f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use DimGray for lid handle//
	SetShaderColor(0.412, 0.412, 0.412, 1);
	// assigns dark metal texture to Metal_T slot to cylinder mesh //
	SetShaderTexture("Metal_T");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	// napkin  /
	// set the XYZ scale for the napkinmesh
	scaleXYZ = glm::vec3(2.0f, 1.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the napkin mesh
	positionXYZ = glm::vec3(4.0f, 0.08f, 3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use SeaShell for napkin //
	SetShaderColor(1, 1, 1, 1);
	// assigns paper texture slot to plane mesh //
	SetShaderTexture("Paper");
	SetShaderMaterial("paper");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
			 //    torus for bagel  ******//
		// set the XYZ scale for the torus mesh
	scaleXYZ = glm::vec3(0.9f, 1.0f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the torus mesh
	positionXYZ = glm::vec3(3.8f, 0.35f, 3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use orange for  torus//
	SetShaderColor(1.000, 0.647, 0.000, 1);
	// assigns everything bagel texture to Bagel_B slot to torus mesh //
	SetShaderTexture("Bagel_B");
	SetShaderMaterial("bagel");


	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
	/****************************************************************/
		//   candle ******//
		// set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(1.0f, 3.95f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the jar mesh
	positionXYZ = glm::vec3(-1.0f, 4.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use texture color //
	SetShaderColor(0.439, 0.502, 0.565, 1);
	// assigns Candle texture to Candle_C slot to cylinder mesh //
	SetShaderTexture("Candle_C");
	SetShaderMaterial("wax");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
			 //    Candle light  ******//
		// set the XYZ scale for the candle cylinder mesh //
		//   candle flame a little longer because I like how it looks // 
	scaleXYZ = glm::vec3(0.2f, 0.5f, 1.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 00.0f;
	ZrotationDegrees = 100.0f;

	// set the XYZ position for the cylinder  mesh
	positionXYZ = glm::vec3(-0.8f, 4.00f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use candle light color//
	SetShaderColor(0.412, 0.412, 0.412, 1);
	// assigns candle light texture to candle_L slot to cylinder mesh //
	SetShaderTexture("Candle_L");
	SetShaderMaterial("candleFlame");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/****************************************************************/
		//   mug ******//
		// set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(2.0f, 2.45f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the jar mesh
	positionXYZ = glm::vec3(-5.0f, 2.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use texture color //
	SetShaderColor(0.439, 0.502, 0.565, 1);
	// assigns Mug texture to Mug_M slot to cylinder mesh //
	SetShaderTexture("Mug_M");
	SetShaderMaterial("ceramic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
		//   water bottle ******//
		// set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.9f, 3.7f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -145.0f;

	// set the XYZ position for the water bottle mesh
	positionXYZ = glm::vec3(-6.3f, 3.9f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use texture color //
	SetShaderColor(0.439, 0.502, 0.565, 1);
	// assigns light blue texture to Lblue_B slot to cylinder lid mesh //
	SetShaderTexture("Lblue_B");
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
		//   white lid water bottle ******//
		// set the XYZ scale for the cylinder mesh
	scaleXYZ = glm::vec3(0.4f, 0.7f, 0.4f);

	// set the XYZ rotation for the lid mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -145.0f;

	// set the XYZ position for the water bottle mesh
	positionXYZ = glm::vec3(-6.6f, 4.3f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// will use texture color //
	SetShaderColor(0.439, 0.502, 0.565, 1);
	// assigns white lid texture to White_Lid slot to cylinder mesh //
	SetShaderTexture("White_Lid");
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	
}

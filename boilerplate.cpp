// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Co-Authors:
//			Jeremy Hart, University of Calgary
//			John Hall, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <vector>

#include "shapes.h"
#include "texture.h"
#include "Camera.h"

using namespace std;
using namespace glm;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

#define PI_F 3.14159265359f

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

bool lbPushed = false, ANIMATE = true;

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

// load, compile, and link shaders, returning true if successful
GLuint InitializeShaders()
{
	// load shader source from files
	string vertexSource = LoadSource("shaders/vertex.glsl");
	string fragmentSource = LoadSource("shaders/fragment.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// link shader program
	GLuint program = LinkProgram(vertex, fragment);

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	// check for OpenGL errors and return false if error occurred
	return program;
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct Geometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	Geometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

bool InitializeVAO(Geometry *geometry){

	const GLuint VERTEX_INDEX = 0;
	const GLuint TEXTURE_INDEX = 1;


	//Generate Vertex Buffer Objects
	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->textureBuffer);

	//Set up Vertex Array Object
	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(
		VERTEX_INDEX,		//Attribute index 
		3, 					//# of components
		GL_FLOAT, 			//Type of component
		GL_FALSE, 			//Should be normalized?
		sizeof(vec3),		//Stride - can use 0 if tightly packed
		0);					//Offset to first element
	glEnableVertexAttribArray(VERTEX_INDEX);

	// associate the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glVertexAttribPointer(
		TEXTURE_INDEX,		//Attribute index 
		2, 					//# of components
		GL_FLOAT, 			//Type of component
		GL_FALSE, 			//Should be normalized?
		sizeof(vec2), 		//Stride - can use 0 if tightly packed
		0);					//Offset to first element
	glEnableVertexAttribArray(TEXTURE_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return !CheckGLErrors();
}

// create buffers and fill with geometry data, returning true if successful
bool LoadGeometry(Geometry *geometry, vec3 *vertices, vec2 *textures, int elementCount)
{
	geometry->elementCount = elementCount;

	// create an array buffer object for storing our vertices
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*geometry->elementCount, vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2)*geometry->elementCount, textures, GL_STATIC_DRAW);

	//Unbind buffer to reset to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(Geometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
	glDeleteBuffers(1, &geometry->colourBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene(Geometry *geometry, GLuint program, vec3 color, Camera* camera, mat4 perspectiveMatrix, mat4 translationMatrix, GLenum rendermode)
{

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(program);

	int vp [4];
	glGetIntegerv(GL_VIEWPORT, vp);
	int width = vp[2];
	int height = vp[3];


	//Bind uniforms
	GLint uniformLocation = glGetUniformLocation(program, "Colour");
	glUniform3f(uniformLocation, color.r, color.g, color.b); 

	mat4 modelViewProjection = perspectiveMatrix*camera->viewMatrix();
	uniformLocation = glGetUniformLocation(program, "modelViewProjection");
	glUniformMatrix4fv(uniformLocation, 1, false, glm::value_ptr(modelViewProjection));

	
	uniformLocation = glGetUniformLocation(program, "translation");
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(translationMatrix));

	glBindVertexArray(geometry->vertexArray);
	glDrawArrays(rendermode, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}


//takes in a sphere and azimuth/altitude interval 
//outputs an approximated sphere
layer approximateLayer(sphere &sph, float interval, float phi_IN, float layerIndex){



	float phi = phi_IN * (PI_F/180);
	vec3 c = sph.center;
	float r = sph.radius * sin(phi);



	if (phi_IN == 90.f)
		r = sph.radius;

	
	//printf("Phi: %f\tsin(90-phi): %f\tr: %f\n", phi_IN, sin((PI_F/2)-phi), r);

	//calculate for y coordinate
	//float yLayer = phi/interval - (180.f / (2*interval)); 
	float y = cos(phi) * sph.radius;

//	cout << r << endl;
	//contains vertices (vec3) of one layer of the approximation 
	vector <vec3> sphereLayer;
	layer texLayer;


	for ( float theta = 0.f; theta < 360.f; theta+= interval){
		vec3 vertex = vec3(r*sin(theta*(PI_F/180.f)), y,r*cos(theta*(PI_F/180.f)) ) ;
		sphereLayer.push_back(vertex+c);
		texLayer.texCoord.push_back( vec2(radians(theta)/(2*PI_F), phi/PI_F) );
		/*
		//determine texCoord
		if (phi_IN == interval){
			vec2 tex1 = vec2(radians(theta)/(2*PI_F) , 0.5);
		}
		vec2 tex1 = vec2(radians(theta)/(2*PI_F) , phi/PI_F);
		vec2 tex2 = vec2(radians(theta+interval)/(2*PI_F) , phi/PI_F);
		vec2 tex3 = vec2(radians(theta+interval)/(2*PI_F) , (phi+interval)/PI_F);
		sph.texCoord.push_back(tex1);
		sph.texCoord.push_back(tex2);
		sph.texCoord.push_back(tex3);

		tex2 = vec2(radians(theta)/(2*PI_F) , (phi+interval)/PI_F);
		sph.texCoord.push_back(tex1);
		sph.texCoord.push_back(tex2);
		sph.texCoord.push_back(tex3);
		*/

		printf("x: %f\ty  %f\n", radians(theta)/(2*PI_F), phi/PI_F);
	} 


	cout <<endl;
	sph.texLayers.push_back(texLayer);	
	return layer(sphereLayer);
	
}

sphere approximateSphere(vec3 center, float radius, float interval){

	//sphere sph = sphere(center, radius);
	//vector<layer> layers;
	sphere sph = sphere(center, radius);
	float layerIndex = (int)(90.f/interval) - 1;
	

	for(float phi = interval; phi <= (180.f-interval); phi += interval ){
		
		layer l = approximateLayer(sph, interval, phi, layerIndex);
		layerIndex--;
		sph.layers.push_back(l);

		
			

	}
	
	//sph = sphere(center, radius, layers);
	return sph;

}

//not so testcode------------------------------------------------------------------
//load all the vertices of a sphere into a vector to render onto the screen
vector<vec3> loadSpherePOINTS(sphere sph){

	vector<vec3> vertices;
	for(layer l:sph.layers){
		for (vec3 v: l.vertices){
			vertices.push_back(v);
		}
	}

	return vertices;
}





//have top cap, work on the rest ,
layer loadSphereTRI(sphere &sph){

	//vector<vec2> texCoord = sph.texCoord;
	vec2 poleTex = vec2(0.f, 1.f);
	vec2 poleSouthTex = vec2(0.f, 0.f);

	vector<vec3> vertices;
	vector<vec2> texCoord;

	for(int indexL = 0; indexL <  sph.layers.size(); indexL++){
		layer l = sph.layers[indexL];
		layer texL = sph.texLayers[indexL];

		for (int indexV = 0; indexV < l.vertices.size(); indexV++){
			if (indexL == 0){
				vertices.push_back(sph.pole);
				vertices.push_back(l.vertices[indexV]);

				//tex
				texCoord.push_back(poleTex);
				texCoord.push_back(texL.texCoord[indexV]);

				if (indexV < l.vertices.size()-1){
					vertices.push_back(l.vertices[indexV+1]);
					texCoord.push_back(texL.texCoord[indexV+1]);
				}
					
				else{
					vertices.push_back(l.vertices[0]);
					texCoord.push_back(texL.texCoord[0]);
				}
					
					
			}
			//else (the  rest of the body read normally)
			//---- up across -> across up
			else{
				
				layer upLayer = sph.layers[indexL-1];
				layer upTexLayer = sph.texLayers[indexL-1];
				//vec3 upVertex = upLayer.vertices[indexV];

				//up across
				vertices.push_back(l.vertices[indexV]);
				vertices.push_back(upLayer.vertices[indexV]);

				//tex
				texCoord.push_back(texL.texCoord[indexV]);
				texCoord.push_back(upTexLayer.texCoord[indexV]);

				if (indexV < l.vertices.size()-1){
					vertices.push_back(upLayer.vertices[indexV+1]);
					texCoord.push_back(upTexLayer.texCoord[indexV+1]);
				}
					
				else {
					vertices.push_back(upLayer.vertices[0]);
					texCoord.push_back(upTexLayer.texCoord[0]);
				}


			
				//across up
				vertices.push_back(l.vertices[indexV]);
				texCoord.push_back(texL.texCoord[indexV]);

				if (indexV < l.vertices.size()-1){
					vertices.push_back(l.vertices[indexV+1]);
					vertices.push_back(upLayer.vertices[indexV+1]);

					texCoord.push_back(texL.texCoord[indexV+1]);
					texCoord.push_back(upTexLayer.texCoord[indexV+1]);
				}
				
				else{
					vertices.push_back(l.vertices[0]);
					vertices.push_back(upLayer.vertices[0]);

					texCoord.push_back(texL.texCoord[0]);
					texCoord.push_back(upTexLayer.texCoord[0]);
				}	
				
				

			}
		}

	
	}

	//-----handle bottom cap here 
	vector<vec3>  lastLayer  = sph.layers[sph.layers.size()-1].vertices;
	vector<vec2>  lastTexLayer = sph.texLayers[sph.texLayers.size()-1].texCoord;
	for (int i = 0; i<lastLayer.size(); i++){
		vertices.push_back(sph.poleSouth);
		vertices.push_back(lastLayer[i]);

		//tex
		texCoord.push_back(poleSouthTex);
		texCoord.push_back(lastTexLayer[i]);

		
		if (i < lastLayer.size()-1){
			vertices.push_back(lastLayer[i+1]);
			texCoord.push_back(lastTexLayer[i+1]);
		}	
		else {
			vertices.push_back(lastLayer[0]);
			texCoord.push_back(lastTexLayer[0]);
		}		
	} 

	layer RESULT = layer(vertices);
	RESULT.texCoord = texCoord;
	return RESULT;
}









































 float intlog (float base, float x) {
    return (log(x) / log(base));
}

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		ANIMATE = !ANIMATE;
}


// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	int width = 1024, height = 1024;
	window = glfwCreateWindow(width, height, "CPSC 453 OpenGL Boilerplate", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwMakeContextCurrent(window);

	//Intialize GLAD
	if (!gladLoadGL())
	{
		cout << "GLAD init failed" << endl;
		return -1;
	}

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	GLuint program = InitializeShaders();
	if (program == 0) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}


	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// three vertex positions and assocated colours of a triangle
	/*
	vec3 vertices[] = {
		vec3( -.6f, -.4f, -0.5f ),
		vec3( .0f,  .6f, -0.5f ),
		vec3( .6f, -.4f,-0.5f )
	};
	*/


	//vector<layer> layers;
	//sphere sph = sphere(center, radius, layers);
	sphere sph = approximateSphere(vec3(0,0,0), 1.f, 10.f );



	vector<vec3> vertices = loadSphereTRI(sph).vertices;
	//vertices.push_back(sph.pole);
	//vec3 test = -1.f*sph.pole;
	//vertices.push_back(test);


	vec3 frustumVertices[] = {
		vec3(-1, -1, -1),
		vec3(-1, -1, 1),
		vec3(-1, 1, 1),
		vec3(1, 1, 1),
		vec3(1, 1, -1),
		vec3(-1, 1, -1),
		vec3(-1, -1, -1),
		vec3(1, -1, -1),
		vec3(1, -1, 1),
		vec3(-1, -1, 1),
		vec3(-1, 1, 1),
		vec3(-1, 1, -1),
		vec3(1, 1, -1),
		vec3(1, -1, -1),
		vec3(1, -1, 1),
		vec3(1, 1, 1)
	};

	mat4 perspectiveMatrix = glm::perspective(PI_F*0.25f, float(width)/float(height), 0.1f, 500.f);	//Fill in with Perspective Matrix

	for(int i=0; i<16; i++){
		vec4 newPoint = inverse(perspectiveMatrix)*vec4(frustumVertices[i], 1);
		frustumVertices[i] = vec3(newPoint)/newPoint.w;
	}

	//-----------------TEXTURE STUFF------------------------
		MyTexture earthTex;

		char filePaths[3][50] ={
			"./textures/2k_earth_daymap.jpg",
			"./textures/2k_moon.jpg",
			"./textures/sun.jpg"
		} ;
		if(InitializeTexture(&earthTex, filePaths[0], GL_TEXTURE_2D)){
			cout<<"hi"<<endl;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, earthTex.textureID);
		GLint sample = glGetUniformLocation(program, "s");
		glUseProgram(program);
		glUniform1i(sample, 0);
		glUseProgram(0);

	//======================================================

	Geometry geometry;
	Geometry frustumGeometry;

	//DONT FORGET THIS
	vector<vec2> texCoord = loadSphereTRI(sph).texCoord;

	//cout << vertices.size() <<endl;
	//cout << texCoord.size() <<endl;

	
	for (int i = 0; i<vertices.size(); i++){
		//printf("x: %f\ty: %f\n", texCoord[i].x, texCoord[i].y);
	}


	// call function to create and fill buffers with geometry data
	if (!InitializeVAO(&geometry))
		cout << "Program failed to intialize geometry!" << endl;

	if(!LoadGeometry(&geometry, &vertices[0], &texCoord[0], vertices.size()))
		cout << "Failed to load geometry" << endl;

	//if (!InitializeVAO(&frustumGeometry))
	//	cout << "Program failed to intialize geometry!" << endl;

	//if(!LoadGeometry(&frustumGeometry, frustumVertices, 16))
	//	cout << "Failed to load geometry" << endl;

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	Camera cam;

	vec2 lastCursorPos;

	float cursorSensitivity = PI_F/200.f;	//PI/hundred pixels
	float movementSpeed = 1.f;
	
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	glfwSetCursorPos(window, width/2, height/2);


	glPointSize(5.f);


	
	cam.initializeCamera(0.f,90.f, -190.f);
	cam.pof = vec3(0,0,0);
	//cam.move(vec3(0.000001f, 0.0000001f,0));

	//---------------TEST CODE---------------
	
	float logDistance = 1.2f;
	float logSize = 2.f;

	

	//sun setup and tranformation
	float sunSize = intlog(logSize, 695508.f);


	//translating earth around the su = rotation matrix in world coord *initial earth pos 
	//earth data setup
	float eDistance = intlog(logDistance, 1496000.f);  
	float eSize = intlog(logSize, 6371.f);
	vec3 earthInitPos = vec3 (eDistance, 0.f, 0.f);
	vec4 axis = rotate(mat4(1.f), radians(23.4f), vec3(0.f,0.f,1.f)) * vec4 (0.f,1.f,0.f, 1.f);
	vec3 earthAxis = vec3 (axis);



	//moon data setup 
	float moonDistance = intlog(logDistance, 384400.f)/2; 
	float moonSize = intlog(logSize, 1737.f);


//	printf("S: %f \tE: %f\tM: %f\n", sunSize, eSize, moonSize);
//	printf("\teDist: %f\tmDis: %f\n", eDistance, moonDistance);

	//mat4 rotationMatrix = rotate(30.f, rotationAxis);

	glm::mat4 transformation = glm::mat4(1.0f);

	
	
	//vec3 rotationAxis = vec3(0,1,0);
	

	//glm::mat4 myScalingMatrix = glm::scale(0.5f, 0.5f, 0.5f);

	//Bind uniforms
	//GLint uniformLocation = glGetUniformLocation(program, "translation");
	//glUniformMatrix4fv(uniformLocation, 1, false, glm::value_ptr(translationMatrix));

	

	//=======================================

	float rotateSun = 0, srSpeed = 1.f;
	float orbitEarth = 0, eoSpeed = srSpeed/14.37; 
	float rotateEarth = 0, erSpeed = srSpeed * 25.4;
	float orbitMoon = 0, moSpeed = erSpeed/27.f;
	float rotateMoon = 0, mrSpeed = erSpeed/27.32;
	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		////////////////////////
		//Camera interaction
		////////////////////////
		//Translation
		vec3 movement(0.f);
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			movement.z += 1.f;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			movement.z -= 1.f;

		
		//cam.move(movement*movementSpeed);
			
	

		//Rotation
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		vec2 cursorPos(xpos, ypos);
		vec2 cursorChange = cursorPos - vec2(width/2, height/2);
	
		cam.move(vec3(cursorChange*0.1f, movement.z*movementSpeed));

		//if(glfwGetMouseButton(window, 	GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
			//cam.rotateHorizontal(-cursorChange.x*cursorSensitivity);
			//cam.rotateVertical(-cursorChange.y*cursorSensitivity);
			
		//}
	
		glfwSetCursorPos(window, width/2, height/2);
		//lastCursorPos = vec2(width/2, height/2);
		
		
		///////////
		//Calcualtions
		//////////
		mat4 translationSun = mat4(1.f);
		mat4 rotationSun = rotate(mat4(1.0), radians(rotateSun), vec3(0.0f, 1.0f, 0.0f)); 
		mat4 scalingSun = scale(mat4(1.0f),vec3(sunSize,sunSize, sunSize));
		mat4 transformSun = translationSun * rotationSun * scalingSun; 

		//earth transformations
		//mat4 translationEarth = translate(mat4(1.0f), vec3(eDistance, 0.0f, 0.0f));
		vec4 temp =  rotate(mat4(1.f), radians(orbitEarth), vec3(0.f,1.f,0.f)) * vec4( eDistance, 0.0f, 0.0f, 1.f);
		vec3 earthlocation = vec3(temp.x, temp.y, temp.z);
		mat4 translationEarth =  translate(mat4(1.0f), earthlocation);
		mat4 rotationEarth = rotate(mat4(1.0), radians(rotateEarth), earthAxis); 
		mat4 scalingEarth = scale(mat4(1.0f),vec3(eSize,eSize, eSize));
		mat4 transformEarth = translationSun * translationEarth * rotationEarth * scalingEarth
									 * rotate(mat4(1.f), radians(23.4f), vec3(0.f,0.f,1.f)); //the axis 

		//moon tranformation
		//mat4 translationMoon = translate(mat4(1.0f), vec3(moonDistance, 0.0f, 0.0f));
		mat4 translationMoon = rotate(mat4(1.f), radians(orbitMoon), vec3(0.f,1.f,0.f)) * translate(mat4(1.0f), vec3(moonDistance, 0.0f, 0.0f));
		mat4 rotationMoon = rotate(mat4(1.0), radians(rotateMoon), vec3(0.0f, -1.0f, 0.0f));
		mat4 scalingMoon = scale(mat4(1.0f),vec3(moonSize,moonSize,moonSize));
		mat4 transformMoon = translationSun * translationEarth * translationMoon * rotationMoon * scalingMoon; 

		

		
		///////////
		//Drawing
		//////////

		//get texture from https://www.solarsystemscope.com/textures/

		// clear screen to a dark grey colour
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		// call function to draw our scene
		RenderScene(&geometry, program, vec3(1, 0, 0), &cam, perspectiveMatrix, transformSun, GL_TRIANGLES);
		RenderScene(&geometry, program, vec3(0, 0, 1), &cam, perspectiveMatrix, transformEarth, GL_TRIANGLES);
		RenderScene(&geometry, program, vec3(1, 1, 1), &cam, perspectiveMatrix, transformMoon, GL_TRIANGLES);
		//RenderScene(&frustumGeometry, program, vec3(0, 0, 1), &cam, perspectiveMatrix, glm::mat4(1.0f), GL_LINE_STRIP);



		glfwSwapBuffers(window);

		glfwPollEvents();

		if (ANIMATE == true){
			rotateSun += srSpeed;
			orbitEarth += eoSpeed;
			rotateEarth += erSpeed;
			orbitMoon += moSpeed;
			rotateMoon += mrSpeed;
		}
	
	}

	// clean up allocated resources before exit
	DestroyGeometry(&geometry);
	glUseProgram(0);
	glDeleteProgram(program);
	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}

//
// seriously modified ZJ Wood January 2015 - conversion to glfw
// inclusion of matrix stack Feb. 2015
// original from Shinjiro Sueda
// October, 2014
//

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cassert>
#include <cmath>
#include <stdio.h>
#include "GLSL.h"
#include <unistd.h>
#include "tiny_obj_loader.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp" //perspective, trans etc
#include "glm/gtc/type_ptr.hpp" //value_ptr
#include "RenderingHelper.h"
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#define SOCK_PATH "../../source/maze_resources/maze_simulator"

GLFWwindow* window;
using namespace std;
typedef struct object object;
struct object {
    vector<tinyobj::shape_t> shape;
    GLuint PosBufObj=0;
    GLuint NorBufObj=0;
    GLuint IndBufObj=0;
};

object body;
object sensor;
object cube;
object sensor_ray;
object left_tire;
object right_tire;
vector<tinyobj::material_t> materials;
GLuint ShadeProg;


GLint h_aPosition;
GLint h_aNormal;
GLint h_uModelMatrix;
GLint h_uViewMatrix;
GLint h_uProjMatrix;
GLint h_uLightPos;
GLint h_uMatAmb, h_uMatDif, h_uMatSpec, h_uMatShine;
GLint h_uShadeM;
GLint h_LightCol;
GLint wall;
int first_run=1;

// Initial position : on +Z
glm::vec3 camera_position;
glm::vec3 auto_move_camera = glm::vec3( 0, 2, 0 );
glm::vec3 floater = glm::vec3( 0, 0, 0 );
glm::vec3 direction = glm::vec3( 0, 0, 0 );
glm::vec3 up =  glm::vec3(0.0f, 1.0f, 0.0f);
// Initial horizontal angle : toward -Z
float horizontalAngle = 4.13f;
// Initial vertical angle : none
float verticalAngle = -1.32f;
// Initial Field of View
float initialFoV = 45.0f;

float speed = 10.0f; // 3 units / second
float mouseSpeed = 0.001f;

glm::vec3 g_light;
static float g_width, g_height;
float theta;
float deltaTime=0.0f;
int view_select=1;
int sensor_on=0;


void mouseFunction();

void *startServer(void *threadid);
char *lines_array;
float left_proximity_reading, right_proximity_reading, top_left_proximity_reading, top_right_proximity_reading=0;

RenderingHelper ModelTrans;

/* helper function to send materials to the shader - you must create your own */
void SetMaterial(int i) {
    
    glUseProgram(ShadeProg);
    switch (i) {
        case 0: //base top
            glUniform3f(h_uMatAmb, 0.05375,	0.05,	0.06625);
            glUniform3f(h_uMatDif, 0.18275,	0.17,	0.22525);
            glUniform3f(h_uMatSpec,0.332741,	0.328634,	0.346435);
            glUniform1f(h_uMatShine, 0.3);
            break;
        case 1: // silver
            glUniform3f(h_uMatAmb,0.135,	0.2225,	0.1575);
            glUniform3f(h_uMatDif, 0.54,	0.89,	0.63);
            glUniform3f(h_uMatSpec, 0.316228,	0.316228,	0.316228);
            glUniform1f(h_uMatShine, 0.1);
            break;
        case 2: // black rubber
            glUniform3f(h_uMatAmb, 0.02, 0.02, 0.02);
            glUniform3f(h_uMatDif, 0.01, 0.01, 0.01);
            glUniform3f(h_uMatSpec, 0.4, 0.4, 0.4);
            glUniform1f(h_uMatShine, 0.078125);
            break;
        case 3: //wall_top red
            glUniform3f(h_uMatAmb, 0.0, 0.0, 0.0);
            glUniform3f(h_uMatDif, 0.5, 0.0, 0.0);
            glUniform3f(h_uMatSpec, 0.7, 0.6, 0.6);
            glUniform1f(h_uMatShine, 0.25);
            break;
        case 4: //body green
            glUniform3f(h_uMatAmb, 0.0, 0.0, 0.0);
            glUniform3f(h_uMatDif, 0.5,	0.5,	0.0);
            glUniform3f(h_uMatSpec, 0.60,	0.60,	0.50);
            glUniform1f(h_uMatShine, 0.25);
            break;
        case 5: //chrome wall side
            glUniform3f(h_uMatAmb, 0.25, 0.25, 0.25);
            glUniform3f(h_uMatDif, 0.4, 0.4, 0.4);
            glUniform3f(h_uMatSpec, 0.774597, 0.774597, 0.774597);
            glUniform1f(h_uMatShine, 0.6);
        break;
    }
}

//Given a vector of shapes which has already been read from an obj file
// resize all vertices to the range [-1, 1]
void resize_obj(std::vector<tinyobj::shape_t> &shapes){
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    float scaleX, scaleY, scaleZ;
    float shiftX, shiftY, shiftZ;
    float epsilon = 0.001;
    
    minX = minY = minZ = 1.1754E+38F;
    maxX = maxY = maxZ = -1.1754E+38F;
    
    //Go through all vertices to determine min and max of each dimension
    for (size_t i = 0; i < shapes.size(); i++) {
        for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
            if(shapes[i].mesh.positions[3*v+0] < minX) minX = shapes[i].mesh.positions[3*v+0];
            if(shapes[i].mesh.positions[3*v+0] > maxX) maxX = shapes[i].mesh.positions[3*v+0];
            
            if(shapes[i].mesh.positions[3*v+1] < minY) minY = shapes[i].mesh.positions[3*v+1];
            if(shapes[i].mesh.positions[3*v+1] > maxY) maxY = shapes[i].mesh.positions[3*v+1];
            
            if(shapes[i].mesh.positions[3*v+2] < minZ) minZ = shapes[i].mesh.positions[3*v+2];
            if(shapes[i].mesh.positions[3*v+2] > maxZ) maxZ = shapes[i].mesh.positions[3*v+2];
        }
    }
    //From min and max compute necessary scale and shift for each dimension
    float maxExtent, xExtent, yExtent, zExtent;
    xExtent = maxX-minX;
    yExtent = maxY-minY;
    zExtent = maxZ-minZ;
    if (xExtent >= yExtent && xExtent >= zExtent) {
        maxExtent = xExtent;
    }
    if (yExtent >= xExtent && yExtent >= zExtent) {
        maxExtent = yExtent;
    }
    if (zExtent >= xExtent && zExtent >= yExtent) {
        maxExtent = zExtent;
    }
    scaleX = 2.0 /maxExtent;
    shiftX = minX + (xExtent/ 2.0);
    scaleY = 2.0 / maxExtent;
    shiftY = minY + (yExtent / 2.0);
    scaleZ = 2.0/ maxExtent;
    shiftZ = minZ + (zExtent)/2.0;
    
    //Go through all verticies shift and scale them
    for (size_t i = 0; i < shapes.size(); i++) {
        for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
            shapes[i].mesh.positions[3*v+0] = (shapes[i].mesh.positions[3*v+0] - shiftX) * scaleX;
            assert(shapes[i].mesh.positions[3*v+0] >= -1.0 - epsilon);
            assert(shapes[i].mesh.positions[3*v+0] <= 1.0 + epsilon);
            shapes[i].mesh.positions[3*v+1] = (shapes[i].mesh.positions[3*v+1] - shiftY) * scaleY;
            assert(shapes[i].mesh.positions[3*v+1] >= -1.0 - epsilon);
            assert(shapes[i].mesh.positions[3*v+1] <= 1.0 + epsilon);
            shapes[i].mesh.positions[3*v+2] = (shapes[i].mesh.positions[3*v+2] - shiftZ) * scaleZ;
            assert(shapes[i].mesh.positions[3*v+2] >= -1.0 - epsilon);
            assert(shapes[i].mesh.positions[3*v+2] <= 1.0 + epsilon);
        }
    }
}

void loadShapes(const string &objFile, vector<tinyobj::shape_t> *shapes)
{
	string err = tinyobj::LoadObj(*shapes, materials, objFile.c_str());
	if(!err.empty())
	{
		cerr << err << endl;
	}
    resize_obj(*shapes);
}

void bind_stuff(object *obj)
{
    // Send the position array to the GPU
    tinyobj::shape_t shape=obj->shape[0];
    GLuint *PosBufObj = &(obj->PosBufObj);
    GLuint *NorBufObj = &(obj->NorBufObj);
    GLuint *IndBufObj = &(obj->IndBufObj);
    const vector<float> &bposBuf = shape.mesh.positions;
    glGenBuffers(1, PosBufObj);
    glBindBuffer(GL_ARRAY_BUFFER, *PosBufObj);
    glBufferData(GL_ARRAY_BUFFER, bposBuf.size()*sizeof(float), &bposBuf[0], GL_STATIC_DRAW);
    
    // TODO compute the normals per vertex - you must fill this in
    vector<float> bnorBuf;
    int idx1, idx2, idx3;
    glm::vec3 v1, v2, v3;
    //for every vertex initialize a normal to 0
    for (int j = 0; j < shape.mesh.positions.size()/3; j++) {
        bnorBuf.push_back(0);
        bnorBuf.push_back(0);
        bnorBuf.push_back(0);
    }
    // DO work here to compute the normals for every face
    //then add its normal to its associated vertex
    for (int i = 0; i < shape.mesh.indices.size()/3; i++) {
        idx1 = shape.mesh.indices[3*i+0];
        idx2 = shape.mesh.indices[3*i+1];
        idx3 = shape.mesh.indices[3*i+2];
        v1 = glm::vec3(shape.mesh.positions[3*idx1 +0],shape.mesh.positions[3*idx1 +1], shape.mesh.positions[3*idx1 +2]);
        v2 = glm::vec3(shape.mesh.positions[3*idx2 +0],shape.mesh.positions[3*idx2 +1], shape.mesh.positions[3*idx2 +2]);
        v3 = glm::vec3(shape.mesh.positions[3*idx3 +0],shape.mesh.positions[3*idx3 +1], shape.mesh.positions[3*idx3 +2]);
        
        vec3 crossproduct=glm::cross(v1-v2, v1-v3);
        //This is not correct, it sets the normal as the vertex value but
        //shows access pattern
        bnorBuf[3*idx1+0] += crossproduct.x;
        bnorBuf[3*idx1+1] += crossproduct.y;
        bnorBuf[3*idx1+2] += crossproduct.z;
        bnorBuf[3*idx2+0] += crossproduct.x;
        bnorBuf[3*idx2+1] += crossproduct.y;
        bnorBuf[3*idx2+2] += crossproduct.z;
        bnorBuf[3*idx3+0] += crossproduct.x;
        bnorBuf[3*idx3+1] += crossproduct.y;
        bnorBuf[3*idx3+2] += crossproduct.z;
    }
    for(int i=0; i<bnorBuf.size();i=i+3)
    {
        vec3 normalized_vector= glm::normalize(glm::vec3(bnorBuf[i],bnorBuf[i+1], bnorBuf[i+2]));
        bnorBuf[i]=normalized_vector.x;
        bnorBuf[i+1]=normalized_vector.y;
        bnorBuf[i+2]=normalized_vector.z;
        
    }
    glGenBuffers(1, NorBufObj);
    glBindBuffer(GL_ARRAY_BUFFER, *NorBufObj);
    glBufferData(GL_ARRAY_BUFFER, bnorBuf.size()*sizeof(float), &bnorBuf[0], GL_STATIC_DRAW);
    
    // Send the index array to the GPU
    const vector<unsigned int> &indBuf = shape.mesh.indices;
    glGenBuffers(1, IndBufObj);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *IndBufObj);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf.size()*sizeof(unsigned int), &indBuf[0], GL_STATIC_DRAW);

}


void initGL()
{
	// Set the background color
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable Z-buffer test
	glEnable(GL_DEPTH_TEST);
    
    bind_stuff(&body);
    bind_stuff(&sensor);
    bind_stuff(&cube);
    bind_stuff(&sensor_ray);
    bind_stuff(&left_tire);
    bind_stuff(&right_tire);
    
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	GLSL::checkVersion();
	assert(glGetError() == GL_NO_ERROR);
    
    ModelTrans.useModelViewMatrix();
    ModelTrans.loadIdentity();
//    srand (time(NULL));
//    for (int i=0; i<=40; i++) {
//        positions[i]=rand()%70;
//    }
    int k;
    k=5;
}

bool installShaders(const string &vShaderName, const string &fShaderName)
{
	GLint rc;
	
	// Create shader handles
	GLuint VS = glCreateShader(GL_VERTEX_SHADER);
	GLuint FS = glCreateShader(GL_FRAGMENT_SHADER);
	
	// Read shader sources
	const char *vshader = GLSL::textFileRead(vShaderName.c_str());
	const char *fshader = GLSL::textFileRead(fShaderName.c_str());
	glShaderSource(VS, 1, &vshader, NULL);
	glShaderSource(FS, 1, &fshader, NULL);
	
	// Compile vertex shader
	glCompileShader(VS);
	GLSL::printError();
	glGetShaderiv(VS, GL_COMPILE_STATUS, &rc);
	GLSL::printShaderInfoLog(VS);
	if(!rc)
	{
		printf("Error compiling vertex shader %s\n", vShaderName.c_str());
		return false;
	}
	
	// Compile fragment shader
	glCompileShader(FS);
	GLSL::printError();
	glGetShaderiv(FS, GL_COMPILE_STATUS, &rc);
	GLSL::printShaderInfoLog(FS);
	if(!rc)
	{
		printf("Error compiling fragment shader %s\n", fShaderName.c_str());
		return false;
	}
	
	// Create the program and link
	ShadeProg = glCreateProgram();
	glAttachShader(ShadeProg, VS);
	glAttachShader(ShadeProg, FS);
	glLinkProgram(ShadeProg);
	GLSL::printError();
	glGetProgramiv(ShadeProg, GL_LINK_STATUS, &rc);
	GLSL::printProgramInfoLog(ShadeProg);
	if(!rc)
	{
		printf("Error linking shaders %s and %s\n", vShaderName.c_str(), fShaderName.c_str());
		return false;
	}
	
	// Set up the shader variables
    h_aPosition = GLSL::getAttribLocation(ShadeProg, "aPosition");
    h_aNormal = GLSL::getAttribLocation(ShadeProg, "aNormal");
    h_uProjMatrix = GLSL::getUniformLocation(ShadeProg, "uProjMatrix");
    h_uViewMatrix = GLSL::getUniformLocation(ShadeProg, "uViewMatrix");
    h_uModelMatrix = GLSL::getUniformLocation(ShadeProg, "uModelMatrix");
    h_uLightPos = GLSL::getUniformLocation(ShadeProg, "uLightPos");
    h_uMatAmb = GLSL::getUniformLocation(ShadeProg, "UaColor");
    h_uMatDif = GLSL::getUniformLocation(ShadeProg, "UdColor");
    h_uMatSpec = GLSL::getUniformLocation(ShadeProg, "UsColor");
    h_uMatShine = GLSL::getUniformLocation(ShadeProg, "Ushine");
    h_LightCol= GLSL::getUniformLocation(ShadeProg, "uLightCol");
	wall = GLSL::getUniformLocation(ShadeProg, "wall");
	assert(glGetError() == GL_NO_ERROR);
	return true;
}
float side_length;
float num_of_cells;
float wall_height=0.5f;
float wall_width=0.2f;
float maze_offset=5.0f;
float line_width;
glm::vec3 mouse_position;
glm::vec3 mouse_forward_vector = glm::vec3( -1, 0, 0 );
float mouse_rotate=0.0f;

int check_collision2(glm::vec3 new_position)
{
    int current_state=1;
    float x_coord, z_coord;
    int i,j, grid_position_x, grid_position_z;
    int line_index;
    for (j=-1;j<=1; j++)
        {
        for (i=0;i<=1; i++) {
            grid_position_x=(int)((new_position.x-maze_offset)/line_width)/2+j;
            grid_position_z=(int)((new_position.z-maze_offset)/line_width)/2+i;
            line_index=(grid_position_x+1)*((int)num_of_cells-1)+1-grid_position_z;
            if (grid_position_z==0 || grid_position_z==num_of_cells || lines_array[line_index]=='1')
            {
                x_coord=maze_offset+grid_position_x*2*line_width;
                z_coord=maze_offset+grid_position_z*2*line_width;
                if ((x_coord-0.25 < new_position.x && new_position.x < x_coord+2*line_width+0.25))
                    if(z_coord-4*wall_width-0.5  < new_position.z && new_position.z < z_coord+4*wall_width+0.5)
                    {
                        current_state=0;
                    }
            }
        }
    }
    for (j=-1;j<=1; j++)
    {
        for (i=0;i<=1; i++) {
            grid_position_x=(int)((mouse_position.x-maze_offset)/line_width)/2+i;
            grid_position_z=(int)((mouse_position.z-maze_offset)/line_width)/2;
            line_index=(int)(num_of_cells*(num_of_cells-1))+(grid_position_x)*(int)num_of_cells-grid_position_z;
            if (grid_position_x==0 || grid_position_x==num_of_cells || lines_array[line_index]=='1')
            {
                x_coord=maze_offset+grid_position_x*2*line_width;
                z_coord=maze_offset+grid_position_z*2*line_width;
                if ((z_coord-0.25 < new_position.z && new_position.z < z_coord+2*line_width+0.25))
                    if(x_coord-4*wall_width-0.5  < new_position.x && new_position.x < x_coord+4*wall_width+0.5)
                    {
                        current_state=0;
                    }
            }
        }
    }
    
    return current_state;
    
}


int check_sensor_ray_collision2(glm::vec3 new_position)
{
    int grid_position_x,grid_position_z;
    int line_index=1;
    float x_coord, z_coord;
    int current_state=1;
    //vertical
    for(grid_position_x=0.0f;grid_position_x<num_of_cells;grid_position_x++)
    {
        for(grid_position_z=num_of_cells;grid_position_z>=0;grid_position_z--)
        {
            if (grid_position_z==0 || grid_position_z==num_of_cells || lines_array[line_index]=='1')
            {
                x_coord=maze_offset+grid_position_x*2*line_width;
                z_coord=maze_offset+grid_position_z*2*line_width;
                if ((x_coord< new_position.x && new_position.x < x_coord+2*line_width))
                    if(z_coord-wall_width  < new_position.z && new_position.z < z_coord)
                    {
                        current_state=0;
                    }
            }
            if (grid_position_z!=0 && grid_position_z!=num_of_cells)
                line_index++;
        }
    }

    for(grid_position_x=0.0f;grid_position_x<=num_of_cells;grid_position_x++)
    {
        for(grid_position_z=num_of_cells-1;grid_position_z>=0;grid_position_z--)
        {
            if (grid_position_x==0 || grid_position_x==num_of_cells || lines_array[line_index]=='1')
            {
                x_coord=maze_offset+grid_position_x*2*line_width;
                z_coord=maze_offset+grid_position_z*2*line_width;
                if ((z_coord < new_position.z && new_position.z < z_coord+2*line_width))
                    if(x_coord-wall_width  < new_position.x && new_position.x < x_coord)
                    {
                        current_state=0;
                    }
            }
            if (grid_position_x!=0 && grid_position_x!=num_of_cells)
                line_index++;
        }
    }

    return current_state;
}

int check_sensor_ray_collision(glm::vec3 new_position)
{
    int current_state=1;
    float x_coord, z_coord;
    int i,j, grid_position_x, grid_position_z;
    int line_index;
    for (j=-4;j<=4; j++)
    {
        for (i=0;i<=1; i++) {
            grid_position_x=(int)((mouse_position.x-maze_offset)/line_width)/2+j;
            grid_position_z=(int)((mouse_position.z-maze_offset)/line_width)/2+i;
            line_index=(grid_position_x+1)*((int)num_of_cells-1)+1-grid_position_z;
            if (grid_position_z==0 || grid_position_z==num_of_cells || lines_array[line_index]=='1')
            {
                x_coord=maze_offset+grid_position_x*2*line_width;
                z_coord=maze_offset+grid_position_z*2*line_width;
                if ((x_coord< new_position.x && new_position.x < x_coord+2*line_width))
                    if(z_coord-wall_width  < new_position.z && new_position.z < z_coord)
                    {
                        current_state=0;
                    }
            }
        }
    }
    for (j=-4;j<=4; j++)
    {
        for (i=0;i<=1; i++) {
            grid_position_x=(int)((mouse_position.x-maze_offset)/line_width)/2+i;
            grid_position_z=(int)((mouse_position.z-maze_offset)/line_width)/2+j;
            line_index=(int)(num_of_cells*(num_of_cells-1))+(grid_position_x)*(int)num_of_cells-grid_position_z;
            if (grid_position_x==0 || grid_position_x==num_of_cells || lines_array[line_index]=='1')
            {
                x_coord=maze_offset+grid_position_x*2*line_width;
                z_coord=maze_offset+grid_position_z*2*line_width;
                if ((z_coord < new_position.z && new_position.z < z_coord+2*line_width))
                    if(x_coord-wall_width  < new_position.x && new_position.x < x_coord)
                    {
                        current_state=0;
                    }
            }
        }
    }
    
    
    return current_state;
    
}


void check_proximities()
{
    
    
    float i=0;
    int hit_wall=1;
    glm::vec3 right_sensor_position= mouse_position + mouse_forward_vector*0.5f;
    glm::vec3 right_sensor_vector= glm::rotate(mouse_forward_vector, -90.0f, glm::vec3(0.f, 1.f, 0.f));
    right_sensor_position+=right_sensor_vector*1.45f;
    
    for(i=0.0;i<line_width*500.0f&&hit_wall==1;i=i+0.1)
    {
        hit_wall=check_sensor_ray_collision2(right_sensor_position+right_sensor_vector*i);
    }
    if (hit_wall==1)
        right_proximity_reading=99.99;
    else
        right_proximity_reading=i;
    
    hit_wall=1;
    glm::vec3 left_sensor_position= mouse_position + mouse_forward_vector*0.5f;
    glm::vec3 left_sensor_vector= glm::rotate(mouse_forward_vector, 90.0f, glm::vec3(0.f, 1.f, 0.f));
    left_sensor_position+=left_sensor_vector*1.45f;
    for(i=0.0;i<line_width*500.0f&&hit_wall==1;i=i+0.1)
    {
        hit_wall=check_sensor_ray_collision2(left_sensor_position+left_sensor_vector*i);
    }
    if (hit_wall==1)
        left_proximity_reading=99.99;
    else
        left_proximity_reading=i;
    
    hit_wall=1;
    glm::vec3 top_left_sensor_position= mouse_position + mouse_forward_vector*0.8f;
    glm::vec3 top_left_sensor_vector= glm::rotate(mouse_forward_vector, 20.0f, glm::vec3(0.f, 1.f, 0.f));
    top_left_sensor_position+=top_left_sensor_vector*(0.7f+0.5f);
    for(i=0.0;i<line_width*500.0f&&hit_wall==1;i=i+0.1)
    {
        hit_wall=check_sensor_ray_collision2(top_left_sensor_position+top_left_sensor_vector*i);
    }
    if (hit_wall==1)
        top_left_proximity_reading=99.99;
    else
        top_left_proximity_reading=i;
    
    hit_wall=1;
    glm::vec3 top_right_sensor_position= mouse_position + mouse_forward_vector*0.8f;
    glm::vec3 top_right_sensor_vector= glm::rotate(mouse_forward_vector, -20.0f, glm::vec3(0.f, 1.f, 0.f));
    top_right_sensor_position+=top_right_sensor_vector*(0.7f+0.5f);
    for(i=0.0;i<line_width*500.0f&&hit_wall==1;i=i+0.1)
    {
        hit_wall=check_sensor_ray_collision2(top_right_sensor_position+top_right_sensor_vector*i);
    }
    if (hit_wall==1)
        top_right_proximity_reading=99.99;
    else
        top_right_proximity_reading=i;
    
}

void mouseFunction()
{
    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();
    
    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    deltaTime = float(currentTime - lastTime);
    // Get mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    
    if(first_run<3)
    {
        xpos=g_width/2;
        ypos=g_height/2;
        first_run++;
    }
    
    // Reset mouse position for next frame
    glfwSetCursorPos(window, g_width/2, g_height/2);
    
    // Compute new orientation
    horizontalAngle += mouseSpeed * float(g_width/2 - xpos );
    verticalAngle   -= mouseSpeed * float(g_height/2 - ypos );
    if(verticalAngle > 1.396)
        verticalAngle=1.396;
    if(verticalAngle < -1.396)
        verticalAngle=-1.396;
    
    direction=glm::vec3(
                        cos(verticalAngle) * cos(horizontalAngle),
                        sin(verticalAngle),
                        cos(verticalAngle) * cos(90.0 - horizontalAngle)
                        );
    
    // Right vector
    glm::vec3 up = glm::vec3(0.0f,1.0f,0.0f);
    
    
    
    glm::vec3 right = glm::cross( up, direction );
    
    if (glfwGetKey( window, GLFW_KEY_W ) == GLFW_PRESS){
        if(view_select==1)
            camera_position += direction * deltaTime * speed;
        else
            auto_move_camera-= direction * deltaTime * speed;
        
    }
    if (glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS){
        if(view_select==1)
            camera_position -= direction * deltaTime * speed;
        else
            auto_move_camera+= direction * deltaTime * speed;
    }
    if (glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS){
        if(view_select==1)
            camera_position -= right * deltaTime * speed * 5.0f;
        else
            auto_move_camera+= right * deltaTime * speed * 5.0f;
    }
    if (glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS){
        if(view_select==1)
            camera_position += right * deltaTime * speed * 5.0f;
        else
            auto_move_camera-= right * deltaTime * speed * 5.0f;
    }
    
    if (glfwGetKey( window, GLFW_KEY_UP ) == GLFW_PRESS){
        
        glm::vec3 new_position=mouse_position + mouse_forward_vector * deltaTime * speed * 2.5f;
        if(check_collision2(new_position)==1)
            mouse_position = new_position;
        if(sensor_on)
        {
            check_proximities();
            printf("l=%0.2f r=%0.2f ",left_proximity_reading, right_proximity_reading);
            printf("tl=%0.2f tr=%0.2f\n",top_left_proximity_reading, top_right_proximity_reading);
        }
}
    if (glfwGetKey( window, GLFW_KEY_RIGHT ) == GLFW_PRESS){
        mouse_rotate-=2;
        mouse_forward_vector = glm::normalize(glm::rotateY(mouse_forward_vector, -2.0f));
        if(sensor_on)
        {
            check_proximities();
            printf("l=%0.2f r=%0.2f ",left_proximity_reading, right_proximity_reading);
            printf("tl=%0.2f tr=%0.2f\n",top_left_proximity_reading, top_right_proximity_reading);
        }
    }
    if (glfwGetKey( window, GLFW_KEY_LEFT ) == GLFW_PRESS){
        mouse_rotate+=2;
        mouse_forward_vector = glm::normalize(glm::rotateY(mouse_forward_vector, 2.0f));
        if(sensor_on)
        {
            check_proximities();
            printf("l=%0.2f r=%0.2f ",left_proximity_reading, right_proximity_reading);
            printf("tl=%0.2f tr=%0.2f\n",top_left_proximity_reading, top_right_proximity_reading);
        }
    }
    if (glfwGetKey( window, GLFW_KEY_B ) == GLFW_PRESS){
        view_select=2;
    }
    if (glfwGetKey( window, GLFW_KEY_C ) == GLFW_PRESS){
        view_select=1;
    }
    if (glfwGetKey( window, GLFW_KEY_M ) == GLFW_PRESS){
        view_select=0;
    }
    if (glfwGetKey( window, GLFW_KEY_P ) == GLFW_PRESS){
        printf("%f %f\n", mouse_position.x/line_width/2, mouse_position.z/line_width/2);
    }
   
    
    if (glfwGetKey( window, GLFW_KEY_Y ) == GLFW_PRESS){
        floater += glm::vec3(0.0f,1.0f,0.0f) * deltaTime * speed*3.0f;
    }
    if (glfwGetKey( window, GLFW_KEY_H ) == GLFW_PRESS){
         floater -= glm::vec3(0.0f,1.0f,0.0f) * deltaTime * speed*3.0f;
    }
    if (glfwGetKey( window, GLFW_KEY_T ) == GLFW_PRESS){
        floater += glm::vec3(1.0f,0.0f,0.0f) * deltaTime * speed*3.0f;
    }
    if (glfwGetKey( window, GLFW_KEY_U ) == GLFW_PRESS){
        floater -= glm::vec3(1.0f,0.0f,0.0f) * deltaTime * speed*3.0f;
    }
    if (glfwGetKey( window, GLFW_KEY_G ) == GLFW_PRESS){
        floater += glm::vec3(0.0f,0.0f,1.0f) * deltaTime * speed*3.0f;
    }
    if (glfwGetKey( window, GLFW_KEY_J ) == GLFW_PRESS){
        floater -= glm::vec3(0.0f,0.0f,1.0f) * deltaTime * speed*3.0f;
    }
    if (glfwGetKey( window, GLFW_KEY_P ) == GLFW_PRESS){
        sensor_on=0;
    }
    if (glfwGetKey( window, GLFW_KEY_O ) == GLFW_PRESS){
        sensor_on=1;
        check_proximities();
        printf("l=%0.2f r=%0.2f ",left_proximity_reading, right_proximity_reading);
        printf("tl=%0.2f tr=%0.2f\n",top_left_proximity_reading, top_right_proximity_reading);
    }
    lastTime=currentTime;
}

void initMaze(const string &file_name)
{
    lines_array = GLSL::textFileRead(file_name.c_str());
    num_of_cells=(float)lines_array[0];
    side_length=40.0f*num_of_cells/16.0f;
    line_width=(float)side_length/num_of_cells;
    mouse_position = glm::vec3(maze_offset+line_width*(num_of_cells-0.5)*2,0.25f,maze_offset+line_width*(num_of_cells-0.5)*2);
    camera_position = glm::vec3( 63.2*num_of_cells/16.0, 58.0f*num_of_cells/16.0, 65.8f*num_of_cells/16.0 );
    g_light= glm::vec3(num_of_cells*2+5, 20,num_of_cells*2+5);
}

int bind_runtime_stuff(object *obj)
{
    GLSL::enableVertexAttribArray(h_aPosition);
    glBindBuffer(GL_ARRAY_BUFFER, obj->PosBufObj);
    glVertexAttribPointer(h_aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
    // Enable and bind normal array for drawing
    GLSL::enableVertexAttribArray(h_aNormal);
    glBindBuffer(GL_ARRAY_BUFFER, obj->NorBufObj);
    glVertexAttribPointer(h_aNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->IndBufObj);
    return (int)obj->shape[0].mesh.indices.size();
}
void drawGL()
{
    // Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Use our GLSL program
	glUseProgram(ShadeProg);
    
    glUniform3f(h_uLightPos, g_light.x, g_light.y, g_light.z);
    glUniform3f(h_LightCol, 1, 1, 1);
    
    // Create the projection matrix
    glm::mat4 Projection = glm::perspective(90.0f, (float)g_width/g_height, 0.1f, 150.f);
    glUniformMatrix4fv(h_uProjMatrix, 1, GL_FALSE, glm::value_ptr(Projection));
    
    mouseFunction();
    glm::mat4 View;
    if(view_select==0)
    {
        View = glm::lookAt(mouse_position, mouse_position+mouse_forward_vector, up);
    }
    else if(view_select==1)
        View = glm::lookAt(camera_position, camera_position+direction, up);
    else
        View = glm::lookAt(mouse_position+auto_move_camera, auto_move_camera+mouse_position+direction, up);;
    glUniformMatrix4fv(h_uViewMatrix, 1, GL_FALSE, glm::value_ptr(View));

    glUniform1i(wall, 0);
    //MOUSE
    
    //BODY
    int nIndices = bind_runtime_stuff(&body);
    ModelTrans.loadIdentity();
    ModelTrans.translate((mouse_position));
    ModelTrans.pushMatrix();
        ModelTrans.rotate(mouse_rotate, glm::vec3(0.0f,1.0f,0.0f));
        SetMaterial(1);
        glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
        glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
    
        //TIRES
        //left
        SetMaterial(2);
        ModelTrans.pushMatrix();
            ModelTrans.translate(((glm::vec3(0.27f,0.0f,0.55f))));
            ModelTrans.scale(0.5, 0.5, 0.5);
            nIndices = bind_runtime_stuff(&left_tire);
            glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
            glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
        ModelTrans.popMatrix();
    
        ModelTrans.pushMatrix();
            ModelTrans.translate(((glm::vec3(0.27f,0.0f,-0.55f))));
            ModelTrans.scale(0.5, 0.5, 0.5);
            nIndices = bind_runtime_stuff(&right_tire);
            glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
            glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
        ModelTrans.popMatrix();
    
        //SENSORS
        //left
        SetMaterial(4);
        ModelTrans.pushMatrix();
            ModelTrans.translate(((glm::vec3(-0.5f,0.0f,0.75f))));
            ModelTrans.rotate(90.0f, glm::vec3(0.0f,1.0f,0.0f));
            ModelTrans.pushMatrix();
                    ModelTrans.scale(0.1, 0.1, 0.1);
                    nIndices = bind_runtime_stuff(&sensor);
                    glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
                    glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
            ModelTrans.popMatrix();
            ModelTrans.translate(((glm::vec3(-0.8f,0.01f,0.0f))));
            ModelTrans.scale(0.7, 0.7, 0.7);
            nIndices = bind_runtime_stuff(&sensor_ray);
            glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
            if(sensor_on)
            glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
        ModelTrans.popMatrix();
    
        //right
        ModelTrans.pushMatrix();
            ModelTrans.translate(((glm::vec3(-0.5f,0.0f,-0.75f))));
            ModelTrans.rotate(-90.0f, glm::vec3(0.0f,1.0f,0.0f));
            ModelTrans.pushMatrix();
                ModelTrans.scale(0.1, 0.1, 0.1);
                nIndices = bind_runtime_stuff(&sensor);
                glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
                glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
            ModelTrans.popMatrix();
            ModelTrans.translate(((glm::vec3(-0.8f,0.01f,0.0f))));
            ModelTrans.scale(0.7, 0.7, 0.7);
            nIndices = bind_runtime_stuff(&sensor_ray);
            glUniform1i(wall, 3);
            glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
            if(sensor_on)
            glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
            glUniform1i(wall, 0);
        ModelTrans.popMatrix();
    
        //top-right
        ModelTrans.pushMatrix();
            ModelTrans.translate(((glm::vec3(-0.8f,0.0f,0.5f))));
            ModelTrans.rotate(20.0f, glm::vec3(0.0f,1.0f,0.0f));
            ModelTrans.pushMatrix();
                ModelTrans.scale(0.1, 0.1, 0.1);
                nIndices = bind_runtime_stuff(&sensor);
                glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
                glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
            ModelTrans.popMatrix();
            ModelTrans.translate(((glm::vec3(-0.8f,0.01f,0.0f))));
            ModelTrans.scale(0.7, 0.7, 0.7);
            nIndices = bind_runtime_stuff(&sensor_ray);
            glUniform1i(wall, 3);
            glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
            if(sensor_on)
            glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
            glUniform1i(wall, 0);
        ModelTrans.popMatrix();
    
        //top-left
        ModelTrans.pushMatrix();
            ModelTrans.translate(((glm::vec3(-0.8f,0.0f,-0.5f))));
            ModelTrans.rotate(-20.0f, glm::vec3(0.0f,1.0f,0.0f));
            ModelTrans.pushMatrix();
                ModelTrans.scale(0.1, 0.1, 0.1);
                nIndices = bind_runtime_stuff(&sensor);
                glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
                glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
            ModelTrans.popMatrix();
            ModelTrans.translate(((glm::vec3(-0.8f,0.01f,0.0f))));
            ModelTrans.scale(0.7, 0.7, 0.7);
            nIndices = bind_runtime_stuff(&sensor_ray);
            glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
            if(sensor_on)
            glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
        ModelTrans.popMatrix();
    ModelTrans.popMatrix();
    
    
    //BASE
    nIndices = bind_runtime_stuff(&cube);
    ModelTrans.loadIdentity();
    ModelTrans.translate(floater);
    ModelTrans.scale(0.2, 0.2, 0.2);
    glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
    glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
    //printf("%f %f %f \n", floater.x, floater.y, floater.z);
    
    ModelTrans.loadIdentity();
    ModelTrans.translate((glm::vec3(0.0f,-0.2f,0.0f)));
    ModelTrans.scale(side_length+maze_offset, 10.0, side_length+maze_offset);
    ModelTrans.translate((glm::vec3(1.0f,-1.0f,1.0f)));
    SetMaterial(1);
    glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
	glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
    SetMaterial(0);
    ModelTrans.loadIdentity();
    ModelTrans.scale(side_length+maze_offset, 0.2, side_length+maze_offset);
    ModelTrans.translate((glm::vec3(1.0f,-1.0f,1.0f)));
    glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
    glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
    
    
    
    //WALLS
    float i, j;
    int line_index=1;
    glUniform1i(wall, 1);
    for(j=0.0f;j<num_of_cells;j++)
    {
        for(i=num_of_cells;i>=0;i--)
        {
            if (i==0 || i==num_of_cells || lines_array[line_index]=='1') {
                SetMaterial(3);
                ModelTrans.loadIdentity();
                ModelTrans.translate((glm::vec3(maze_offset+j*2*line_width,0.0f,maze_offset+i*2*line_width)));
                ModelTrans.scale(line_width, wall_height, wall_width);
                ModelTrans.translate((glm::vec3(1.0f,1.0f,0.0f)));
                glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
                glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
                SetMaterial(3);
                ModelTrans.loadIdentity();
                ModelTrans.translate((glm::vec3(maze_offset+j*2*line_width,0,maze_offset+i*2*line_width)));
                ModelTrans.translate((glm::vec3(0,1.2,0)));
                ModelTrans.scale(line_width, 0.2, wall_width);
                ModelTrans.translate((glm::vec3(1,0,0)));
                glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
                glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
            }
            if (i!=0 && i!=num_of_cells)
                line_index++;
        }
    }

    for(i=0.0f;i<=num_of_cells;i++)
    {
        for(j=num_of_cells-1;j>=0;j--)
        {
             if (i==0 || i==num_of_cells || lines_array[line_index]=='1') {
                SetMaterial(3);
                ModelTrans.loadIdentity();
                ModelTrans.translate((glm::vec3(maze_offset+i*2*line_width,0.0f,maze_offset+j*2*line_width)));
                ModelTrans.scale(wall_width, wall_height, line_width);
                ModelTrans.translate((glm::vec3(0.0f,1.0f,1.0f)));
                glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
                glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
                SetMaterial(3);
                ModelTrans.loadIdentity();
                ModelTrans.translate((glm::vec3(maze_offset+i*2*line_width,0,maze_offset+j*2*line_width)));
                ModelTrans.translate((glm::vec3(0,1.2,0)));
                ModelTrans.scale(wall_width, 0.2, line_width);
                ModelTrans.translate((glm::vec3(0,0,1)));
                glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, glm::value_ptr(ModelTrans.modelViewMatrix));
                glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
             }
            if (i!=0 && i!=num_of_cells)
            line_index++;
        }
    }
    glUniform1i(wall, 0);
    //printf("%f %f %f \n", camera_position.x, camera_position.y, camera_position.z);
	// Disable and unbind
	GLSL::disableVertexAttribArray(h_aPosition);
	GLSL::disableVertexAttribArray(h_aNormal);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glUseProgram(0);
	assert(glGetError() == GL_NO_ERROR);
}

void reshapeGL(int w, int h)
{
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	g_width = w;
	g_height = h;
}
void *startServer(void *threadid)
{
    int s, s2, len;
    socklen_t t;
    struct sockaddr_un local, remote;
    char str[100];
    
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family)+1;
    if (bind(s, (struct sockaddr *)&local, len) == -1) {
        perror("bind");
        exit(1);
    }
//    int g=unlink("../../source/maze_resources/socke");
    if (listen(s, 5) == -1) {
        perror("listen");
        exit(1);
    }
    
    for(;;) {
        int done, n;
        printf("Waiting for a connection...\n");
        t = sizeof(remote);
        if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
            perror("accept");
            exit(1);
        }
        char temp_buf[100];
        sprintf(temp_buf, "Connected cells_in_row %2d corner 0 direction 1 detail_driver 1\n",
                (int)num_of_cells);
        send(s2, temp_buf, strlen(temp_buf), 0);
        float i;
        done = 0;
        do {
            n = recv(s2, str, 100, 0);
            if (n <= 0) {
                if (n < 0) perror("recv");
                done = 1;
            }
            char *command=(char *)calloc(100,sizeof(char) );
            char *response=(char *)calloc(100,sizeof(char) );
            //printf("%d \n",str);
            if (strcmp(strncpy(command, str, 6), "rotate")==0)
            {
                command[6]=0;
                float units=atof(strncpy(command, str+6, 10));
                float increment=units/100;
//                mouse_rotate+=units;
//                mouse_forward_vector = glm::normalize(glm::rotateY(mouse_forward_vector, units));

                for (i=0.0; i<=100; i++)
                {
                    mouse_rotate+=increment;
                    mouse_forward_vector = glm::normalize(glm::rotateY(mouse_forward_vector, increment));
                    usleep(1000);
                }
                
                response="rotated";
            }
            else if(strcmp(strncpy(command, str, 12), "move forward")==0)
            {
                command[12]=0;
                float units=atof(strncpy(command, str+12, 16));
                glm::vec3 new_position;
                for (i=0.0; i<=line_width*2.0f*units; i=i+0.1) {
                    new_position=mouse_position + glm::normalize(mouse_forward_vector)*0.1f;
                    if(check_collision2(new_position)==1)
                    {
                        mouse_position=new_position;
                                        usleep(10000);
                    }
                }
                response="Moved forward";
            }
            else if (strcmp(str, "wall to the right?\n")==0)
            {
                sensor_on=1;
                check_proximities();
//                sprintf(response, "%f", right_proximity_reading);
                if(right_proximity_reading<line_width)
                    response="1";
                else
                    response="0";
            }
            else if (strcmp(str, "wall to the left?\n")==0)
            {
                sensor_on=1;
                check_proximities();
                if(left_proximity_reading<line_width)
                    response="1";
                else
                    response="0";
            }
            else if (strcmp(str, "wall in front?\n")==0)
            {
                sensor_on=1;
                check_proximities();
                if(top_left_proximity_reading<line_width && top_right_proximity_reading < line_width)
                    response="1";
                else
                    response="0";
            }
            if (!done)
                if (send(s2, response, strlen(response), 0) < 0) {
                    perror("send");
                    done = 1;
                }
//            free(response);
            free(command);
        } while (!done);
        
        close(s2);
    }
    pthread_exit(NULL);

}

int main(int argc, char **argv)
{
    
    // Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_RESIZABLE,GL_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	g_width = 720;
	g_height = 720;
	// Open a window and create its OpenGL context
	window = glfwCreateWindow( g_width, g_height, "Final Project", NULL, NULL);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if( window == NULL )
	{
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
//    glfwSetKeyCallback(window, key_callback);
	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	loadShapes("../../source/obj_files/cube.obj", &cube.shape);
    loadShapes("../../source/obj_files/body.obj", &body.shape);
    loadShapes("../../source/obj_files/sensor.obj", &sensor.shape);
    loadShapes("../../source/obj_files/sensor_ray.obj", &sensor_ray.shape);
    loadShapes("../../source/obj_files/left_tire.obj", &left_tire.shape);
    loadShapes("../../source/obj_files/right_tire.obj", &right_tire.shape);
    
	std::cout << " loaded the object " << endl;

	// Initialize glad
	if (!gladLoadGL())
	{
		fprintf(stderr, "Unable to initialize glad");
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    initMaze("../../source/maze_resources/flood_fill.maze");
	initGL();
	installShaders("../../source/vert.glsl", "../../source/frag.glsl");
    
    pthread_t thread;
    pthread_create(&thread, NULL, startServer, (void *)1);
	do
	{
		drawGL();
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0 );

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
    pthread_exit(NULL);
	return 0;
}





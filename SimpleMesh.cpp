#include <GL/glew.h>

#include "SimpleMesh.h"
#include "objloader.hpp"
#include "final.h"
#include "PoolGame.h"
#include <vector>

namespace PoolGame
{


SimpleMesh::SimpleMesh(char *objFileName)
{
	// refactor into a "load model" method or something...
	loadOBJ(objFileName, vertices, uvs, normals); 

	glErrorCheck();

	glGenVertexArrays(1, &VAO_id); glErrorCheck();
	glBindVertexArray(VAO_id); glErrorCheck();

	glGenBuffers(1, &vertexbuffer); glErrorCheck();
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer); glErrorCheck();
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW); glErrorCheck();
	glEnableVertexAttribArray(0); glErrorCheck();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); glErrorCheck();

	glGenBuffers(1, &uvbuffer); glErrorCheck();
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer); glErrorCheck();
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW); glErrorCheck();
	glEnableVertexAttribArray(1); glErrorCheck();
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0); glErrorCheck();

	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);	
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	loaded = true;
}

SimpleMesh::SimpleMesh() : loaded(false)
{
}


SimpleMesh::~SimpleMesh(void)
{
}

void SimpleMesh::render( GLuint tex, glm::dmat4 &parent_model, glm::dmat4 &model_to_world_space )
{
	glEnable(GL_TEXTURE_2D);

	if (tex)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);		
	}

	glm::dmat4 model = parent_model * model_to_world_space;

	glm::mat4 MVP;
	MVP << (perspective * view * model);

	glm::mat4 MV;
	MV << (view * model);

	glm::mat4 M;
	M << model;

	glm::mat4 V;
	V << view;

	glm::mat3 N(V * M); // normal matrix	
	N = glm::transpose(glm::inverse(N)); // ref: http://www.arcsynthesis.org/gltut/Illumination/Tut09%20Normal%20Transformation.html	

	//NaNtest(perspective);
	//NaNtest(view);
	//NaNtest(model);
	//NaNtest(MVP);

	glm::vec4 test(0.0, 0.0, 0.0, 1.0);
	test = MVP * test;
	//printf("%lf, %lf, %lf, %lf\n", test[0], test[1], test[2], test[3]);

	glErrorCheck();

	glUseProgram(idProgram); glErrorCheck();

	glUniformMatrix4fv(idMVP, 1, 0, &MVP[0][0]); glErrorCheck();
	glUniformMatrix4fv(idMV, 1, 0, &MV[0][0]); glErrorCheck();
	glUniformMatrix3fv(idN, 1, 0, &N[0][0]); glErrorCheck();
	glUniform1i(idTex, 0); glErrorCheck();	

	glUniformMatrix4fv(glGetUniformLocation(idProgram, "M"), 1, 0, &M[0][0]); glErrorCheck();
	glUniformMatrix4fv(glGetUniformLocation(idProgram, "V"), 1, 0, &V[0][0]); glErrorCheck();

	glUniform4fv(glGetUniformLocation(idProgram, "sceneColor"), 1, mat_black); glErrorCheck();
	glUniform1f(glGetUniformLocation(idProgram, "ambient"), 0.3f); glErrorCheck();
	glUniform1f(glGetUniformLocation(idProgram, "diffuse"), 0.7f); glErrorCheck();
	glUniform1i(glGetUniformLocation(idProgram, "shininess"), 115); glErrorCheck();
	glUniform1f(glGetUniformLocation(idProgram, "specular"), 1.0f); glErrorCheck();
	glUniform4fv(glGetUniformLocation(idProgram, "lightPosition_worldspace[0]"), 1, light_position); glErrorCheck();
	glUniform4fv(glGetUniformLocation(idProgram, "light_ambient[0]"), 1, dim_ambiance); glErrorCheck();
	glUniform4fv(glGetUniformLocation(idProgram, "light_diffuse[0]"), 1, tungsten_100w); glErrorCheck();
	glUniform4fv(glGetUniformLocation(idProgram, "light_specular[0]"), 1, tungsten_100w); glErrorCheck();
	glUniform1f(glGetUniformLocation(idProgram, "light_quadraticAttenuation[0]"), 0.1f); glErrorCheck();
	glUniform3fv(glGetUniformLocation(idProgram, "eye_position"), 1, &eye[0]); glErrorCheck();


	glBindVertexArray(VAO_id); glErrorCheck();

	glEnableVertexAttribArray(0); glErrorCheck();
	//glBindBuffer(GL_ARRAY_BUFFER, PoolBall::vertexbuffer); glErrorCheck();
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); glErrorCheck();

	glEnableVertexAttribArray(1); glErrorCheck();
	//glEnableVertexAttribArray(2);

	glDrawArrays(GL_TRIANGLES, 0, vertices.size()); glErrorCheck();
}

}
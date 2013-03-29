#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

namespace PoolGame
{


class SimpleMesh
{
public:
	bool loaded;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	GLuint VAO_id, vertexbuffer, uvbuffer, normalbuffer;


	void render(GLuint tex, glm::dmat4 &parent_model, glm::dmat4 &model_to_world_space);

	SimpleMesh();
	SimpleMesh(char *objFileName);
	~SimpleMesh(void);
};

}
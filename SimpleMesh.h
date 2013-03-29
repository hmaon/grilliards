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
	std::vector<unsigned short> indices;

	GLuint VAO_id, vertexbuffer, uvbuffer, normalbuffer, elementbuffer;


	void render(GLuint tex, glm::dmat4 &model_to_world_space);
	void load(char *objFileName, glm::vec2 *texture_scaler = nullptr);

	SimpleMesh();
	SimpleMesh(char *objFileName);
	~SimpleMesh(void);
};

}

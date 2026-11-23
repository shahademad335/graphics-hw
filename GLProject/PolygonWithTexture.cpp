#include <glad/glad.h>
#include <vector>
#include <learnopengl/shader_m.h>
#include <glm/glm.hpp>

class PolygonWithTexture
{
public:
	PolygonWithTexture(std::vector<glm::vec3> v, std::vector<glm::vec2> t, glm::vec3 c = glm::vec3(1.0f, 1.0f, 1.0f))
	{
		for (int i = 0; i < v.size(); i++)
		{
			for (int j = 0; j < v[i].length(); j++)
				vertices.push_back(v[i][j]);
			for (int j = 0; j < t[i].length(); j++)
				vertices.push_back(t[i][j]);
		}
		color = c;
		model = glm::mat4(1.0f);
		texture = 0;

		GLuint VBO, VAO;
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glGenBuffers(1, &VBO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
		glEnableVertexAttribArray(2); 
		this->VAO = VAO;
	}

	void transformation(glm::mat4 t)
	{
		model = t;
	}

	void setTexture(GLuint &t)
	{
		texture = t;
	}

	void setColor(glm::vec3 c)
	{
		color = c;
	}

	void draw(Shader &shader)
	{
		shader.setVec3("objectColor", color);
		shader.setMat4("model", model);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(this->VAO);
		glDrawArrays(GL_TRIANGLE_FAN, 0, vertices.size() / 5);
	}

	void deleteBuffers()
	{
		glDeleteVertexArrays(1, &VAO);
	}

private:
	std::vector<float> vertices;
	glm::vec3 color;
	glm::mat4 model;
	GLuint texture;
	GLuint VAO;
	PolygonWithTexture();
};

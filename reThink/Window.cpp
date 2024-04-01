#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shaders/Impl/Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;


int main() {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

	//创建窗口

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT , "LearnOpenGLTest", NULL, NULL);
	if (window == NULL) {
		std::cout << "GLFW window创建失败" << std::endl;
		glfwTerminate();
		return -1;
	}

	//设置窗口上下文
	glfwMakeContextCurrent(window);
	//当窗口的帧缓冲大小发生变化时，自动调用framebuffer_size_callback，传给它现在的宽高值
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	
	//检测GLAD初始化
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "GLAD初始化失败" << '\n';
		return -1;
	}

	Shader testShader(R"(D:\Projects\TA\OpenGLProjects\reThink\reThink\Shaders\shader.vs)",
	                  R"(D:\Projects\TA\OpenGLProjects\reThink\reThink\Shaders\shader.fs)");

	float vertices[] = {
		// 位置         // 颜色
		 0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // 右下角
		-0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // 左下角
		 0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // 顶部 
	};

	//创建VAO,VBO
	unsigned int VAO,VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	//绑定VAO,VBO
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),vertices,GL_STATIC_DRAW);

	//位置信息
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//颜色信息
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glm::mat4 mat = glm::mat4(1.0f);

	//主循环
	while (!glfwWindowShouldClose(window)) {

		processInput(window);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		testShader.use();
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	
	glfwTerminate();
	return 0;

}


//输入ESC退出
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window,GLFW_KEY_ESCAPE) == GLFW_PRESS){
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS){
		
	}
}


//依据新的窗口宽高值更改视口大小
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}
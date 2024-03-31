#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);



float vertices[] = {
	-0.5f,-0.5f,0.0f,
	0.5f,-0.5f,0.0f,
	0.0f,0.5f,0.0f
};

int main() {
	system("chcp 65001");
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

	//创建VBO
	unsigned int VBO;
	glGenBuffers(1, &VBO);
	//绑定VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),vertices,GL_STATIC_DRAW);

	//创建顶点着色器对象
	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);

	//将顶点着色器传给这个对象
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);


	//创建着色器程序对象
	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram,vertexShader)

	//创建窗口

	GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGLTest", NULL, NULL);
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
		std::cout << "GLAD初始化失败" << std::endl;
		return -1;
	}

	//设置视口
	glViewport(0, 0, 800, 600);

	

	
	//主循环
	while (!glfwWindowShouldClose(window)) {
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;

}
//依据新的窗口宽高值更改视口大小
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}
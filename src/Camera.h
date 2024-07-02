#include <vulkan/vulkan.h>
#include <glm/ext.hpp>
#include <GLFW/glfw3.h>

#include <iostream>

class Camera {
public:
	Camera();

	glm::mat4 getVP() {return vp;}
	glm::mat4 getEnvVP() {return envvp;}
	glm::vec3 getCartesianPosition();

	void setR(float r2);
	void setTheta(float theta2);
	void setPhi(float phi2);
	void addR(float r2);
	void addTheta(float theta2);
	void addPhi(float phi2);

	void update(GLFWwindow* w, float dt);

private:
	float r, dr, theta, dtheta, phi, dphi, nearclip, farclip, aspect, fovy, speed;
	glm::mat4 vp, envvp;

	glm::mat4 updateVP();
};

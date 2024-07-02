#include "Camera.h"

Camera::Camera() {
	r = 1000.;
	dr = 0.;
	theta = -0.9;
	dtheta = 0.;
	phi = -0.785;
	dphi = 0.;
	
	nearclip = 0.1;
	farclip = 10000.;
	// TODO: pass in aspect from gh
	aspect = 16./9.;
	fovy = 0.785;

	speed = 2.;

	updateVP();
}

void Camera::setR(float r2) {
	r = r2;
	updateVP();
}

void Camera::setTheta(float theta2) {
	theta = theta2;
	updateVP();
}

void Camera::setPhi(float phi2) {
	phi = phi2;
	updateVP();
}

void Camera::addR(float r2) {
	r += r2;
	updateVP();
}

void Camera::addTheta(float theta2) {
	theta += theta2;
	updateVP();
}

void Camera::addPhi(float phi2) {
	phi += phi2;
	updateVP();
}

void Camera::update(GLFWwindow* w, float dt) {
	if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) dphi = speed * dt;
	else if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) dphi = -speed * dt;
	else if (glfwGetKey(w, GLFW_KEY_D) == GLFW_RELEASE) dphi = 0.;
	else if (glfwGetKey(w, GLFW_KEY_A) == GLFW_RELEASE) dphi = 0.;

	if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) dtheta = speed * dt;
	else if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) dtheta = -speed * dt;
	else if (glfwGetKey(w, GLFW_KEY_W) == GLFW_RELEASE) dtheta = 0.;
	else if (glfwGetKey(w, GLFW_KEY_S) == GLFW_RELEASE) dtheta = 0.;

	if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) dr = r * speed * dt;
	else if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) dr = r * -speed * dt;
	else if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_RELEASE) dr = 0.;
	else if (glfwGetKey(w, GLFW_KEY_E) == GLFW_RELEASE) dr = 0.;

	if (glfwGetKey(w, GLFW_KEY_TAB) == GLFW_PRESS) theta = acos(2. / r);
	
	phi += dphi;
	theta += dtheta;
	r += dr;
	updateVP();
}

glm::mat4 Camera::updateVP() {
	vp = glm::perspective<float>(fovy, aspect, nearclip, farclip);
	vp[1][1] *= -1.;
	envvp = vp;
	vp *= glm::lookAt<float>(
		getCartesianPosition(),
		glm::vec3(0.), 
		glm::vec3(0., 1., 0.));
	envvp *= glm::lookAt<float>(
		glm::vec3(0),
		-getCartesianPosition(),
		glm::vec3(0, 1, 0));
	return vp;
}

glm::vec3 Camera::getCartesianPosition() {
	return glm::vec3(
		r * sin(theta) * sin(phi), 
		r * cos(theta),
		r * sin(theta) * cos(phi)
	);
}

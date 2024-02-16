#include "Camera.h"

Camera::Camera() {
	r = 50.;
	theta = -0.9;
	phi = 0.;
	
	nearclip = 0.1;
	farclip = 100.;
	// TODO: pass in aspect from gh
	aspect = 16./9.;
	fovy = 0.785;

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

glm::mat4 Camera::updateVP() {
	vp = glm::perspective<float>(fovy, aspect, nearclip, farclip);
	vp[1][1] *= -1.;
	vp *= glm::lookAt<float>(
		getCartesianPosition(),
		glm::vec3(0.), 
		glm::vec3(0., 1., 0.));
	return vp;
}

glm::vec3 Camera::getCartesianPosition() {
	return glm::vec3(
		r * sin(theta) * sin(phi), 
		r * cos(theta),
		r * sin(theta) * cos(phi)
	);
}

#include <glm/ext.hpp>

class Camera {
public:
	Camera();

	glm::mat4 getVP() {return vp;}
	glm::vec3 getCartesianPosition();

	void setR(float r2);
	void setTheta(float theta2);
	void setPhi(float phi2);
	void addR(float r2);
	void addTheta(float theta2);
	void addPhi(float phi2);


private:
	float r, theta, phi, nearclip, farclip, aspect, fovy;
	glm::mat4 vp;

	glm::mat4 updateVP();
};

#include "UI.h"
#include "GraphicsHandler.h"

class UIHandler {
public:
	UIHandler() : gh(nullptr), uicb(VK_NULL_HANDLE), window(nullptr) {}
	UIHandler(GH* g, VkCommandBuffer c, GLFWwindow* w);
	~UIHandler();

	void draw();

	void setCommandBuffer(VkCommandBuffer c) {uicb = c;}

	void addRoot(UIComponent* c) {roots.push_back(c);}

	std::vector<UIComponent*> getRoots() {return roots;}

private:
	std::vector<UIComponent*> roots;
	GH* gh;
	VkCommandBuffer uicb;
	GLFWwindow* window;
	VkSampler textsampler;

	static vec2 glfwCoordsToPixels(GLFWwindow* w, double x, double y);

};

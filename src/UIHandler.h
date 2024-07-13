#include "UI.h"
#include "GraphicsHandler.h"

class UIHandler {
public:
	UIHandler() : gh(nullptr), uicb(VK_NULL_HANDLE) {}
	UIHandler(GH* g, VkCommandBuffer c);
	~UIHandler();

	void draw();

	void setCommandBuffer(VkCommandBuffer c) {uicb = c;}

private:
	std::vector<UIComponent*> roots;
	GH* gh;
	VkCommandBuffer uicb;

};

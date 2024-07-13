#include <string>
#include <vector>
#include <iostream>
#include <glm/ext.hpp>
#include <vulkan/vulkan.hpp>

using namespace glm;

class UIComponent;

typedef std::function<void (UIComponent*)> dfType;

typedef struct UIPipelineInfo {
	VkPipelineLayout layout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
	VkDescriptorSetLayoutCreateInfo descsetlayoutci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
	};
	VkPushConstantRange pushconstantrange = {};
	VkSpecializationInfo specinfo = {};
} UIPipelineInfo;

typedef struct UIPushConstantData {
	// these fields are in pixels (although sub-pixel values should still compute correctly)
	vec2 position = vec2(0), extent = vec2(0);
} UIPushConstantData;

class UIComponent {
public:
	UIComponent() : renderpriority(0), drawFunc(defaultDrawFunc) {}
	UIComponent(vec2 p, vec2 e, uint8_t rp) : pcdata({p, e}), renderpriority(rp), drawFunc(defaultDrawFunc) {}

	virtual std::vector<UIComponent*> getChildren() = 0;

	void draw();

	static void setGraphicsPipeline(UIPipelineInfo p) {graphicspipeline = p;}
	static UIPipelineInfo getGraphicsPipeline() {return graphicspipeline;}
	static void setDefaultDrawFunc(dfType ddf) {defaultDrawFunc = ddf;}
	UIPushConstantData* getPCDataPtr() {return &pcdata;}
	void setPos(vec2 p) {pcdata.position = p;}
	void setExt(vec2 e) {pcdata.extent = e;}

private:
	UIPushConstantData pcdata;
	// TODO: actually make renderpriority work lol
	uint8_t renderpriority; // kinda like z-index, but only in relation to siblings (parent always lower)
	dfType drawFunc;
	static UIPipelineInfo graphicspipeline;
	static dfType defaultDrawFunc;

};

class UIText : public UIComponent {
public:
	UIText() : text(L"") {}
	UIText(std::wstring t) : text(t) {}

	std::vector<UIComponent*> getChildren();

	void* generateTexture();

private:
	std::wstring text;
};

class UIButton : public UIComponent {
public:
	UIButton() : onclick(nullptr) {}

	std::vector<UIComponent*> getChildren();

private:
	UIText text;
	std::function<void (void*)> onclick;
};

class UIDropdown : public UIComponent {
public:
	UIDropdown() = default;

	std::vector<UIComponent*> getChildren();

private:
	std::vector<UIButton> options;
};

class UIDropdownButtons : public UIDropdown {
public:
	UIDropdownButtons() = default;
	UIDropdownButtons(std::wstring t) : title(t), UIDropdown() {}

private:
	UIText title;
};

class UIDropdownSelector : public UIDropdown {
public:
	UIDropdownSelector() : selected(nullptr) {}

private:
	UIText* selected;
};

class UIRibbon : public UIComponent {
public:
	UIRibbon() = default;

	std::vector<UIComponent*> getChildren();

	void addOption(std::wstring name);

private:
	std::vector<UIDropdownButtons> options;
};

#include <string>
#include <vector>
#include <iostream>
#include <glm/ext.hpp>
#include <vulkan/vulkan.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#define UI_DEFAULT_SANS_FILEPATH "/System/Library/fonts/Avenir Next.ttc"
#define UI_DEFAULT_SANS_IDX 2
#define UI_DEFAULT_BG_COLOR vec4(0.3, 0.3, 0.3, 1)
#define UI_DEFAULT_HOVER_BG_COLOR vec4(0.4, 0.4, 0.4, 1)


using namespace glm;

class UIComponent;

class UIText;

typedef unsigned char unorm;

typedef std::function<void (UIComponent*)> dfType;

typedef std::function<void (UIText*, unorm*)> tfType;

typedef std::function<void (UIText*)> tdfType;

typedef std::function<void (UIComponent*, void*)> cfType;

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

typedef struct UIImageInfo {
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkExtent2D extent = {0, 0};
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
} UIImageInfo;

typedef struct UIPushConstantData {
	// these fields are in pixels (although sub-pixel values should still compute correctly)
	vec2 position = vec2(0), extent = vec2(0);
	vec4 bgcolor = vec4(0.3, 0.3, 0.3, 1);
} UIPushConstantData;

typedef uint8_t UIEventFlags;

typedef enum UIEventFlagBits {
	UI_EVENT_FLAG_NONE = 0x00,
	UI_EVENT_FLAG_HOVER = 0x01,
	UI_EVENT_FLAG_CLICK = 0x02
} UIEventFlagBits;

class UIComponent {
public:
	UIComponent() : 
		drawFunc(defaultDrawFunc), 
		onHover(defaultOnHover),
		onHoverBegin(defaultOnHoverBegin),
		onHoverEnd(defaultOnHoverEnd),
		onClick(defaultOnClick),
		onClickBegin(defaultOnClickBegin),
		onClickEnd(defaultOnClickEnd),
		ds(defaultds), 
		events(UI_EVENT_FLAG_NONE) {}
	UIComponent(vec2 p, vec2 e) : 
		pcdata({p, e, UI_DEFAULT_BG_COLOR}), 
		drawFunc(defaultDrawFunc), 
		onHover(defaultOnHover),
		onHoverBegin(defaultOnHoverBegin),
		onHoverEnd(defaultOnHoverEnd),
		onClick(defaultOnClick),
		onClickBegin(defaultOnClickBegin),
		onClickEnd(defaultOnClickEnd),
		ds(defaultds),
		events(UI_EVENT_FLAG_NONE) {}
	UIComponent(const UIComponent& rhs) :
		pcdata(rhs.pcdata),
		drawFunc(rhs.drawFunc),
		onHover(rhs.onHover),
		onHoverBegin(rhs.onHoverBegin),
		onHoverEnd(rhs.onHoverEnd),
		onClick(rhs.onClick),
		onClickBegin(rhs.onClickBegin),
		onClickEnd(rhs.onClickEnd),
		ds(rhs.ds),
		events(rhs.events) {}
	UIComponent(UIComponent&& rhs) noexcept;


	virtual std::vector<UIComponent*> getChildren() = 0;

	void draw();
	void listenMousePos(vec2 mousepos, void* data);
	void listenMouseClick(bool click, void* data);

	static void setGraphicsPipeline(UIPipelineInfo p) {graphicspipeline = p;}
	static UIPipelineInfo getGraphicsPipeline() {return graphicspipeline;}
	static void setNoTex(UIImageInfo i) {notex = i;}
	static UIImageInfo getNoTex() {return notex;}
	static void setDefaultDS(VkDescriptorSet d) {defaultds = d;}
	static void setDefaultDrawFunc(dfType ddf) {defaultDrawFunc = ddf;}
	static void setScreenExtent(vec2 e) {screenextent = e;}
	UIPushConstantData* getPCDataPtr() {return &pcdata;}
	// also changes position of children
	void setPos(vec2 p);
	vec2 getPos() {return pcdata.position;}
	void setExt(vec2 e) {pcdata.extent = e;}
	vec2 getExt() {return pcdata.extent;}
	void setDS(VkDescriptorSet d) {ds = d;}
	VkDescriptorSet* getDSPtr() {return &ds;}

protected:
	UIPushConstantData pcdata;
	static vec2 screenextent;

private:
	dfType drawFunc;
	cfType onHover, onHoverBegin, onHoverEnd,
		onClick, onClickBegin, onClickEnd;
	VkDescriptorSet ds;
	UIEventFlags events;
	static UIPipelineInfo graphicspipeline;
	static UIImageInfo notex;
	static VkDescriptorSet defaultds;
	static dfType defaultDrawFunc;
	static cfType defaultOnHover, defaultOnHoverBegin, defaultOnHoverEnd, 
			defaultOnClick, defaultOnClickBegin, defaultOnClickEnd;
};

class UIText : public UIComponent {
public:
	// Note: default constructor does not initialize the texture
	UIText();
	UIText(const UIText& rhs) :
		text(rhs.text),
		tex(rhs.tex),
		UIComponent(rhs) {}
	UIText(UIText&& rhs) noexcept;
	UIText(std::wstring t);
	UIText(std::wstring t, vec2 p); 
	~UIText();

	std::vector<UIComponent*> getChildren();
	UIImageInfo* getTexPtr() {return &tex;}
	void setText(std::wstring t);

	static void setTexLoadFunc(tfType tf) {texLoadFunc = tf;}
	static void setTexDestroyFunc(tdfType tdf) {texDestroyFunc = tdf;}

private:
	std::wstring text;
	UIImageInfo tex;

	void genTex();

	static FT_Library ft;
	static FT_Face typeface;
	static tfType texLoadFunc;
	static tdfType texDestroyFunc;
};

// TODO: remove this class
class UIButton : public UIComponent {
public:
	UIButton() : text(L"") {}
	UIButton(std::wstring t) : text(t) {}

	std::vector<UIComponent*> getChildren();

private:
	UIText text;

};

class UIDropdown : public UIComponent {
public:
	UIDropdown() = default;

	std::vector<UIComponent*> getChildren();

private:
	std::vector<UIText> options;
};

class UIDropdownButtons : public UIDropdown {
public:
	UIDropdownButtons() = default;
	UIDropdownButtons(std::wstring t);

	std::vector<UIComponent*> getChildren();

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
	UIRibbon();

	std::vector<UIComponent*> getChildren();

	void addOption(std::wstring name);

private:
	std::vector<UIDropdownButtons> options;
};

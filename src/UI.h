#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <glm/ext.hpp>
#include <vulkan/vulkan.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#define UI_DEFAULT_SANS_FILEPATH "/System/Library/fonts/Avenir Next.ttc"
#define UI_DEFAULT_SANS_IDX 2
#define UI_DEFAULT_BG_COLOR vec4(0.3, 0.3, 0.3, 1)
#define UI_DEFAULT_HOVER_BG_COLOR vec4(0.4, 0.4, 0.4, 1)
#define UI_DEFAULT_CLICK_BG_COLOR vec4(1, 0.4, 0.4, 1)

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
	UI_EVENT_FLAG_NONE =  0x00,
	UI_EVENT_FLAG_HOVER = 0x01,
	UI_EVENT_FLAG_CLICK = 0x02
} UIEventFlagBits;

typedef uint8_t UIDisplayFlags;

typedef enum UIDisplayFlagBits {
	UI_DISPLAY_FLAG_SHOW =                 0x01,
	UI_DISPLAY_FLAG_OVERFLOWING_CHILDREN = 0x02
} UIDisplayFlagBits;

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
		events(UI_EVENT_FLAG_NONE),
		display(UI_DISPLAY_FLAG_SHOW) {}
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
		events(UI_EVENT_FLAG_NONE),
		display(UI_DISPLAY_FLAG_SHOW) {}
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
		events(rhs.events),
		display(rhs.display) {}
	UIComponent(UIComponent&& rhs) noexcept;

	friend void swap(UIComponent& c1, UIComponent& c2);

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
	void setOnClickBegin(cfType f) {onClickBegin = f;}
	static void setScreenExtent(vec2 e) {screenextent = e;}
	UIPushConstantData* getPCDataPtr() {return &pcdata;}
	// also changes position of children
	void setPos(vec2 p);
	vec2 getPos() {return pcdata.position;}
	void setExt(vec2 e) {pcdata.extent = e;}
	vec2 getExt() {return pcdata.extent;}
	void setDS(VkDescriptorSet d) {ds = d;}
	VkDescriptorSet* getDSPtr() {return &ds;}
	void setDisplayFlag(UIDisplayFlags f) {display |= f;}
	void unsetDisplayFlag(UIDisplayFlags f) {display &= ~f;}

protected:
	UIPushConstantData pcdata;
	static vec2 screenextent;
	UIDisplayFlags display;

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
	UIText(const UIText& rhs);
	UIText(UIText&& rhs) noexcept;
	UIText(std::wstring t);
	UIText(std::wstring t, vec2 p); 
	~UIText();

	friend void swap(UIText& t1, UIText& t2);

	UIText& operator=(UIText rhs);
	/*
	UIText& operator=(const UIText&) = delete;
	UIText& operator=(UIText&&) = delete;
	*/

	std::vector<UIComponent*> getChildren();
	UIImageInfo* getTexPtr() {return &tex;}
	void setText(std::wstring t);
	const std::wstring& getText() {return text;}

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
	static std::map<VkImage, uint8_t> imgusers;
};

class UIDropdown : public UIComponent {
public:
	UIDropdown() = default;
	UIDropdown(const UIDropdown& rhs) :
		options(rhs.options),
		UIComponent(rhs) {}
	UIDropdown(UIDropdown&& rhs) noexcept;
	UIDropdown(std::vector<std::wstring> o);

	std::vector<UIComponent*> getChildren();

private:
	std::vector<UIText> options;
};

class UIDropdownButtons : public UIDropdown {
public:
	UIDropdownButtons() = default;
	UIDropdownButtons(const UIDropdownButtons& rhs) :
		title(rhs.title),
		UIDropdown(rhs) {}
	UIDropdownButtons(UIDropdownButtons&& rhs) noexcept;
	UIDropdownButtons(std::wstring t);
	UIDropdownButtons(std::wstring t, std::vector<std::wstring> o);

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
	void addOption(UIDropdownButtons&& o);
	void addOption(std::wstring t, std::vector<std::wstring> o);
	// TODO: get rid of this function, should be using getChildren() instead
	std::vector<UIDropdownButtons> getOptions() {return options;}

private:
	std::vector<UIDropdownButtons> options;
};

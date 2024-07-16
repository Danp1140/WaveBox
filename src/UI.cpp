#include "UI.h"

/* 
 * ---------------
 * | UIComponent |
 * ---------------
 */

// -- Public --

vec2 UIComponent::screenextent = vec2(0);
UIPipelineInfo UIComponent::graphicspipeline = {};
UIImageInfo UIComponent::notex = {};
VkDescriptorSet UIComponent::defaultds = VK_NULL_HANDLE;
dfType UIComponent::defaultDrawFunc = nullptr;

UIComponent::UIComponent(UIComponent&& rhs) noexcept :
		pcdata(rhs.pcdata),
		drawFunc(rhs.drawFunc),
		onHover(rhs.onHover),
		onHoverBegin(rhs.onHoverBegin),
		onHoverEnd(rhs.onHoverEnd),
		onClick(rhs.onClick),
		onClickBegin(rhs.onClickBegin),
		onClickEnd(rhs.onClickEnd),
		ds(rhs.ds),
		events(rhs.events) {
	rhs.pcdata = (UIPushConstantData){};
	rhs.drawFunc = nullptr;
	rhs.onHover = nullptr;
	rhs.onHoverBegin = nullptr;
	rhs.onHoverEnd = nullptr;
	rhs.onClick = nullptr;
	rhs.onClickBegin = nullptr;
	rhs.onClickEnd = nullptr;
	rhs.ds = VK_NULL_HANDLE;
	rhs.events = UI_EVENT_FLAG_NONE;
}

void UIComponent::draw() {
	drawFunc(this);
	for (UIComponent* c : getChildren()) {
		c->draw();
	}
}

void UIComponent::listenMousePos(vec2 mousepos, void* data) {
	if (mousepos.x > this->getPos().x
		&& mousepos.y > this->getPos().y
		&& mousepos.x < this->getPos().x + this->getExt().x
		&& mousepos.y < this->getPos().y + this->getExt().y
		) {
		onHover(this, nullptr);
		if (!(events & UI_EVENT_FLAG_HOVER)) {
			onHoverBegin(this, nullptr);
			events |= UI_EVENT_FLAG_HOVER;
		}
		for (UIComponent* c : getChildren()) c->listenMousePos(mousepos, data);
	} else if (events & UI_EVENT_FLAG_HOVER) {
		onHoverEnd(this, nullptr);
		events &= ~UI_EVENT_FLAG_HOVER;
		for (UIComponent* c : getChildren()) c->listenMousePos(mousepos, data);
	}
}

void UIComponent::listenMouseClick(bool click, void* data) {
	if ((events & UI_EVENT_FLAG_HOVER) && click) {
		onClick(this, nullptr);
		if (!(events & UI_EVENT_FLAG_CLICK)) {
			onClickBegin(this, nullptr);
			events |= UI_EVENT_FLAG_CLICK;
		}
		for (UIComponent* c : getChildren()) c->listenMouseClick(click, data);
	} else if (events & UI_EVENT_FLAG_CLICK) {
		onClickEnd(this, nullptr);
		events &= ~UI_EVENT_FLAG_CLICK;
		for (UIComponent* c : getChildren()) c->listenMouseClick(click, data);
	}
}

void UIComponent::setPos(vec2 p) {
	vec2 diff = p - pcdata.position;
	pcdata.position = p;
	for (UIComponent* c : getChildren()) c->setPos(c->getPos() + diff);
}

// -- Private --

cfType UIComponent::defaultOnHover = [] (UIComponent* self, void* d) {};
cfType UIComponent::defaultOnHoverBegin = [] (UIComponent* self, void* d) {};
cfType UIComponent::defaultOnHoverEnd = [] (UIComponent* self, void* d) {};
cfType UIComponent::defaultOnClick = [] (UIComponent* self, void* d) {};
cfType UIComponent::defaultOnClickBegin = [] (UIComponent* self, void* d) {};
cfType UIComponent::defaultOnClickEnd = [] (UIComponent* self, void* d) {};

/* 
 * ----------
 * | UIText |
 * ----------
 */

// -- Public --

std::vector<UIComponent*> UIText::getChildren() {return {};}

void UIText::setText(std::wstring t) {
	text = t;
	genTex();
}

// -- Private --

FT_Library UIText::ft = nullptr;
FT_Face UIText::typeface = nullptr;
tfType UIText::texLoadFunc = nullptr; 
tdfType UIText::texDestroyFunc = nullptr;

UIText::UIText() : text(L""), tex({}) {
	if (!ft) {
		ft = FT_Library();
		FT_Init_FreeType(&ft);
	}
	if (!typeface) {
		FT_New_Face(ft, UI_DEFAULT_SANS_FILEPATH, UI_DEFAULT_SANS_IDX, &typeface); 
	}
	tex = {};
}

UIText::UIText(UIText&& rhs) noexcept :
	text(rhs.text),
	tex(rhs.tex),
	UIComponent(rhs) {
	rhs.text = L"";
	rhs.tex = (UIImageInfo){};
}

UIText::UIText(std::wstring t) : UIText() {
	text = t;
	genTex();
}

UIText::UIText(std::wstring t, vec2 p) : UIComponent(p, vec2(0)) {
	// TODO: figure out how to call this other constructor
	tex = {};
	text = t;
	genTex();
}

UIText::~UIText() {
	texDestroyFunc(this);
}

void UIText::genTex() {
	const uint32_t pixelscale = 64;
	// FT_Set_Char_Size(typeface, 0, 50 * pixelscale, 0, 0);
	FT_Set_Pixel_Sizes(typeface, 0, 32);
	uint32_t maxlinelength = 0, linelengthcounter = 0, numlines = 1;
	for (char c : text) {
		if (c == '\n') {
			if (linelengthcounter > maxlinelength) maxlinelength = linelengthcounter;
			linelengthcounter = 0;
			numlines++;
			continue;
		}
		FT_Load_Char(typeface, c, FT_LOAD_DEFAULT);
		linelengthcounter += typeface->glyph->metrics.horiAdvance;
	}
	if (linelengthcounter > maxlinelength) maxlinelength = linelengthcounter;

	FT_Size_Metrics m = typeface->size->metrics;
	const uint32_t hres = maxlinelength / pixelscale, vres = numlines * m.height / pixelscale;
	// TODO: switch all hres, vres to this extent
	tex.extent = {hres, vres};
	unorm* texturedata = (unorm*)malloc(hres * vres * sizeof(float));
	memset(&texturedata[0], 0.0f, hres * vres * sizeof(float));
	// TODO: should this be int or float?
	ivec2 penposition = glm::ivec2(0, vres - m.ascender / pixelscale);

	FT_Glyph_Metrics gm;
	for (char c : text) {
		if (c == '\n') {
			penposition.y -= m.height / pixelscale;
			penposition.x = 0;
			continue;
		}
		FT_Load_Char(typeface, c, FT_LOAD_RENDER);
		if (typeface->glyph) FT_Render_Glyph(typeface->glyph, FT_RENDER_MODE_NORMAL);
		gm = typeface->glyph->metrics;
		penposition += vec2(gm.horiBearingX, gm.horiBearingY) / pixelscale;
		unsigned char* bitmapbuffer = typeface->glyph->bitmap.buffer;
		uint32_t xtex = 0, ytex = 0;
		for (uint32_t y = 0; y < typeface->glyph->bitmap.rows; y++) {
			for (uint32_t x = 0; x < typeface->glyph->bitmap.width; x++) {
				xtex = (uint32_t)penposition.x + x;
				ytex = (uint32_t)penposition.y - y;
				texturedata[ytex * hres + xtex] = *bitmapbuffer++;
			}
		}
		penposition += vec2(gm.horiAdvance - gm.horiBearingX, -gm.horiBearingY) / pixelscale;
	}

	pcdata.extent = vec2(hres, vres);

	texLoadFunc(this, texturedata);
}

/* 
 * --------------
 * | UIDropdown |
 * --------------
 */

// -- Public --

std::vector<UIComponent*> UIDropdown::getChildren() {
	std::vector<UIComponent*> result = {};
	for (size_t i = 0; i < options.size(); i++) result.push_back(&options[i]);
	return result;
}

// -- Private --


/* 
 * ---------------------
 * | UIDropdownButtons |
 * ---------------------
 */

// -- Public --

UIDropdownButtons::UIDropdownButtons(std::wstring t) : title(t), UIDropdown() {
	this->setExt(title.getExt());
}

std::vector<UIComponent*> UIDropdownButtons::getChildren() {
	std::vector<UIComponent*> result = UIDropdown::getChildren();
	result.insert(result.end(), &title);
	return result;
}

// -- Private --


/* 
 * ----------------------
 * | UIDropdownSelector |
 * ----------------------
 */

// -- Public --

// -- Private --


/* 
 * ------------
 * | UIRibbon |
 * ------------
 */

// -- Public --

UIRibbon::UIRibbon() : UIComponent(), options({}) {
	setPos(vec2(0, screenextent.y - 50));
	setExt(vec2(screenextent.x, 50));
}

std::vector<UIComponent*> UIRibbon::getChildren() {
	std::vector<UIComponent*> result;
	for (size_t i = 0; i < options.size(); i++) result.push_back(&options[i]);
	return result;
}

void UIRibbon::addOption(std::wstring name) {
	float xlen = options.size() ? options.back().getPos().x + options.back().getExt().x : 0;
	options.emplace_back(name);
	options.back().setPos(vec2(50 + xlen, this->getPos().y));
	options.back().setExt(options.back().getExt() + vec2(50, 0));
}

// -- Private --

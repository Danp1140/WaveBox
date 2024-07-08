#include "Drawable.h"

using namespace glm;

class UIComponent {
public:
	UIComponent() : position(0, 0), extent(0, 0) {}
	UIComponent(vec2 p, vec2 e) : position(p), extent(e) {}

	virtual std::vector<UIComponent*> getChildren() = 0;

private:
	vec2 position, extent; // these fields are in uv coords on a quad covering the screen (bottom left origin)
};

class UIText : public UIComponent {
public:
	UIText() : text(L"") {}

	std::vector<UIComponent*> getChildren();

private:
	std::wstring text;
};

class UIButton : public UIComponent {
public:
	UIButton() : onclick(nullptr) {}

	std::vector<UIComponent*> getChildren();

private:
	UIText text;
	std::function<void ()> onclick;
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

private:
	std::vector<UIDropdown> options;
};

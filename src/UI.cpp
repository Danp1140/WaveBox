#include "UI.h"

/* 
 * ---------------
 * | UIComponent |
 * ---------------
 */

// -- Public --

// -- Private --


/* 
 * ----------
 * | UIText |
 * ----------
 */

// -- Public --

std::vector<UIComponent*> UIText::getChildren() {return {};}

// -- Private --


/* 
 * ------------
 * | UIButton |
 * ------------
 */

// -- Public --

std::vector<UIComponent*> UIButton::getChildren() {return {&text};}

// -- Private --


/* 
 * --------------
 * | UIDropdown |
 * --------------
 */

// -- Public --

std::vector<UIComponent*> UIDropdown::getChildren() {
	std::vector<UIComponent*> result;
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

std::vector<UIComponent*> UIRibbon::getChildren() {
	std::vector<UIComponent*> result;
	for (size_t i = 0; i < options.size(); i++) result.push_back(&options[i]);
	return result;
}

// -- Private --

#ifndef JAYD_LIBRARY_APPICON_HPP
#define JAYD_LIBRARY_APPICON_HPP

#include <UI/CustomElement.h>

class AppIcon : public CustomElement {
public:
	AppIcon(ElementContainer *parent, const uint16_t *icon);

	void draw() override;

	void setSelected(bool selected);

private:
	const uint16_t *icon;
	bool selected = false;

};

#endif //JAYD_LIBRARY_APPICON_HPP

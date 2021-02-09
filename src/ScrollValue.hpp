#ifndef JAYD_LIBRARY_SCROLLVALUE_HPP
#define JAYD_LIBRARY_SCROLLVALUE_HPP

#include <UI/CustomElement.h>

class ScrollValue : public CustomElement {
public:
	ScrollValue(ElementContainer *parent);

	void scrollValue(int8_t newValue);

	void draw() override;

private:

	int value = 0;
};

#endif //JAYD_LIBRARY_SCROLLVALUE_HPP

#ifndef JAYD_LIBRARY_BTNVALUE_HPP
#define JAYD_LIBRARY_BTNVALUE_HPP

#include <UI/CustomElement.h>


class BtnValue : public CustomElement {
public:
	BtnValue(ElementContainer *parent);

	void buttonTap();

	void draw() override;

private:

	int btnValue=0;

};

#endif //JAYD_LIBRARY_BTNVALUE_HPP

#ifndef JAYD_LIBRARY_ENCVALUE_HPP
#define JAYD_LIBRARY_ENCVALUE_HPP

#include <UI/CustomElement.h>


class EncValue : public CustomElement {
public:
	EncValue(ElementContainer *parent);

	void encoderMove(int8_t newValue);

	void draw() override;

private:

	int value=0;

};

#endif //JAYD_LIBRARY_ENCVALUE_HPP

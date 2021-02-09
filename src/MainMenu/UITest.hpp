#ifndef JAYD_LIBRARY_UITEST_HPP
#define JAYD_LIBRARY_UITEST_HPP

#include <Support/Context.h>
#include <UI/LinearLayout.h>
#include <UI/Image.h>
#include <UI/TextElement.h>
#include <Elements/GridMenu.h>
#include "../InputLib/InputJayD.h"
#include "../EncValue.hpp"
#include "../BtnValue.hpp"
#include "../ScrollValue.hpp"
#include "AppIcon.hpp"


class UITest : public Context {
public:

	UITest(Display &display);

	void start() override;

	void stop() override;

	void draw() override;

private:
	static UITest *instance;

	GridLayout menu;

	/*Image image1;
	Image image2;
	Image image3;
	Image image4;*/

	std::vector<AppIcon> appIcons;

	uint8_t selectedIcon=0;

	void selectNext();
	void selectPrev();
	void buildUI();

};

#endif //JAYD_LIBRARY_UITEST_HPP
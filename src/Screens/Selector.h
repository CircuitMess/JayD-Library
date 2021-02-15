#ifndef JAYD_LIBRARY_SELECTOR_H
#define JAYD_LIBRARY_SELECTOR_H

#import <Support/Context.h>
#include <Elements/ListMenu.h>
#include <Loop/LoopListener.h>
#include "Playback.h"

class Selector : public Context, public LoopListener {
public:
	Selector(Display& display);

	void draw() override;
	void start() override;
	void stop() override;
	virtual ~Selector();
	void loop(uint micros) override;

private:
	static Selector* instance;

	Playback* playback = nullptr;

	ListMenu menu;
	uint32_t step = 0;
};


#endif //JAYD_LIBRARY_SELECTOR_H

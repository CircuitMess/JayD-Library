#ifndef JAYD_LIBRARY_PLAYBACK_H
#define JAYD_LIBRARY_PLAYBACK_H


#include <Support/Context.h>
#include <Loop/LoopListener.h>

class Playback : public Context, public LoopListener {
public:
	Playback(Display& display, const File& file);
	void draw() override;
	void start() override;
	void stop() override;
	void loop(uint micros) override;
	virtual ~Playback();

private:
	static Playback* instance;

	File file;

};


#endif //JAYD_LIBRARY_PLAYBACK_H

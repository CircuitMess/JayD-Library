// Integration self-check for MixSystem::_openChannel()'s pause-state
// preservation across a hot-swap (remoteLoad/openChannel replacing a
// channel's source while the deck is playing or paused).
//
// Reported defect: `_openChannel()` unconditionally paused the channel
// before replacing its source, then resumed it whenever the replacement
// succeeded, regardless of whether the deck had been paused by the user
// beforehand:
//
//   if(replaceSource(channel, newSource) || !wasPaused) mixer->resumeChannel(channel);
//
// Since replaceSource() returns true on any successful swap, a deck that
// was deliberately paused before a hot-swap request would be silently
// resumed by it (status API and firmware UI would then read
// paused=false for a deck the user had explicitly paused).
//
// Fixed: perform the replacement unconditionally, then resume only if the
// deck was not paused beforehand - restoring, not overriding, the prior
// pause state regardless of whether the replacement itself succeeded or
// failed:
//
//   replaceSource(channel, newSource);
//   if(!wasPaused) mixer->resumeChannel(channel);
//
// This has two halves:
//  1. A structural scan of the real, shipped _openChannel() proving the
//     fixed pattern is present and the reported buggy pattern is gone.
//  2. A deterministic functional harness mirroring _openChannel()'s exact
//     pause/resume decision, covering: successful replacement while
//     paused stays paused; successful replacement while playing keeps
//     playing; failed replacement preserves the prior state in both
//     directions.

#include <assert.h>
#include <fstream>
#include <iterator>
#include <string>

namespace {

std::string readFile(const char* path){
	std::ifstream f(path);
	assert(f.good());
	return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

bool contains(const std::string& source, const char* token){
	return source.find(token) != std::string::npos;
}

// --- Functional harness: mirrors _openChannel()'s fixed decision, with a
// fake mixer/replaceSource standing in for the real hardware-backed ones.

struct FakeMixer {
	bool paused = false;
	void pauseChannel(){ paused = true; }
	void resumeChannel(){ paused = false; }
	bool isChannelPaused() const { return paused; }
};

// Fixed contract: replace unconditionally, resume only if it wasn't
// paused beforehand - independent of whether the replacement succeeded.
void openChannelFixed(FakeMixer& mixer, bool replaceSucceeds){
	const bool wasPaused = mixer.isChannelPaused();
	mixer.pauseChannel();
	(void) replaceSucceeds; // replaceSource()'s own return no longer gates resume
	if(!wasPaused) mixer.resumeChannel();
}

// Pre-fix behaviour: resumes whenever the replacement succeeds regardless
// of the prior pause state. Used to prove this check discriminates the
// reported defect class.
void openChannelBuggy(FakeMixer& mixer, bool replaceSucceeds){
	const bool wasPaused = mixer.isChannelPaused();
	mixer.pauseChannel();
	if(replaceSucceeds || !wasPaused) mixer.resumeChannel();
}

} // namespace

int main(){
	// --- 1. Structural: the real _openChannel() no longer lets a
	// successful replacement force a resume.
	{
		const std::string source = readFile("src/AudioLib/Systems/MixSystem.cpp");
		assert(contains(source, "replaceSource(channel, newSource);"));
		assert(contains(source, "if(!wasPaused) mixer->resumeChannel(channel);"));
		assert(!contains(source, "if(replaceSource(channel, newSource) || !wasPaused)"));
	}

	// --- 2. Functional: successful replacement while paused must stay
	// paused (the reported bug: this used to resume).
	{
		FakeMixer mixer;
		mixer.paused = true;
		openChannelFixed(mixer, /*replaceSucceeds=*/true);
		assert(mixer.paused);
	}

	// --- 3. Successful replacement while playing must keep playing.
	{
		FakeMixer mixer;
		mixer.paused = false;
		openChannelFixed(mixer, /*replaceSucceeds=*/true);
		assert(!mixer.paused);
	}

	// --- 4. Failed replacement must also preserve prior state, both ways.
	{
		FakeMixer mixer;
		mixer.paused = true;
		openChannelFixed(mixer, /*replaceSucceeds=*/false);
		assert(mixer.paused);
	}
	{
		FakeMixer mixer;
		mixer.paused = false;
		openChannelFixed(mixer, /*replaceSucceeds=*/false);
		assert(!mixer.paused);
	}

	// --- 5. The pre-fix pattern must fail case 2, proving this check
	// would have caught the reported regression.
	{
		FakeMixer mixer;
		mixer.paused = true;
		openChannelBuggy(mixer, /*replaceSucceeds=*/true);
		assert(!mixer.paused); // bug: silently resumed a paused deck
	}

	return 0;
}

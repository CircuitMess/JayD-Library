// Integration self-check for the SDScheduler::addJob() bool-return contract
// introduced by the recording-core work (queue-full => delete-on-full,
// return false) versus every job-issuing call site merged from the AAC/MP3/
// WAV sources and the AAC/WAV outputs.
//
// It has two halves:
//  1. A structural scan of the real, shipped call sites proving none of them
//     discard SDScheduler::addJob()'s return value as a bare statement (the
//     exact defect class reported: SourceAAC ignored the bool at its read-job
//     and seek-job call sites, so a full queue left readJobPending stuck true
//     with readResult forever null, spinning processReadJob(true)/open()
//     forever).
//  2. A deterministic functional harness mirroring the fixed addReadJob/
//     processReadJob/seek state machine against an injectable fake scheduler,
//     covering: read-job queue-full rejection + recovery, seek rejection
//     preserving state (no silent success) + recovery, and a bounded
//     close/teardown wait that would only ever hang under the pre-fix
//     unconditional-pending pattern (also exercised here, and shown to fail
//     the same bounded wait, proving this check discriminates the bug).

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

// True if every "Sched.addJob(" occurrence in `source` is used (checked with
// if/return/assignment, etc.) rather than discarded as a bare statement.
bool everyAddJobCallIsChecked(const std::string& source){
	size_t pos = 0;
	bool found = false;
	while((pos = source.find("Sched.addJob(", pos)) != std::string::npos){
		found = true;
		size_t lineStart = source.rfind('\n', pos);
		lineStart = (lineStart == std::string::npos) ? 0 : lineStart + 1;
		size_t contentStart = source.find_first_not_of(" \t", lineStart);
		if(contentStart == std::string::npos || contentStart >= pos) return false;
		// A bare, discarded call looks like "Sched.addJob(" starting the
		// statement. Any checked form (if(!..., if(..., return ...,
		// x = ...) has other tokens before "Sched.addJob(" on that line.
		if(source.compare(contentStart, pos - contentStart, "Sched.addJob(") == 0){
			return false;
		}
		pos += 1;
	}
	return found;
}

// --- Functional harness: mirrors SourceAAC's fixed addReadJob/processReadJob ---

struct FakeScheduler {
	int capacity;
	int inFlight = 0;
	explicit FakeScheduler(int cap) : capacity(cap){}
	bool addJob(){
		if(inFlight >= capacity) return false;
		inFlight++;
		return true;
	}
	void completeOne(){ if(inFlight > 0) inFlight--; }
};

struct ReadJobHarness {
	FakeScheduler& sched;
	bool pending = false;
	bool resultReady = false;
	int allocated = 0;
	int freed = 0;
	explicit ReadJobHarness(FakeScheduler& s) : sched(s){}

	// Fixed contract: only claim a job is in flight once it is actually queued.
	void addReadJob(){
		if(pending) return;
		allocated++;
		if(!sched.addJob()){
			freed++; // caller must free its own buffer; addJob() never took ownership.
			return;
		}
		pending = true;
	}

	// Bounded stand-in for the real busy-wait: returns false ("would have
	// hung") instead of spinning forever if a wait is requested on a result
	// that will never arrive.
	bool processReadJob(bool wait, int maxSpin){
		if(!pending) return true;
		if(!resultReady){
			if(!wait) return true;
			int i = 0;
			for(; i < maxSpin && !resultReady; i++){}
			if(!resultReady) return false;
		}
		resultReady = false;
		pending = false;
		return true;
	}

	void deliverResult(){ resultReady = true; sched.completeOne(); }
};

// Pre-fix behaviour: pending is claimed unconditionally, regardless of
// whether the job was actually queued. Used to prove this check would have
// caught the reported defect.
struct BuggyReadJobHarness {
	FakeScheduler& sched;
	bool pending = false;
	bool resultReady = false;
	explicit BuggyReadJobHarness(FakeScheduler& s) : sched(s){}
	void addReadJob(){
		if(pending) return;
		sched.addJob(); // return value ignored, exactly like the reported bug
		pending = true;
	}
	bool processReadJob(bool wait, int maxSpin){
		if(!pending) return true;
		if(!resultReady){
			if(!wait) return true;
			int i = 0;
			for(; i < maxSpin && !resultReady; i++){}
			if(!resultReady) return false;
		}
		resultReady = false;
		pending = false;
		return true;
	}
};

struct SeekHarness {
	FakeScheduler& sched;
	uint64_t elapsedFrames;
	explicit SeekHarness(FakeScheduler& s, uint64_t initial) : sched(s), elapsedFrames(initial){}
	// Fixed contract: enqueue first, only commit state on success.
	bool seek(uint64_t frame){
		if(!sched.addJob()) return false;
		elapsedFrames = frame;
		return true;
	}
};

struct BuggySeekHarness {
	FakeScheduler& sched;
	uint64_t elapsedFrames;
	explicit BuggySeekHarness(FakeScheduler& s, uint64_t initial) : sched(s), elapsedFrames(initial){}
	bool seek(uint64_t frame){
		sched.addJob(); // return value ignored, exactly like the reported bug
		elapsedFrames = frame; // silent "success" even if the seek was dropped
		return true;
	}
};

template<typename Harness>
bool waitForIdle(Harness& h, int maxIterations){
	for(int i = 0; i < maxIterations; i++){
		if(!h.pending) return true;
	}
	return false;
}

} // namespace

int main(){
	// --- 1. Structural: every real call site must check addJob()'s return value.
	const char* callers[] = {
			"src/AudioLib/SourceAAC.cpp",
			"src/AudioLib/SourceMP3.cpp",
			"src/AudioLib/SourceWAV.cpp",
			"src/AudioLib/OutputAAC.cpp",
			"src/AudioLib/OutputWAV.cpp",
	};
	for(const char* path : callers){
		assert(everyAddJobCallIsChecked(readFile(path)));
	}

	// --- 2. Read job: queue-full rejection must not claim a pending job.
	{
		FakeScheduler sched(1);
		assert(sched.addJob()); // fill the only slot with an unrelated job
		ReadJobHarness h(sched);
		h.addReadJob();
		assert(!h.pending); // fixed: rejection leaves the read retryable
		assert(h.allocated == h.freed); // no leaked buffer on rejection
		assert(h.processReadJob(true, 100000)); // no job pending: never spins

		// Recovery: free capacity, retry succeeds.
		sched.completeOne();
		h.addReadJob();
		assert(h.pending);
		h.deliverResult();
		assert(h.processReadJob(true, 100000));
		assert(!h.pending);
	}

	// --- 2b. Same scenario against the pre-fix pattern must fail the bounded
	// wait, proving this check discriminates the reported defect.
	{
		FakeScheduler sched(1);
		assert(sched.addJob());
		BuggyReadJobHarness h(sched);
		h.addReadJob();
		assert(h.pending); // bug: falsely claims the job is in flight
		assert(!h.processReadJob(true, 1000)); // would spin forever in production
	}

	// --- 3. Seek: rejection must not silently move the tracked position.
	{
		FakeScheduler sched(1);
		assert(sched.addJob());
		SeekHarness h(sched, 42);
		assert(!h.seek(1000));
		assert(h.elapsedFrames == 42); // state preserved for a coherent retry

		sched.completeOne();
		assert(h.seek(1000));
		assert(h.elapsedFrames == 1000);
	}
	{
		FakeScheduler sched(1);
		assert(sched.addJob());
		BuggySeekHarness h(sched, 42);
		assert(h.seek(1000)); // bug: reports success despite the dropped seek
		assert(h.elapsedFrames == 1000); // silent corruption: file was never seeked
	}

	// --- 4. Close/teardown must not hang after a rejected job.
	{
		FakeScheduler sched(1);
		assert(sched.addJob());
		ReadJobHarness h(sched);
		h.addReadJob();
		assert(waitForIdle(h, 1000)); // fixed: already idle, returns immediately
	}
	{
		FakeScheduler sched(1);
		assert(sched.addJob());
		BuggyReadJobHarness h(sched);
		h.addReadJob();
		assert(!waitForIdle(h, 1000)); // bug: stuck pending forever
	}

	return 0;
}

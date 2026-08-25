// Integration self-check for the SDScheduler dual-API contract restored by
// "Scope nonblocking SD enqueue to recording": legacy blocking
// void SDScheduler::addJob(SDJob*) for every decode/encode call site
// (SourceAAC, SourceMP3, SourceWAV, OutputAAC), plus a new nonblocking
// bool SDScheduler::tryAddJob(SDJob*) scoped exclusively to OutputWAV's
// real-time recording write/finalize path.
//
// This supersedes an earlier universal "every addJob() caller must check a
// bool return" contract, which was correct for one intermediate revision of
// the recording-core work but is no longer what ships: addJob() is void
// again, so a caller checking its return would not even compile, and a
// caller silently reusing tryAddJob() outside OutputWAV would reintroduce
// the false-pending queue-full class of bug this check exists to prevent.
//
// It has two halves:
//  1. A structural scan of the real, shipped call sites proving: decode/
//     encode sources call only the blocking Sched.addJob(...) (never
//     Sched.tryAddJob(...)), and OutputWAV calls only Sched.tryAddJob(...)
//     (never the blocking Sched.addJob(...)) with every call site checking
//     the bool return.
//  2. A deterministic functional harness mirroring OutputWAV's
//     addWriteJob()/queueFinalizeJob() use of tryAddJob(): queue-full
//     rejection must leave the buffer retryable (no leak, no false
//     "in flight" state), and recovery once the queue has room must
//     succeed cleanly.

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

// True if every occurrence of `token` (e.g. "Sched.tryAddJob(") in `source`
// is used (if/return/assignment, etc.) rather than discarded as a bare
// statement starting its line.
bool everyCallIsChecked(const std::string& source, const char* token){
	const size_t tokenLen = std::string(token).length();
	size_t pos = 0;
	bool found = false;
	while((pos = source.find(token, pos)) != std::string::npos){
		found = true;
		size_t lineStart = source.rfind('\n', pos);
		lineStart = (lineStart == std::string::npos) ? 0 : lineStart + 1;
		size_t contentStart = source.find_first_not_of(" \t", lineStart);
		if(contentStart == std::string::npos || contentStart >= pos) return false;
		if(source.compare(contentStart, pos - contentStart, token) == 0){
			return false;
		}
		pos += tokenLen;
	}
	return found;
}

// --- Functional harness: mirrors OutputWAV's fixed addWriteJob(), which
// only claims a buffer is in flight once tryAddJob() actually enqueues it.

struct FakeScheduler {
	int capacity;
	int inFlight = 0;
	explicit FakeScheduler(int cap) : capacity(cap){}
	bool tryAddJob(){
		if(inFlight >= capacity) return false;
		inFlight++;
		return true;
	}
	void completeOne(){ if(inFlight > 0) inFlight--; }
};

struct WriteJobHarness {
	FakeScheduler& sched;
	bool pending = false;
	bool bufferHeld = true; // starts owned by the free-buffer pool
	explicit WriteJobHarness(FakeScheduler& s) : sched(s){}

	// Fixed contract: only hand the buffer to the scheduler once it is
	// actually queued; a rejection leaves it retryable in the pool.
	bool addWriteJob(){
		if(pending || !bufferHeld) return false;
		if(!sched.tryAddJob()) return false; // buffer stays in the pool
		bufferHeld = false;
		pending = true;
		return true;
	}

	void deliverResult(){
		sched.completeOne();
		pending = false;
		bufferHeld = true; // returned to the pool once written
	}
};

// Pre-fix behaviour: the buffer is handed off unconditionally regardless of
// whether tryAddJob() actually queued it. Used to prove this check would
// have caught the reported defect class if it recurred.
struct BuggyWriteJobHarness {
	FakeScheduler& sched;
	bool pending = false;
	bool bufferHeld = true;
	explicit BuggyWriteJobHarness(FakeScheduler& s) : sched(s){}
	bool addWriteJob(){
		if(pending || !bufferHeld) return false;
		sched.tryAddJob(); // return value ignored, exactly like the reported bug
		bufferHeld = false;
		pending = true;
		return true;
	}
};

} // namespace

int main(){
	// --- 1. Structural: decode/encode sources use only the restored
	// blocking API; OutputWAV uses only the nonblocking API, checked.
	const char* blockingCallers[] = {
			"src/AudioLib/SourceAAC.cpp",
			"src/AudioLib/SourceMP3.cpp",
			"src/AudioLib/SourceWAV.cpp",
			"src/AudioLib/OutputAAC.cpp",
	};
	for(const char* path : blockingCallers){
		const std::string source = readFile(path);
		assert(contains(source, "Sched.addJob("));
		assert(!contains(source, "Sched.tryAddJob("));
	}
	{
		const std::string source = readFile("src/AudioLib/OutputWAV.cpp");
		assert(!contains(source, "Sched.addJob("));
		assert(everyCallIsChecked(source, "Sched.tryAddJob("));
	}
	{
		const std::string header = readFile("src/Services/SDScheduler.h");
		assert(contains(header, "void addJob(SDJob *job);"));
		assert(contains(header, "bool tryAddJob(SDJob *job);"));
	}

	// --- 2. Write job: queue-full rejection must not claim a pending job
	// or strand the buffer outside the free-buffer pool.
	{
		FakeScheduler sched(1);
		assert(sched.tryAddJob()); // fill the only slot with an unrelated job
		WriteJobHarness h(sched);
		assert(!h.addWriteJob()); // fixed: rejection leaves the write retryable
		assert(!h.pending);
		assert(h.bufferHeld); // buffer never left the pool

		// Recovery: free capacity, retry succeeds.
		sched.completeOne();
		assert(h.addWriteJob());
		assert(h.pending);
		h.deliverResult();
		assert(!h.pending);
		assert(h.bufferHeld);
	}

	// --- 2b. Same scenario against the pre-fix pattern must strand the
	// buffer, proving this check discriminates the reported defect class.
	{
		FakeScheduler sched(1);
		assert(sched.tryAddJob());
		BuggyWriteJobHarness h(sched);
		assert(h.addWriteJob()); // bug: reports success despite the drop
		assert(h.pending);       // bug: falsely claims the job is in flight
		assert(!h.bufferHeld);   // bug: buffer stranded outside the pool
	}

	return 0;
}

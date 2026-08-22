#ifndef JAYD_LIBRARY_RECORDINGFINALIZE_H
#define JAYD_LIBRARY_RECORDINGFINALIZE_H

#include <stdint.h>

enum class FinalizeEnqueueResult : uint8_t {
	QUEUED,
	RETRY,
	EXHAUSTED
};

inline FinalizeEnqueueResult recordFinalizeEnqueue(
		bool queued,
		uint8_t& failures,
		uint8_t maxFailures
){
	if(queued) return FinalizeEnqueueResult::QUEUED;
	if(failures < maxFailures) failures++;
	return failures >= maxFailures
			? FinalizeEnqueueResult::EXHAUSTED
			: FinalizeEnqueueResult::RETRY;
}

#endif //JAYD_LIBRARY_RECORDINGFINALIZE_H

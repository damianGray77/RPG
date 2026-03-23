#pragma once
#ifndef THREAD_H
#define THREAD_H

class WindowWin32;

class ThreadPool {
public:
	typedef void (*JobFunc)(void* ctx, int32 start, int32 end);

	static const uint32 MAX_WORKERS = 8;

	uint32 worker_count;

	bool init(WindowWin32 *os, uint32 count);
	void shutdown();
	void batch(JobFunc func, void **ctxs, int32 total_start, int32 total_end, uint32 num_workers, bool caller_participates = false);

	ThreadPool();
private:
	struct WorkItem {
		JobFunc func;
		void*   ctx;
		int32   start;
		int32   end;
	};

	WindowWin32    *os;
	void           *thread_handles[MAX_WORKERS];
	void           *work_ready    [MAX_WORKERS];
	void           *work_done     [MAX_WORKERS];
	WorkItem        work_items    [MAX_WORKERS];
	volatile bool   work_pending  [MAX_WORKERS];
	volatile bool   alive;

	static void worker_entry(void *param);
};

#endif

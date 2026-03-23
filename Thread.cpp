#include "pch.h"
#include "Thread.h"
#include "WindowWin32.h"
#include <immintrin.h>

// ── worker entry point ──────────────────────────────────────────────────────

struct WorkerParam {
	ThreadPool *pool;
	uint32      index;
};

static WorkerParam worker_params[ThreadPool::MAX_WORKERS];

void ThreadPool::worker_entry(void *param) {
	WorkerParam *wp = (WorkerParam*)param;
	ThreadPool  *pool = wp->pool;
	const uint32 index = wp->index;

	while (true) {
		pool->os->event_wait(pool->work_ready[index]);
		if (!pool->alive) { break; }
		pool->os->event_reset(pool->work_ready[index]);

		const WorkItem &item = pool->work_items[index];
		item.func(item.ctx, item.start, item.end);

		pool->os->event_signal(pool->work_done[index]);
	}
}

// ── ThreadPool ──────────────────────────────────────────────────────────────

ThreadPool::ThreadPool() {
	os           = NULL;
	worker_count = 0;
	alive        = false;
	memset(thread_handles, 0, sizeof(thread_handles));
	memset(work_ready,     0, sizeof(work_ready));
	memset(work_done,      0, sizeof(work_done));
	memset((void*)work_pending, 0, sizeof(work_pending));
	memset(work_items,     0, sizeof(work_items));
}

bool ThreadPool::init(WindowWin32 *os, uint32 count) {
	this->os     = os;
	worker_count = MIN(count, MAX_WORKERS);
	alive        = true;

	for (uint32 i = 0; i < worker_count; ++i) {
		work_ready[i] = os->event_create(true, false);
		work_done[i]  = os->event_create(true, true);

		if (!work_ready[i] || !work_done[i]) { return false; }

		worker_params[i] = { this, i };
		thread_handles[i] = os->thread_create(worker_entry, &worker_params[i], i);

		if (!thread_handles[i]) { return false; }

		// pin worker to P-core (i+1, leaving core 0 for main thread)
		os->set_thread_affinity(thread_handles[i], i + 1);
	}

	return true;
}

void ThreadPool::batch(JobFunc func, void **ctxs, int32 total_start, int32 total_end, uint32 num_workers, bool caller_participates) {
	num_workers = MIN(num_workers, worker_count);

	// fallback: run everything on the calling thread
	if (num_workers == 0) {
		func(ctxs[0], total_start, total_end);
		return;
	}

	const uint32 total_threads = num_workers + (caller_participates ? 1 : 0);
	const int32  total  = total_end - total_start;
	const int32  chunk  = total / (int32)total_threads;
	const int32  remain = total % (int32)total_threads;
	int32 offset = total_start;

	// dispatch to workers
	for (uint32 i = 0; i < num_workers; ++i) {
		const int32 rows = chunk + (((int32)i < remain) ? 1 : 0);
		os->event_reset(work_done[i]);
		work_items[i] = { func, ctxs[i], offset, offset + rows };
		os->event_signal(work_ready[i]);
		offset += rows;
	}

	// caller takes last chunk if participating
	if (caller_participates) {
		func(ctxs[num_workers], offset, total_end);
	}

	// wait for all workers to finish
	os->event_wait_all(work_done, num_workers);
}

void ThreadPool::shutdown() {
	if (worker_count == 0) { return; }

	alive = false;
	for (uint32 i = 0; i < worker_count; ++i) {
		os->event_signal(work_ready[i]);
	}

	os->event_wait_all(thread_handles, worker_count);

	for (uint32 i = 0; i < worker_count; ++i) {
		os->handle_close(thread_handles[i]);
		os->handle_close(work_ready[i]);
		os->handle_close(work_done[i]);
	}

	worker_count = 0;
}

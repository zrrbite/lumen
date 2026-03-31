#pragma once

#include <array>
#include <cstdint>

#include "lumen/core/types.hpp"

namespace lumen {

/// Base class for all cooperative tasks.
class Task
{
  public:
	virtual ~Task() = default;

	/// Called by the scheduler each time this task is due.
	virtual void execute(TickMs now) = 0;

	/// How often this task runs (0 = every cycle).
	virtual TickMs period_ms() const { return 0; }

	/// Priority: lower = higher priority. Default 128.
	virtual uint8_t priority() const { return 128; }

	/// Name for debugging.
	virtual const char* name() const { return "task"; }
};

/// Cooperative task scheduler. Replaces RTOS for simple systems,
/// coexists with RTOS when one is present.
class Scheduler
{
  public:
	static constexpr uint8_t MAX_TASKS = 16;

	void add(Task& task)
	{
		for (auto& entry : tasks_)
		{
			if (entry.task == nullptr)
			{
				entry.task	   = &task;
				entry.last_run = 0;
				return;
			}
		}
	}

	void remove(Task& task)
	{
		for (auto& entry : tasks_)
		{
			if (entry.task == &task)
			{
				entry.task = nullptr;
				return;
			}
		}
	}

	/// Run one scheduler cycle. Call this in a loop or from an OS task.
	template <typename TickSource> void tick(TickSource& clock)
	{
		TickMs now = clock.now();
		for (auto& entry : tasks_)
		{
			if (entry.task == nullptr)
				continue;
			TickMs period = entry.task->period_ms();
			if (now - entry.last_run >= period)
			{
				entry.last_run = now;
				entry.task->execute(now);
			}
		}
	}

	/// Main loop — never returns. Calls tick() and idle_hook between cycles.
	template <typename TickSource, typename IdleFunc> [[noreturn]] void run(TickSource& clock, IdleFunc idle)
	{
		while (true)
		{
			tick(clock);
			idle();
		}
	}

	/// Main loop with default idle (busy wait).
	template <typename TickSource> [[noreturn]] void run(TickSource& clock)
	{
		run(clock, [] {});
	}

  private:
	struct TaskEntry
	{
		Task* task		= nullptr;
		TickMs last_run = 0;
	};

	std::array<TaskEntry, MAX_TASKS> tasks_{};
};

} // namespace lumen

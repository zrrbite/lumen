#pragma once

#include "lumen/core/types.hpp"

// Only include when FreeRTOS is available
#if defined(LUMEN_HAS_FREERTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#endif

namespace lumen::hal {

/// FreeRTOS OS abstraction — yields to OS scheduler when idle.
struct FreeRtosOS
{
	static void idle()
	{
#if defined(LUMEN_HAS_FREERTOS)
		vTaskDelay(1);
#endif
	}

	struct Mutex
	{
#if defined(LUMEN_HAS_FREERTOS)
		Mutex() { handle_ = xSemaphoreCreateMutex(); }
		~Mutex() { vSemaphoreDelete(handle_); }
		void lock() { xSemaphoreTake(handle_, portMAX_DELAY); }
		void unlock() { xSemaphoreGive(handle_); }

	  private:
		SemaphoreHandle_t handle_;
#else
		void lock() {}
		void unlock() {}
#endif
	};
};

} // namespace lumen::hal

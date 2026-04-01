#pragma once

/// Linux evdev touch/input driver.
/// Reads touch events from /dev/input/event*.
/// Works with RPi official touchscreen and USB touchscreens.

#ifdef __linux__

#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>

#include "lumen/hal/touch_driver.hpp"

namespace lumen::drivers {

class LinuxEvdev
{
  public:
	~LinuxEvdev()
	{
		if (fd_ >= 0)
			close(fd_);
	}

	/// Try to open a touch device. Scans /dev/input/event* for a touchscreen.
	bool init()
	{
		// Try to find a touch device
		for (int i = 0; i < 16; ++i)
		{
			char path[32];
			snprintf(path, sizeof(path), "/dev/input/event%d", i);

			int fd = open(path, O_RDONLY | O_NONBLOCK);
			if (fd < 0)
				continue;

			// Check if this device has ABS_MT_POSITION_X (multitouch)
			unsigned long abs_bits[(ABS_MAX + 1) / (sizeof(unsigned long) * 8) + 1]{};
			if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) >= 0)
			{
				// Check for ABS_MT_POSITION_X (0x35)
				if (abs_bits[0x35 / (sizeof(unsigned long) * 8)] & (1UL << (0x35 % (sizeof(unsigned long) * 8))))
				{
					fd_ = fd;

					// Get axis ranges
					struct input_absinfo abs_x{};
					struct input_absinfo abs_y{};
					ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &abs_x);
					ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &abs_y);
					abs_x_max_ = abs_x.maximum;
					abs_y_max_ = abs_y.maximum;
					return true;
				}
			}

			// Check for ABS_X (single touch)
			if (abs_bits[ABS_X / (sizeof(unsigned long) * 8)] & (1UL << (ABS_X % (sizeof(unsigned long) * 8))))
			{
				fd_ = fd;
				struct input_absinfo abs_x{};
				struct input_absinfo abs_y{};
				ioctl(fd, EVIOCGABS(ABS_X), &abs_x);
				ioctl(fd, EVIOCGABS(ABS_Y), &abs_y);
				abs_x_max_ = abs_x.maximum;
				abs_y_max_ = abs_y.maximum;
				return true;
			}

			close(fd);
		}
		return false;
	}

	/// Set display resolution for coordinate mapping.
	void set_display_size(uint16_t w, uint16_t h)
	{
		disp_w_ = w;
		disp_h_ = h;
	}

	bool poll(hal::TouchPoint& out)
	{
		if (fd_ < 0)
			return false;

		struct input_event ev{};
		bool changed = false;

		// Read all pending events
		while (read(fd_, &ev, sizeof(ev)) == sizeof(ev))
		{
			if (ev.type == EV_ABS)
			{
				if (ev.code == ABS_MT_POSITION_X || ev.code == ABS_X)
				{
					raw_x_	= ev.value;
					changed = true;
				}
				else if (ev.code == ABS_MT_POSITION_Y || ev.code == ABS_Y)
				{
					raw_y_	= ev.value;
					changed = true;
				}
				else if (ev.code == ABS_MT_TRACKING_ID)
				{
					// -1 = finger lifted
					touching_ = (ev.value >= 0);
					changed	  = true;
				}
			}
			else if (ev.type == EV_KEY && ev.code == BTN_TOUCH)
			{
				touching_ = (ev.value != 0);
				changed	  = true;
			}
		}

		if (changed)
		{
			// Map raw coordinates to display
			int16_t px = 0;
			int16_t py = 0;
			if (abs_x_max_ > 0)
				px = static_cast<int16_t>((raw_x_ * disp_w_) / abs_x_max_);
			if (abs_y_max_ > 0)
				py = static_cast<int16_t>((raw_y_ * disp_h_) / abs_y_max_);

			if (px < 0)
				px = 0;
			if (px >= disp_w_)
				px = disp_w_ - 1;
			if (py < 0)
				py = 0;
			if (py >= disp_h_)
				py = disp_h_ - 1;

			out.pos		 = {px, py};
			out.pressed	 = touching_;
			out.pressure = touching_ ? 128 : 0;
			return true;
		}

		return false;
	}

	uint8_t max_points() const { return 1; }

  private:
	int fd_			   = -1;
	int32_t raw_x_	   = 0;
	int32_t raw_y_	   = 0;
	int32_t abs_x_max_ = 4096;
	int32_t abs_y_max_ = 4096;
	bool touching_	   = false;
	uint16_t disp_w_   = 800;
	uint16_t disp_h_   = 480;
};

} // namespace lumen::drivers

#endif // __linux__

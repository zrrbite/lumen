#pragma once

/// Linux DRM/KMS display driver.
/// Uses dumb buffers + page flip for tear-free double-buffered rendering.
/// Works on Raspberry Pi 4, any Linux system with /dev/dri/card*.

#ifdef __linux__

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>

// DRM headers — minimal inline definitions to avoid system header dependency
// These match the kernel UAPI structs

#include <drm/drm.h>
#include <drm/drm_mode.h>

#include "lumen/core/types.hpp"

namespace lumen::drivers {

/// DRM display driver with double buffering.
class LinuxDrm
{
  public:
	using PixelFormat = Argb8888;
	using pixel_t	  = uint32_t;

	~LinuxDrm()
	{
		if (mapped_[0])
			munmap(mapped_[0], map_size_[0]);
		if (mapped_[1])
			munmap(mapped_[1], map_size_[1]);
		if (fd_ >= 0)
			close(fd_);
	}

	/// Initialize DRM: open device, find connector, create buffers.
	bool init()
	{
		fd_ = open("/dev/dri/card0", O_RDWR);
		if (fd_ < 0)
		{
			fd_ = open("/dev/dri/card1", O_RDWR);
			if (fd_ < 0)
				return false;
		}

		// Get resources
		struct drm_mode_card_res res{};
		if (ioctl(fd_, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0)
			return false;

		uint32_t conn_ids[8]{};
		uint32_t crtc_ids[8]{};
		res.connector_id_ptr = reinterpret_cast<uint64_t>(conn_ids);
		res.crtc_id_ptr		 = reinterpret_cast<uint64_t>(crtc_ids);
		res.count_connectors = 8;
		res.count_crtcs		 = 8;
		if (ioctl(fd_, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0)
			return false;

		// Find connected connector
		for (uint32_t i = 0; i < res.count_connectors; ++i)
		{
			struct drm_mode_get_connector conn{};
			conn.connector_id = conn_ids[i];
			if (ioctl(fd_, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0)
				continue;

			if (conn.connection != 1) // DRM_MODE_CONNECTED
				continue;

			// Get modes
			struct drm_mode_modeinfo modes[16]{};
			uint32_t encoders[4]{};
			conn.modes_ptr		= reinterpret_cast<uint64_t>(modes);
			conn.count_modes	= 16;
			conn.encoders_ptr	= reinterpret_cast<uint64_t>(encoders);
			conn.count_encoders = 4;
			if (ioctl(fd_, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0)
				continue;

			if (conn.count_modes == 0)
				continue;

			// Use first (preferred) mode
			mode_		  = modes[0];
			connector_id_ = conn_ids[i];
			s_width_	  = mode_.hdisplay;
			s_height_	  = mode_.vdisplay;

			// Find encoder + CRTC
			if (conn.encoder_id != 0)
			{
				struct drm_mode_get_encoder enc{};
				enc.encoder_id = conn.encoder_id;
				if (ioctl(fd_, DRM_IOCTL_MODE_GETENCODER, &enc) == 0)
				{
					crtc_id_ = enc.crtc_id;
				}
			}
			if (crtc_id_ == 0 && res.count_crtcs > 0)
				crtc_id_ = crtc_ids[0];

			break;
		}

		if (s_width_ == 0 || connector_id_ == 0 || crtc_id_ == 0)
			return false;

		// Create two dumb buffers
		for (int i = 0; i < 2; ++i)
		{
			if (!create_buffer(i))
				return false;
		}

		// Set CRTC to first buffer
		struct drm_mode_crtc crtc{};
		crtc.crtc_id			= crtc_id_;
		uint32_t conn			= connector_id_;
		crtc.set_connectors_ptr = reinterpret_cast<uint64_t>(&conn);
		crtc.count_connectors	= 1;
		crtc.fb_id				= fb_id_[0];
		crtc.mode				= mode_;
		crtc.mode_valid			= 1;
		if (ioctl(fd_, DRM_IOCTL_MODE_SETCRTC, &crtc) < 0)
			return false;

		front_ = 0;
		back_  = 1;
		return true;
	}

	static uint16_t width() { return s_width_; }
	static uint16_t height() { return s_height_; }

	/// Get the back buffer for rendering.
	pixel_t* framebuffer() { return static_cast<pixel_t*>(mapped_[back_]); }

	/// Page flip: swap front/back and wait for vsync.
	void flush()
	{
		struct drm_mode_crtc_page_flip flip{};
		flip.crtc_id = crtc_id_;
		flip.fb_id	 = fb_id_[back_];
		flip.flags	 = 0x01; // DRM_MODE_PAGE_FLIP_EVENT

		if (ioctl(fd_, DRM_IOCTL_MODE_PAGE_FLIP, &flip) == 0)
		{
			// Wait for flip event (vsync)
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(fd_, &fds);
			struct timeval timeout{};
			timeout.tv_sec = 1;
			select(fd_ + 1, &fds, nullptr, nullptr, &timeout);

			// Read and discard the event
			char buf[256];
			read(fd_, buf, sizeof(buf));
		}

		// Swap
		front_ ^= 1;
		back_ ^= 1;
	}

	/// Compatibility: set_window / write_pixels (not used in framebuffer mode)
	void set_window(Rect) {}
	void write_pixels(const pixel_t*, uint32_t) {}
	void write_pixels_dma(const pixel_t*, uint32_t) {}
	void write_pixels_dma_wait() {}
	bool dma_busy() const { return false; }

  private:
	static inline uint16_t s_width_	 = 0;
	static inline uint16_t s_height_ = 0;

	int fd_				   = -1;
	uint32_t connector_id_ = 0;
	uint32_t crtc_id_	   = 0;

	struct drm_mode_modeinfo mode_{};

	uint32_t handle_[2]{};
	uint32_t fb_id_[2]{};
	void* mapped_[2]{};
	uint64_t map_size_[2]{};
	uint32_t pitch_[2]{};

	int front_ = 0;
	int back_  = 1;

	bool create_buffer(int idx)
	{
		struct drm_mode_create_dumb create{};
		create.width  = s_width_;
		create.height = s_height_;
		create.bpp	  = 32;
		if (ioctl(fd_, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0)
			return false;

		handle_[idx] = create.handle;
		pitch_[idx]	 = create.pitch;

		// Create framebuffer
		struct drm_mode_fb_cmd cmd{};
		cmd.width  = s_width_;
		cmd.height = s_height_;
		cmd.pitch  = create.pitch;
		cmd.bpp	   = 32;
		cmd.depth  = 24;
		cmd.handle = create.handle;
		if (ioctl(fd_, DRM_IOCTL_MODE_ADDFB, &cmd) < 0)
			return false;

		fb_id_[idx] = cmd.fb_id;

		// Map to userspace
		struct drm_mode_map_dumb map{};
		map.handle = create.handle;
		if (ioctl(fd_, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0)
			return false;

		map_size_[idx] = create.size;
		mapped_[idx]   = mmap(nullptr, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, map.offset);
		if (mapped_[idx] == MAP_FAILED)
		{
			mapped_[idx] = nullptr;
			return false;
		}

		memset(mapped_[idx], 0, create.size);
		return true;
	}
};

} // namespace lumen::drivers

#endif // __linux__

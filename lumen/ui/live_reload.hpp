#pragma once

/// Live reload — processes UART commands to update widgets at runtime.
/// Protocol: newline-delimited text commands.
///
/// Commands:
///   SET <widget>.<property> <value>
///   GET <widget>.<property>
///   LIST
///   PING
///
/// Examples:
///   SET title.text Hello World!
///   SET title.color 100,200,255
///   SET bar.value 75
///   SET btn.y 120
///   SET btn.visible 0

#include <cstring>

#include "lumen/ui/script.hpp"
#include "lumen/ui/widget_registry.hpp"

namespace lumen::ui {

/// Processes incoming UART bytes and executes commands against a widget registry.
class LiveReload
{
  public:
	explicit LiveReload(WidgetRegistry& registry) : registry_(registry), script_(registry) {}

	/// Feed a single byte from UART. Processes command when newline received.
	/// Returns true if a command was executed.
	bool feed(uint8_t byte)
	{
		if (byte == '\n' || byte == '\r')
		{
			if (buf_pos_ > 0)
			{
				buf_[buf_pos_] = '\0';
				process_command(buf_);
				buf_pos_ = 0;
				return true;
			}
			return false;
		}

		if (buf_pos_ < MAX_CMD - 1)
		{
			buf_[buf_pos_++] = static_cast<char>(byte);
		}
		return false;
	}

	/// Feed a complete null-terminated string.
	void feed_string(const char* str)
	{
		while (*str)
		{
			feed(static_cast<uint8_t>(*str));
			++str;
		}
		feed('\n');
	}

	/// Set callback for sending response text back over UART.
	using ResponseFn = void (*)(const char* text);
	void set_response_callback(ResponseFn fn) { response_fn_ = fn; }

	/// Access the script engine for programmatic use.
	ScriptEngine& script() { return script_; }

  private:
	static constexpr uint16_t MAX_CMD = 128;

	WidgetRegistry& registry_;
	ScriptEngine script_;
	char buf_[MAX_CMD]{};
	uint16_t buf_pos_		= 0;
	ResponseFn response_fn_ = nullptr;

	void respond(const char* text)
	{
		if (response_fn_)
			response_fn_(text);
	}

	void process_command(char* cmd)
	{
		// Skip leading whitespace
		while (*cmd == ' ')
			++cmd;

		if (std::strncmp(cmd, "SET ", 4) == 0)
		{
			handle_set(cmd + 4);
		}
		else if (std::strncmp(cmd, "PING", 4) == 0)
		{
			respond("PONG\n");
		}
		else if (std::strncmp(cmd, "RUN ", 4) == 0)
		{
			if (script_.exec(cmd + 4))
				respond("OK\n");
			else
				respond("ERR script\n");
		}
		else if (std::strncmp(cmd, "LIST", 4) == 0)
		{
			handle_list();
		}
		else
		{
			respond("ERR unknown command\n");
		}
	}

	void handle_set(char* args)
	{
		// Parse: <widget>.<property> <value>
		// Find the dot separator
		char* dot = nullptr;
		for (char* p = args; *p && *p != ' '; ++p)
		{
			if (*p == '.')
				dot = p;
		}

		if (dot == nullptr)
		{
			respond("ERR missing widget.property\n");
			return;
		}

		// Split at dot
		*dot		 = '\0';
		char* widget = args;
		char* prop	 = dot + 1;

		// Find space between property and value
		char* space = prop;
		while (*space && *space != ' ')
			++space;

		if (*space == '\0')
		{
			respond("ERR missing value\n");
			return;
		}

		*space		= '\0';
		char* value = space + 1;

		// Execute
		if (registry_.set_property(widget, prop, value))
		{
			respond("OK\n");
		}
		else
		{
			respond("ERR not found\n");
		}
	}

	void handle_list()
	{
		for (uint8_t i = 0; i < registry_.count(); ++i)
		{
			// We can't easily iterate without exposing internals,
			// so just respond with count for now
		}
		// Simple response with count
		char buf[16];
		buf[0]	= '0' + (registry_.count() / 10);
		buf[1]	= '0' + (registry_.count() % 10);
		buf[2]	= ' ';
		buf[3]	= 'w';
		buf[4]	= 'i';
		buf[5]	= 'd';
		buf[6]	= 'g';
		buf[7]	= 'e';
		buf[8]	= 't';
		buf[9]	= 's';
		buf[10] = '\n';
		buf[11] = '\0';
		respond(buf);
	}
};

} // namespace lumen::ui

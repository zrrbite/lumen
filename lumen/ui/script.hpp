#pragma once

/// Minimal scripting engine for UI logic.
/// Evaluates simple expressions and statements with widget property access.
/// No heap allocation — fixed-size variable table and expression stack.
///
/// Supported:
///   Variables:    counter = 0, x = 42
///   Arithmetic:   +, -, *, /, %
///   Comparison:   ==, !=, <, >, <=, >=
///   Logic:        &&, ||, !
///   Assignment:   =, +=, -=, *=
///   Widget props: widget.property (read/write via registry)
///   If/else:      if expr { stmts } else { stmts }
///   Strings:      "text" (only for widget text properties)

#include <cstdint>
#include <cstring>

#include "lumen/ui/widget_registry.hpp"

namespace lumen::ui {

/// A script value — either an integer or a string pointer.
struct ScriptValue
{
	enum class Type : uint8_t
	{
		Int,
		Str
	};

	int32_t num		= 0;
	const char* str = nullptr;
	Type type		= Type::Int;

	static ScriptValue from_int(int32_t v) { return {v, nullptr, Type::Int}; }
	static ScriptValue from_str(const char* s) { return {0, s, Type::Str}; }

	bool is_truthy() const { return type == Type::Int ? num != 0 : (str != nullptr && str[0] != '\0'); }
};

/// Named variable in the script environment.
struct ScriptVar
{
	char name[16] = {};
	ScriptValue value;
	bool used = false;
};

/// The script engine. Parses and executes simple statements.
class ScriptEngine
{
  public:
	static constexpr uint8_t MAX_VARS = 32;

	explicit ScriptEngine(WidgetRegistry& registry) : registry_(registry) {}

	/// Execute a single line of script. Returns true on success.
	bool exec(const char* line)
	{
		skip_ws(line);
		if (*line == '\0' || *line == '#')
			return true; // Empty or comment

		// if statement
		if (match(line, "if "))
			return exec_if(line);

		// Assignment or expression
		return exec_assignment(line);
	}

	/// Execute a multi-line script (newline-separated).
	bool exec_block(const char* script)
	{
		char line_buf[128];
		while (*script)
		{
			// Extract one line
			uint8_t len = 0;
			while (*script && *script != '\n' && len < sizeof(line_buf) - 1)
				line_buf[len++] = *script++;
			line_buf[len] = '\0';
			if (*script == '\n')
				++script;

			if (!exec(line_buf))
				return false;
		}
		return true;
	}

	/// Get a variable value by name.
	ScriptValue get_var(const char* name) const
	{
		for (uint8_t i = 0; i < MAX_VARS; ++i)
		{
			if (vars_[i].used && std::strcmp(vars_[i].name, name) == 0)
				return vars_[i].value;
		}
		return ScriptValue::from_int(0);
	}

	/// Set a variable value.
	void set_var(const char* name, ScriptValue val)
	{
		// Find existing
		for (uint8_t i = 0; i < MAX_VARS; ++i)
		{
			if (vars_[i].used && std::strcmp(vars_[i].name, name) == 0)
			{
				vars_[i].value = val;
				return;
			}
		}
		// Create new
		for (uint8_t i = 0; i < MAX_VARS; ++i)
		{
			if (!vars_[i].used)
			{
				std::strncpy(vars_[i].name, name, sizeof(vars_[i].name) - 1);
				vars_[i].value = val;
				vars_[i].used  = true;
				return;
			}
		}
	}

  private:
	WidgetRegistry& registry_;
	ScriptVar vars_[MAX_VARS]{};

	// --- Parsing helpers ---
	static void skip_ws(const char*& p)
	{
		while (*p == ' ' || *p == '\t')
			++p;
	}

	static bool match(const char*& p, const char* keyword)
	{
		size_t len = std::strlen(keyword);
		if (std::strncmp(p, keyword, len) == 0)
		{
			p += len;
			return true;
		}
		return false;
	}

	static bool is_ident_char(char c)
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
	}

	static bool is_digit(char c) { return c >= '0' && c <= '9'; }

	// Read an identifier into buf, return length
	static uint8_t read_ident(const char*& p, char* buf, uint8_t max_len)
	{
		uint8_t len = 0;
		while (is_ident_char(*p) && len < max_len - 1)
			buf[len++] = *p++;
		buf[len] = '\0';
		return len;
	}

	// --- Expression evaluation ---
	// Simple recursive descent: comparison > additive > multiplicative > unary > primary

	ScriptValue eval_expr(const char*& p) { return eval_logic_or(p); }

	ScriptValue eval_logic_or(const char*& p)
	{
		ScriptValue left = eval_logic_and(p);
		skip_ws(p);
		while (p[0] == '|' && p[1] == '|')
		{
			p += 2;
			ScriptValue right = eval_logic_and(p);
			left			  = ScriptValue::from_int(left.is_truthy() || right.is_truthy() ? 1 : 0);
			skip_ws(p);
		}
		return left;
	}

	ScriptValue eval_logic_and(const char*& p)
	{
		ScriptValue left = eval_comparison(p);
		skip_ws(p);
		while (p[0] == '&' && p[1] == '&')
		{
			p += 2;
			ScriptValue right = eval_comparison(p);
			left			  = ScriptValue::from_int(left.is_truthy() && right.is_truthy() ? 1 : 0);
			skip_ws(p);
		}
		return left;
	}

	ScriptValue eval_comparison(const char*& p)
	{
		ScriptValue left = eval_additive(p);
		skip_ws(p);
		if (p[0] == '=' && p[1] == '=')
		{
			p += 2;
			ScriptValue right = eval_additive(p);
			return ScriptValue::from_int(left.num == right.num ? 1 : 0);
		}
		if (p[0] == '!' && p[1] == '=')
		{
			p += 2;
			ScriptValue right = eval_additive(p);
			return ScriptValue::from_int(left.num != right.num ? 1 : 0);
		}
		if (p[0] == '<' && p[1] == '=')
		{
			p += 2;
			ScriptValue right = eval_additive(p);
			return ScriptValue::from_int(left.num <= right.num ? 1 : 0);
		}
		if (p[0] == '>' && p[1] == '=')
		{
			p += 2;
			ScriptValue right = eval_additive(p);
			return ScriptValue::from_int(left.num >= right.num ? 1 : 0);
		}
		if (p[0] == '<')
		{
			++p;
			ScriptValue right = eval_additive(p);
			return ScriptValue::from_int(left.num < right.num ? 1 : 0);
		}
		if (p[0] == '>')
		{
			++p;
			ScriptValue right = eval_additive(p);
			return ScriptValue::from_int(left.num > right.num ? 1 : 0);
		}
		return left;
	}

	ScriptValue eval_additive(const char*& p)
	{
		ScriptValue left = eval_multiplicative(p);
		skip_ws(p);
		while (*p == '+' || *p == '-')
		{
			char op			  = *p++;
			ScriptValue right = eval_multiplicative(p);
			if (op == '+')
				left.num += right.num;
			else
				left.num -= right.num;
			skip_ws(p);
		}
		return left;
	}

	ScriptValue eval_multiplicative(const char*& p)
	{
		ScriptValue left = eval_unary(p);
		skip_ws(p);
		while (*p == '*' || *p == '/' || *p == '%')
		{
			char op			  = *p++;
			ScriptValue right = eval_unary(p);
			if (op == '*')
				left.num *= right.num;
			else if (op == '/' && right.num != 0)
				left.num /= right.num;
			else if (op == '%' && right.num != 0)
				left.num %= right.num;
			skip_ws(p);
		}
		return left;
	}

	ScriptValue eval_unary(const char*& p)
	{
		skip_ws(p);
		if (*p == '!')
		{
			++p;
			ScriptValue val = eval_unary(p);
			return ScriptValue::from_int(val.is_truthy() ? 0 : 1);
		}
		if (*p == '-')
		{
			++p;
			ScriptValue val = eval_primary(p);
			val.num			= -val.num;
			return val;
		}
		return eval_primary(p);
	}

	ScriptValue eval_primary(const char*& p)
	{
		skip_ws(p);

		// Number literal
		if (is_digit(*p))
		{
			int32_t num = 0;
			while (is_digit(*p))
				num = num * 10 + (*p++ - '0');
			return ScriptValue::from_int(num);
		}

		// String literal
		if (*p == '"')
		{
			++p;
			const char* start = p;
			while (*p && *p != '"')
				++p;
			// Store string in static buffer (limited, single string at a time)
			static char str_buf[64];
			uint8_t len = 0;
			while (start < p && len < sizeof(str_buf) - 1)
				str_buf[len++] = *start++;
			str_buf[len] = '\0';
			if (*p == '"')
				++p;
			return ScriptValue::from_str(str_buf);
		}

		// Parenthesized expression
		if (*p == '(')
		{
			++p;
			ScriptValue val = eval_expr(p);
			skip_ws(p);
			if (*p == ')')
				++p;
			return val;
		}

		// Identifier (variable or widget.property)
		if (is_ident_char(*p))
		{
			char name[32];
			read_ident(p, name, sizeof(name));
			skip_ws(p);

			// Check for widget.property (dot notation)
			if (*p == '.')
			{
				++p;
				char prop[16];
				read_ident(p, prop, sizeof(prop));
				// Read widget property as int (value for progressbar, etc.)
				// For now, return 0 — full property read needs registry extension
				return ScriptValue::from_int(0);
			}

			// Variable lookup
			return get_var(name);
		}

		return ScriptValue::from_int(0);
	}

	// --- Statement execution ---

	bool exec_assignment(const char*& p)
	{
		skip_ws(p);

		// Read LHS (variable name or widget.property)
		char lhs[32];
		read_ident(p, lhs, sizeof(lhs));
		skip_ws(p);

		// Check for widget.property
		char prop[16]		= {};
		bool is_widget_prop = false;
		if (*p == '.')
		{
			++p;
			read_ident(p, prop, sizeof(prop));
			is_widget_prop = true;
			skip_ws(p);
		}

		// Assignment operator
		char op = '=';
		if (p[0] == '+' && p[1] == '=')
		{
			op = '+';
			p += 2;
		}
		else if (p[0] == '-' && p[1] == '=')
		{
			op = '-';
			p += 2;
		}
		else if (p[0] == '*' && p[1] == '=')
		{
			op = '*';
			p += 2;
		}
		else if (p[0] == '=')
		{
			op = '=';
			++p;
		}
		else
		{
			return false; // Not an assignment
		}

		skip_ws(p);
		ScriptValue rhs = eval_expr(p);

		if (is_widget_prop)
		{
			// Write to widget property
			char val_buf[64];
			if (rhs.type == ScriptValue::Type::Str && rhs.str)
			{
				registry_.set_property(lhs, prop, rhs.str);
			}
			else
			{
				int_to_str(val_buf, rhs.num);
				registry_.set_property(lhs, prop, val_buf);
			}
		}
		else
		{
			// Variable assignment
			ScriptValue current = get_var(lhs);
			switch (op)
			{
			case '+':
				rhs.num = current.num + rhs.num;
				break;
			case '-':
				rhs.num = current.num - rhs.num;
				break;
			case '*':
				rhs.num = current.num * rhs.num;
				break;
			default:
				break;
			}
			set_var(lhs, rhs);
		}

		return true;
	}

	bool exec_if(const char*& p)
	{
		skip_ws(p);
		ScriptValue cond = eval_expr(p);
		skip_ws(p);

		// Expect '{'
		if (*p != '{')
			return false;
		++p;

		if (cond.is_truthy())
		{
			// Execute body
			while (*p && *p != '}')
			{
				skip_ws(p);
				if (*p == '}')
					break;

				// Execute one statement (up to newline or semicolon or '}')
				char stmt[128];
				uint8_t len = 0;
				while (*p && *p != '\n' && *p != ';' && *p != '}' && len < sizeof(stmt) - 1)
					stmt[len++] = *p++;
				stmt[len] = '\0';
				if (*p == '\n' || *p == ';')
					++p;

				const char* sp = stmt;
				exec_assignment(sp);
			}
			if (*p == '}')
				++p;

			// Skip else block if present
			skip_ws(p);
			if (match(p, "else"))
			{
				skip_ws(p);
				if (*p == '{')
				{
					++p;
					int depth = 1;
					while (*p && depth > 0)
					{
						if (*p == '{')
							++depth;
						if (*p == '}')
							--depth;
						++p;
					}
				}
			}
		}
		else
		{
			// Skip true block
			int depth = 1;
			while (*p && depth > 0)
			{
				if (*p == '{')
					++depth;
				if (*p == '}')
					--depth;
				++p;
			}

			// Check for else
			skip_ws(p);
			if (match(p, "else"))
			{
				skip_ws(p);
				if (*p == '{')
				{
					++p;
					while (*p && *p != '}')
					{
						skip_ws(p);
						if (*p == '}')
							break;
						char stmt[128];
						uint8_t len = 0;
						while (*p && *p != '\n' && *p != ';' && *p != '}' && len < sizeof(stmt) - 1)
							stmt[len++] = *p++;
						stmt[len] = '\0';
						if (*p == '\n' || *p == ';')
							++p;
						const char* sp = stmt;
						exec_assignment(sp);
					}
					if (*p == '}')
						++p;
				}
			}
		}

		return true;
	}

	static void int_to_str(char* buf, int32_t val)
	{
		if (val == 0)
		{
			buf[0] = '0';
			buf[1] = '\0';
			return;
		}
		if (val < 0)
		{
			*buf++ = '-';
			val	   = -val;
		}
		char tmp[12];
		int len = 0;
		while (val > 0)
		{
			tmp[len++] = '0' + (val % 10);
			val /= 10;
		}
		for (int i = len - 1; i >= 0; --i)
			*buf++ = tmp[i];
		*buf = '\0';
	}
};

} // namespace lumen::ui

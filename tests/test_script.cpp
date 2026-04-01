/// Unit tests for the ScriptEngine.

#include <cstring>

#include "lumen/ui/script.hpp"
#include "lumen/ui/widget_registry.hpp"
#include "lumen/ui/widgets/label.hpp"
#include "lumen/ui/widgets/progress_bar.hpp"
#include "tests/test.hpp"

using namespace lumen::ui;

static WidgetRegistry make_reg()
{
	return {};
}

// --- Variable Assignment ---

TEST(assign_int)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 42");
	EXPECT_EQ(eng.get_var("x").num, 42);
}

TEST(assign_zero)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 0");
	EXPECT_EQ(eng.get_var("x").num, 0);
}

TEST(assign_negative)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = -10");
	EXPECT_EQ(eng.get_var("x").num, -10);
}

// --- Arithmetic ---

TEST(add)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 3 + 7");
	EXPECT_EQ(eng.get_var("x").num, 10);
}

TEST(subtract)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 20 - 8");
	EXPECT_EQ(eng.get_var("x").num, 12);
}

TEST(multiply)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 6 * 7");
	EXPECT_EQ(eng.get_var("x").num, 42);
}

TEST(divide)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 100 / 4");
	EXPECT_EQ(eng.get_var("x").num, 25);
}

TEST(modulo)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 10 % 3");
	EXPECT_EQ(eng.get_var("x").num, 1);
}

TEST(parentheses)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = (2 + 3) * 4");
	EXPECT_EQ(eng.get_var("x").num, 20);
}

TEST(precedence)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 2 + 3 * 4");
	EXPECT_EQ(eng.get_var("x").num, 14);
}

// --- Compound Assignment ---

TEST(plus_eq)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 10");
	eng.exec("x += 5");
	EXPECT_EQ(eng.get_var("x").num, 15);
}

TEST(minus_eq)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 10");
	eng.exec("x -= 3");
	EXPECT_EQ(eng.get_var("x").num, 7);
}

TEST(times_eq)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 5");
	eng.exec("x *= 4");
	EXPECT_EQ(eng.get_var("x").num, 20);
}

// --- Comparison ---

TEST(eq_true)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 5 == 5");
	EXPECT_EQ(eng.get_var("x").num, 1);
}

TEST(eq_false)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 5 == 3");
	EXPECT_EQ(eng.get_var("x").num, 0);
}

TEST(neq)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 5 != 3");
	EXPECT_EQ(eng.get_var("x").num, 1);
}

TEST(less_than)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 3 < 5");
	EXPECT_EQ(eng.get_var("x").num, 1);
}

TEST(greater_than)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 5 > 3");
	EXPECT_EQ(eng.get_var("x").num, 1);
}

TEST(less_eq)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 5 <= 5");
	EXPECT_EQ(eng.get_var("x").num, 1);
}

TEST(greater_eq_false)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 5 >= 6");
	EXPECT_EQ(eng.get_var("x").num, 0);
}

// --- Logic ---

TEST(and_true)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 1 && 1");
	EXPECT_EQ(eng.get_var("x").num, 1);
}

TEST(and_false)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 1 && 0");
	EXPECT_EQ(eng.get_var("x").num, 0);
}

TEST(or_true)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 0 || 1");
	EXPECT_EQ(eng.get_var("x").num, 1);
}

TEST(not_op)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = !0");
	EXPECT_EQ(eng.get_var("x").num, 1);
}

// --- If/Else ---

TEST(if_true)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 10");
	eng.exec("if x > 5 { x = 99 }");
	EXPECT_EQ(eng.get_var("x").num, 99);
}

TEST(if_false)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 3");
	eng.exec("if x > 5 { x = 99 }");
	EXPECT_EQ(eng.get_var("x").num, 3);
}

TEST(if_else)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("x = 3");
	eng.exec("if x > 5 { x = 99 } else { x = 1 }");
	EXPECT_EQ(eng.get_var("x").num, 1);
}

// --- Variable Chaining ---

TEST(var_chain)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec("a = 10");
	eng.exec("b = a + 5");
	eng.exec("c = a * b");
	EXPECT_EQ(eng.get_var("c").num, 150);
}

// --- Widget Property ---

TEST(widget_set_value)
{
	WidgetRegistry reg;
	ProgressBar bar;
	bar.set_bounds({0, 0, 100, 20});
	reg.add("bar", bar);

	ScriptEngine eng(reg);
	eng.exec("bar.value = 75");
	EXPECT_EQ(bar.value(), 75);
}

TEST(widget_set_text)
{
	WidgetRegistry reg;
	Label label;
	label.set_bounds({0, 0, 100, 20});
	reg.add("lbl", label);

	ScriptEngine eng(reg);
	eng.exec("lbl.text = \"Hello\"");
	EXPECT_STREQ(label.text(), "Hello");
}

// --- Multi-statement ---

TEST(multi_line)
{
	auto reg = make_reg();
	ScriptEngine eng(reg);
	eng.exec_block("x = 1\ny = 2\nz = x + y");
	EXPECT_EQ(eng.get_var("z").num, 3);
}

int main()
{
	return lumen_test::run_all();
}

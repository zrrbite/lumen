#pragma once

/// Lumen test framework — lightweight, single-header.
/// No dependencies, no heap, works on desktop and embedded.
///
/// Usage:
///   TEST(name) { EXPECT_EQ(1 + 1, 2); }
///   int main() { return lumen_test::run_all(); }

#include <cstdio>

namespace lumen_test {

struct TestCase
{
	const char* name;
	void (*func)();
	TestCase* next;
};

inline TestCase*& head()
{
	static TestCase* h = nullptr;
	return h;
}

inline int& pass_count()
{
	static int c = 0;
	return c;
}

inline int& fail_count()
{
	static int c = 0;
	return c;
}

inline const char*& current_test()
{
	static const char* n = nullptr;
	return n;
}

struct TestRegistrar
{
	TestRegistrar(const char* name, void (*func)())
	{
		static TestCase cases[256];
		static int count = 0;
		if (count < 256)
		{
			cases[count] = {name, func, head()};
			head()		 = &cases[count];
			++count;
		}
	}
};

inline void check(bool condition, const char* expr, const char* file, int line)
{
	if (!condition)
	{
		printf("    FAIL: %s (%s:%d)\n", expr, file, line);
		fail_count()++;
	}
	else
	{
		pass_count()++;
	}
}

inline int run_all()
{
	int total	   = 0;
	int test_fails = 0;

	for (TestCase* tc = head(); tc != nullptr; tc = tc->next)
	{
		current_test() = tc->name;
		int before	   = fail_count();
		tc->func();
		bool ok = (fail_count() == before);
		printf("  %s  %s\n", ok ? "PASS" : "FAIL", tc->name);
		if (!ok)
			++test_fails;
		++total;
	}

	printf("\n%d/%d tests passed (%d assertions, %d failures)\n",
		   total - test_fails,
		   total,
		   pass_count() + fail_count(),
		   fail_count());
	return test_fails == 0 ? 0 : 1;
}

} // namespace lumen_test

#define TEST(name)                                                   \
	static void test_##name();                                       \
	static lumen_test::TestRegistrar reg_##name(#name, test_##name); \
	static void test_##name()

#define EXPECT_TRUE(expr)  lumen_test::check((expr), #expr, __FILE__, __LINE__)
#define EXPECT_FALSE(expr) lumen_test::check(!(expr), "!" #expr, __FILE__, __LINE__)
#define EXPECT_EQ(a, b)	   lumen_test::check((a) == (b), #a " == " #b, __FILE__, __LINE__)
#define EXPECT_NE(a, b)	   lumen_test::check((a) != (b), #a " != " #b, __FILE__, __LINE__)
#define EXPECT_GT(a, b)	   lumen_test::check((a) > (b), #a " > " #b, __FILE__, __LINE__)
#define EXPECT_LT(a, b)	   lumen_test::check((a) < (b), #a " < " #b, __FILE__, __LINE__)
#define EXPECT_GE(a, b)	   lumen_test::check((a) >= (b), #a " >= " #b, __FILE__, __LINE__)
#define EXPECT_LE(a, b)	   lumen_test::check((a) <= (b), #a " <= " #b, __FILE__, __LINE__)
#define EXPECT_STREQ(a, b) lumen_test::check(strcmp((a), (b)) == 0, #a " streq " #b, __FILE__, __LINE__)

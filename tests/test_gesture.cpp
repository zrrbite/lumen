/// Unit tests for GestureDetector.

#include "lumen/ui/gesture.hpp"
#include "tests/test.hpp"

using namespace lumen::ui;

static GestureType last_gesture = GestureType::None;

static void on_gesture(const GestureEvent& evt)
{
	last_gesture = evt.type;
}

static void reset()
{
	last_gesture = GestureType::None;
}

TEST(tap)
{
	reset();
	GestureDetector det;
	det.set_on_gesture(on_gesture);

	det.on_touch({TouchEvent::Type::Press, {100, 100}}, 0);
	det.on_touch({TouchEvent::Type::Release, {100, 100}}, 50);

	EXPECT_EQ(static_cast<int>(last_gesture), static_cast<int>(GestureType::Tap));
}

TEST(long_press)
{
	reset();
	GestureDetector det;
	det.set_on_gesture(on_gesture);

	det.on_touch({TouchEvent::Type::Press, {100, 100}}, 0);
	det.update(600); // Past 500ms threshold

	EXPECT_EQ(static_cast<int>(last_gesture), static_cast<int>(GestureType::LongPress));
}

TEST(swipe_right)
{
	reset();
	GestureDetector det;
	det.set_on_gesture(on_gesture);

	det.on_touch({TouchEvent::Type::Press, {50, 100}}, 0);
	det.on_touch({TouchEvent::Type::Move, {150, 105}}, 100);
	det.on_touch({TouchEvent::Type::Release, {150, 105}}, 150);

	EXPECT_EQ(static_cast<int>(last_gesture), static_cast<int>(GestureType::SwipeRight));
}

TEST(swipe_left)
{
	reset();
	GestureDetector det;
	det.set_on_gesture(on_gesture);

	det.on_touch({TouchEvent::Type::Press, {150, 100}}, 0);
	det.on_touch({TouchEvent::Type::Move, {50, 100}}, 100);
	det.on_touch({TouchEvent::Type::Release, {50, 100}}, 150);

	EXPECT_EQ(static_cast<int>(last_gesture), static_cast<int>(GestureType::SwipeLeft));
}

TEST(swipe_down)
{
	reset();
	GestureDetector det;
	det.set_on_gesture(on_gesture);

	det.on_touch({TouchEvent::Type::Press, {100, 50}}, 0);
	det.on_touch({TouchEvent::Type::Move, {105, 150}}, 100);
	det.on_touch({TouchEvent::Type::Release, {105, 150}}, 150);

	EXPECT_EQ(static_cast<int>(last_gesture), static_cast<int>(GestureType::SwipeDown));
}

TEST(swipe_up)
{
	reset();
	GestureDetector det;
	det.set_on_gesture(on_gesture);

	det.on_touch({TouchEvent::Type::Press, {100, 150}}, 0);
	det.on_touch({TouchEvent::Type::Move, {100, 50}}, 100);
	det.on_touch({TouchEvent::Type::Release, {100, 50}}, 150);

	EXPECT_EQ(static_cast<int>(last_gesture), static_cast<int>(GestureType::SwipeUp));
}

TEST(no_long_press_after_move)
{
	reset();
	GestureDetector det;
	det.set_on_gesture(on_gesture);

	det.on_touch({TouchEvent::Type::Press, {100, 100}}, 0);
	det.on_touch({TouchEvent::Type::Move, {200, 100}}, 100);
	det.update(600); // Would be long press, but finger moved

	EXPECT_NE(static_cast<int>(last_gesture), static_cast<int>(GestureType::LongPress));
}

TEST(tap_not_fired_after_long_press)
{
	reset();
	GestureDetector det;
	det.set_on_gesture(on_gesture);

	det.on_touch({TouchEvent::Type::Press, {100, 100}}, 0);
	det.update(600); // Long press fires
	EXPECT_EQ(static_cast<int>(last_gesture), static_cast<int>(GestureType::LongPress));

	reset();
	det.on_touch({TouchEvent::Type::Release, {100, 100}}, 700);
	EXPECT_NE(static_cast<int>(last_gesture), static_cast<int>(GestureType::Tap));
}

int main()
{
	return lumen_test::run_all();
}

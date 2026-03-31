#pragma once

#include "lumen/ui/container.hpp"

namespace lumen::ui {

/// Screen — the root container representing a full page/view.
/// Subclass this for each screen in your app.
class Screen : public Container
{
  public:
	/// Called when this screen becomes active.
	virtual void on_enter() {}

	/// Called when leaving this screen.
	virtual void on_leave() {}

	/// Called once per render cycle before drawing.
	/// Pull data from your model and update widget properties here.
	virtual void update_model() {}
};

} // namespace lumen::ui

#include "native_click_through.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "platform/windows/windows_headers.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace godot {

struct NativeClickThrough::Impl {
	struct WinPoint {
		LONG x;
		LONG y;
	};

	struct CachedPolygon {
		std::vector<WinPoint> points;
	};

	HWND hwnd = nullptr;

	bool window_transparent = false;

	std::unordered_map<uint64_t, CachedPolygon> polygons;

	bool initialize();

	HWND find_window();

	bool is_cursor_inside_polygon();

	void set_window_transparent(bool enabled);
};

NativeClickThrough::NativeClickThrough() : impl_(std::make_unique<Impl>()) {
}

NativeClickThrough::~NativeClickThrough() = default;

void NativeClickThrough::_bind_methods() {
	ClassDB::bind_method(D_METHOD("enable"), &NativeClickThrough::enable);
	ClassDB::bind_method(D_METHOD("disable"), &NativeClickThrough::disable);
	ClassDB::bind_method(D_METHOD("update"), &NativeClickThrough::update);
	ClassDB::bind_method(D_METHOD("debug"), &NativeClickThrough::debug);
	ClassDB::bind_method(D_METHOD("is_supported"), &NativeClickThrough::is_supported);
	ClassDB::bind_method(D_METHOD("set_polygon", "id", "polygon"), &NativeClickThrough::set_polygon);
	ClassDB::bind_method(D_METHOD("remove_polygon", "id"), &NativeClickThrough::remove_polygon);
	ClassDB::bind_method(D_METHOD("clear"), &NativeClickThrough::clear);
	ClassDB::bind_method(D_METHOD("debug_mouse"), &NativeClickThrough::debug_mouse);
}

Dictionary NativeClickThrough::debug_mouse() {
	Dictionary result;

	result["valid"] = false;
	result["error"] = "";

	if (!impl_->initialize()) {
		result["error"] =
				"Could not initialize the native Windows handle.";

		return result;
	}

	POINT screen_point{};

	if (!GetCursorPos(&screen_point)) {
		result["error"] = "GetCursorPos() failed.";
		return result;
	}

	POINT client_point = screen_point;

	if (!ScreenToClient(impl_->hwnd, &client_point)) {
		result["error"] = "ScreenToClient() failed.";
		return result;
	}

	RECT client_rect{};

	if (!GetClientRect(impl_->hwnd, &client_rect)) {
		result["error"] = "GetClientRect() failed.";
		return result;
	}

	RECT window_rect{};

	if (!GetWindowRect(impl_->hwnd, &window_rect)) {
		result["error"] = "GetWindowRect() failed.";
		return result;
	}

	result["valid"] = true;

	result["screen_mouse"] = Vector2i(
			static_cast<int32_t>(screen_point.x),
			static_cast<int32_t>(screen_point.y));

	result["client_mouse"] = Vector2i(
			static_cast<int32_t>(client_point.x),
			static_cast<int32_t>(client_point.y));

	result["client_size"] = Vector2i(
			static_cast<int32_t>(
					client_rect.right - client_rect.left),
			static_cast<int32_t>(
					client_rect.bottom - client_rect.top));

	result["window_position"] = Vector2i(
			static_cast<int32_t>(window_rect.left),
			static_cast<int32_t>(window_rect.top));

	result["window_size"] = Vector2i(
			static_cast<int32_t>(
					window_rect.right - window_rect.left),
			static_cast<int32_t>(
					window_rect.bottom - window_rect.top));

	return result;
}

bool NativeClickThrough::is_supported() {
#ifdef _WIN32
	return true;
#else
	return false;
#endif
}

bool NativeClickThrough::enable() {
	if (impl_->hwnd != nullptr) {
		return true;
	}

	if (!impl_->initialize()) {
		UtilityFunctions::print("Initialize failed");
		return false;
	}

	impl_->set_window_transparent(false);

	UtilityFunctions::print("Initialize success");

	return true;
}

void NativeClickThrough::disable() {
	if (impl_->hwnd == nullptr) {
		return;
	}

	impl_->set_window_transparent(false);

	impl_->hwnd = nullptr;
}

void NativeClickThrough::update() {
	if (impl_->hwnd == nullptr) {
		return;
	}

	const bool inside = impl_->is_cursor_inside_polygon();

	impl_->set_window_transparent(!inside);
}

void NativeClickThrough::set_polygon(
		uint64_t id,
		const PackedVector2Array &polygon) {
	Impl::CachedPolygon cache;

	cache.points.reserve(polygon.size());

	for (int64_t i = 0; i < polygon.size(); ++i) {
		const Vector2 &p = polygon[i];

		cache.points.push_back({ static_cast<LONG>(p.x), static_cast<LONG>(p.y) });
	}

	impl_->polygons[id] = std::move(cache);

	// UtilityFunctions::print(
	// 		"Polygon ",
	// 		static_cast<int64_t>(id),
	// 		" cached with ",
	// 		polygon.size(),
	// 		" points.");
}

void NativeClickThrough::remove_polygon(uint64_t id) {
	impl_->polygons.erase(id);
}

void NativeClickThrough::clear() {
	impl_->polygons.clear();
}

bool NativeClickThrough::debug() {
	return impl_->is_cursor_inside_polygon();
}

bool NativeClickThrough::Impl::initialize() {
	if (hwnd != nullptr)
		return true;

	hwnd = find_window();

	if (!hwnd)
		return false;

	return true;
}

HWND NativeClickThrough::Impl::find_window() {
	auto *display_server = DisplayServer::get_singleton();

	if (display_server == nullptr)
		return nullptr;

	auto handle = display_server->window_get_native_handle(
			DisplayServer::WINDOW_HANDLE,
			DisplayServer::MAIN_WINDOW_ID);

	UtilityFunctions::print(
			"Native Handle = ",
			static_cast<uint64_t>(handle));

	HWND hwnd = reinterpret_cast<HWND>(handle);

	char class_name[256] = {};
	GetClassNameA(hwnd, class_name, sizeof(class_name));

	char title[256] = {};
	GetWindowTextA(hwnd, title, sizeof(title));

	UtilityFunctions::print("Class = ", class_name);
	UtilityFunctions::print("Title = ", title);

	return hwnd;
}

bool NativeClickThrough::Impl::is_cursor_inside_polygon() {
	if (hwnd == nullptr)
		return false;

	POINT cursor_pos;

	if (!GetCursorPos(&cursor_pos))
		return false;

	if (!ScreenToClient(hwnd, &cursor_pos))
		return false;

	for (const auto &pair : polygons) {
		const CachedPolygon &polygon = pair.second;

		if (polygon.points.size() < 3)
			continue;

		size_t j = polygon.points.size() - 1;

		bool inside = false;

		for (size_t i = 0; i < polygon.points.size(); j = i++) {
			const WinPoint &a = polygon.points[i];
			const WinPoint &b = polygon.points[j];

			const bool crosses_vertical_range = ((a.y > cursor_pos.y) != (b.y > cursor_pos.y));

			if (!crosses_vertical_range)
				continue;

			const double intersection_x =
					static_cast<double>(b.x - a.x) *
							static_cast<double>(cursor_pos.y - a.y) /
							static_cast<double>(b.y - a.y) +
					static_cast<double>(a.x);

			if (static_cast<double>(cursor_pos.x) < intersection_x)
				inside = !inside;
		}

		if (inside) {
			// UtilityFunctions::print(
			// 		"Cursor is inside polygon ",
			// 		static_cast<int64_t>(pair.first));
			return true;
		}
	}

	return false;
}

void NativeClickThrough::Impl::set_window_transparent(bool transparent) {
	if (hwnd == nullptr)
		return;

	if (window_transparent == transparent)
		return;

	window_transparent = transparent;

	LONG_PTR old_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

	// UtilityFunctions::print("Old EXSTYLE = ", (uint64_t)old_style);

	LONG_PTR new_style = old_style;

	if (transparent) {
		new_style |= WS_EX_TRANSPARENT;
		new_style |= WS_EX_LAYERED;
	} else {
		new_style &= ~WS_EX_TRANSPARENT;
	}

	SetWindowLongPtr(hwnd, GWL_EXSTYLE, new_style);

	LONG_PTR verify_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

	// UtilityFunctions::print("New EXSTYLE = ", (uint64_t)verify_style);

	// UtilityFunctions::print(
	// 		"ClickThrough = ",
	// 		transparent ? "ON" : "OFF");
}

} //namespace godot
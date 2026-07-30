#pragma once

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <memory>

namespace godot {

class NativeClickThrough : public Object {
	GDCLASS(NativeClickThrough, Object);

public:
	NativeClickThrough();
	~NativeClickThrough();

	bool is_supported();

	bool debug();

	bool enable();
	void disable();

	void update();

	void set_polygon(
			uint64_t id,
			const PackedVector2Array &polygon);
	void remove_polygon(uint64_t id);

	void clear();

	Dictionary debug_mouse();

private:
	static void _bind_methods();

	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace godot
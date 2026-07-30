extends Polygon2D

signal polygon_changed

var dragging := false
var drag_offset := Vector2.ZERO


func _process(_delta: float) -> void:
	var mouse_position := get_viewport().get_mouse_position()

	var cursor_inside := Geometry2D.is_point_in_polygon(
		to_local(mouse_position),
		polygon
	)

	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
		if not dragging and cursor_inside:
			dragging = true
			drag_offset = global_position - mouse_position

		if dragging:
			global_position = mouse_position + drag_offset

			# Notify Main that native polygon coordinates changed.
			polygon_changed.emit()
	else:
		dragging = false

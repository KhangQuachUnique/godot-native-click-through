extends Node2D

const POLYGON_ID := 1
const UPDATE_RATE := 60.0

@onready var polygon: Polygon2D = $DraggablePolygon

var native := NativeClickThrough.new()


func _ready() -> void:
	native.enable()

	polygon.polygon_changed.connect(_sync_polygon)

	_sync_polygon()
	_start_update_timer()


func _start_update_timer() -> void:
	var timer := Timer.new()

	timer.wait_time = 1.0 / UPDATE_RATE
	timer.timeout.connect(native.update)

	add_child(timer)
	timer.start()


func _sync_polygon() -> void:
	var window_points := PackedVector2Array()

	# Convert the local polygon points to window coordinates.
	for local_point in polygon.polygon:
		window_points.append(polygon.to_global(local_point))

	native.set_polygon(POLYGON_ID, window_points)


func _exit_tree() -> void:
	native.remove_polygon(POLYGON_ID)
	native.disable()

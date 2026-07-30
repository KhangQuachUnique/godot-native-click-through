# Native Click Through

`NativeClickThrough` is a Windows GDExtension for Godot 4 that allows parts of a transparent Godot window to receive mouse input while the rest of the window remains click-through.

Godot defines the interactive regions as polygons. The extension stores those polygons, checks the current mouse position with the Win32 API, and toggles the native window's click-through state.

## Features

- Native Windows click-through control
- Supports one or more clickable polygons
- Polygon data is managed from GDScript
- Works with moving or animated objects
- Does not use `mouse_passthrough_polygon`
- Does not use `WM_NCHITTEST`
- Does not require an `EditorPlugin`
- Includes debug and release DLLs for Windows x86_64

## Supported Platforms

- Windows x86_64
- Godot 4.x

The included binaries are built for Windows x86_64 only.

## Installation

Copy the entire `native_click_through` folder into your Godot project's `addons` directory.

Your project should contain:

```text
your_project/
└── addons/
    └── native_click_through/
        ├── bin/
        │   └── windows/
        │       ├── native_click_through.windows.template_debug.x86_64.dll
        │       └── native_click_through.windows.template_release.x86_64.dll
        ├── native_click_through.gdextension
        ├── README.md
        └── LICENSE
```

Godot loads the `.gdextension` file automatically.

This addon does not require a `plugin.cfg` file and does not need to be enabled in:

```text
Project Settings → Plugins
```

## Basic Usage

```gdscript
extends Node

var click_through: NativeClickThrough


func _ready() -> void:
    click_through = NativeClickThrough.new()
    click_through.enable()


func _process(_delta: float) -> void:
    click_through.update()


func _exit_tree() -> void:
    click_through.clear()
    click_through.disable()
```

Calling `update()` checks whether the system cursor is inside any registered polygon.

- Cursor inside a polygon: the Godot window receives mouse input.
- Cursor outside every polygon: mouse input passes through the Godot window.

## Registering a Polygon

Each polygon requires a unique integer ID.

```gdscript
var polygon_id := 1

var points := PackedVector2Array([
    Vector2(100, 100),
    Vector2(300, 100),
    Vector2(300, 300),
    Vector2(100, 300),
])

click_through.set_polygon(polygon_id, points)
```

The polygon must contain at least three points.

Calling `set_polygon()` again with the same ID replaces the previous polygon data.

## Using a Polygon2D

The extension expects polygon points in window client coordinates.

For a `Polygon2D`, convert each local polygon point into global viewport coordinates before sending it to the extension.

```gdscript
extends Node2D

@onready var click_polygon: Polygon2D = $CharacterBody2D/ClickPolygon

var click_through: NativeClickThrough
var polygon_id := 1


func _ready() -> void:
    click_through = NativeClickThrough.new()
    click_through.enable()

    update_native_polygon()


func _process(_delta: float) -> void:
    click_through.update()


func update_native_polygon() -> void:
    var points := PackedVector2Array()

    for local_point in click_polygon.polygon:
        points.append(click_polygon.to_global(local_point))

    click_through.set_polygon(polygon_id, points)


func _exit_tree() -> void:
    click_through.remove_polygon(polygon_id)
    click_through.disable()
```

Update the native polygon whenever the object moves, rotates, scales, or its polygon shape changes.

Do not resend unchanged polygon data every frame unless necessary.

## Moving Object Example

```gdscript
extends CharacterBody2D

@onready var click_polygon: Polygon2D = $ClickPolygon

var click_through: NativeClickThrough
var polygon_id := 1


func _ready() -> void:
    click_through = NativeClickThrough.new()
    click_through.enable()
    sync_polygon()


func _physics_process(_delta: float) -> void:
    move_and_slide()
    sync_polygon()


func _process(_delta: float) -> void:
    click_through.update()


func sync_polygon() -> void:
    var points := PackedVector2Array()

    for local_point in click_polygon.polygon:
        points.append(click_polygon.to_global(local_point))

    click_through.set_polygon(polygon_id, points)


func _exit_tree() -> void:
    click_through.remove_polygon(polygon_id)
    click_through.disable()
```

For better performance, cache the last transform and only call `set_polygon()` when the transform or polygon data changes.

## Multiple Interactive Regions

Register multiple polygons with different IDs:

```gdscript
click_through.set_polygon(1, first_polygon)
click_through.set_polygon(2, second_polygon)
click_through.set_polygon(3, third_polygon)
```

The window receives mouse input when the cursor is inside any registered polygon.

## Removing Polygon Data

Remove one polygon:

```gdscript
click_through.remove_polygon(1)
```

Remove all polygons:

```gdscript
click_through.clear()
```

## Enable and Disable

Enable native click-through handling:

```gdscript
click_through.enable()
```

Disable native click-through handling:

```gdscript
click_through.disable()
```

Call `disable()` before the node or application exits so the native window style is restored.

## Recommended Update Rate

`update()` does not need to run at the full render frame rate.

For many desktop overlay applications, a timer running at 30 to 60 Hz is enough.

```gdscript
extends Node

var click_through: NativeClickThrough
var update_timer: Timer


func _ready() -> void:
    click_through = NativeClickThrough.new()
    click_through.enable()

    update_timer = Timer.new()
    update_timer.wait_time = 1.0 / 60.0
    update_timer.timeout.connect(_on_update_timer_timeout)
    add_child(update_timer)
    update_timer.start()


func _on_update_timer_timeout() -> void:
    click_through.update()


func _exit_tree() -> void:
    click_through.clear()
    click_through.disable()
```

## API

### `enable()`

Initializes the native window handle and enables click-through management.

```gdscript
click_through.enable()
```

### `disable()`

Disables click-through management and restores the window to an interactive state.

```gdscript
click_through.disable()
```

### `set_polygon(id, points)`

Creates or replaces a cached polygon.

```gdscript
click_through.set_polygon(
    1,
    PackedVector2Array([
        Vector2(100, 100),
        Vector2(300, 100),
        Vector2(300, 300),
        Vector2(100, 300),
    ])
)
```

Parameters:

- `id`: unique integer identifier
- `points`: polygon vertices in window client coordinates

### `remove_polygon(id)`

Removes one cached polygon.

```gdscript
click_through.remove_polygon(1)
```

### `clear()`

Removes all cached polygons.

```gdscript
click_through.clear()
```

### `update()`

Checks the current cursor position and updates the native window's click-through state.

```gdscript
click_through.update()
```

## Coordinate System

Polygon points must use the same coordinate system as the Windows client area.

For the tested fullscreen borderless Godot window configuration, Godot viewport coordinates match Windows client coordinates directly.

Example fullscreen setup:

```gdscript
func setup_window() -> void:
    var screen := DisplayServer.screen_get_usable_rect(
        DisplayServer.window_get_current_screen()
    )

    get_window().position = screen.position
    get_window().size = screen.size
```

If your project uses viewport scaling, stretch modes, embedded subwindows, or custom content scaling, verify the coordinate conversion before registering polygons.

## Window Configuration

A typical transparent desktop overlay may use:

```gdscript
func setup_overlay_window() -> void:
    get_window().transparent = true
    get_window().borderless = true
    get_window().always_on_top = true
```

The exact window configuration depends on your project.

This addon controls native mouse click-through behavior only. It does not configure rendering transparency, window size, borderless mode, or always-on-top mode.

## Architecture

Runtime flow:

```text
Godot calculates polygon points
            ↓
set_polygon(id, points)
            ↓
Native extension caches polygons
            ↓
update() runs at 30–60 Hz
            ↓
GetCursorPos
            ↓
ScreenToClient
            ↓
Point-in-polygon test
            ↓
WS_EX_TRANSPARENT enabled or disabled
```

Godot remains responsible for:

- Scene management
- Rendering
- Animation
- Physics
- Polygon generation
- Transform updates

The extension is responsible for:

- Native Windows cursor coordinates
- Polygon hit testing
- Native click-through window style

## Performance Notes

- Only call `set_polygon()` when polygon data changes.
- Run `update()` at a fixed 30–60 Hz interval when possible.
- Avoid debug output inside `update()`.
- Use simple polygons when practical.
- Remove unused polygons with `remove_polygon()`.

## Limitations

- Windows only
- Included binaries support x86_64 only
- Polygon hit testing is based on the current cursor position
- The extension does not automatically inspect the Godot scene tree
- The extension does not automatically generate collision shapes
- The extension does not configure transparent rendering
- Coordinate conversion may need adjustment when using custom viewport scaling

## Building from Source

Requirements:

- Python
- SCons
- A C++ compiler supported by `godot-cpp`
- Godot-compatible `godot-cpp` source
- Windows SDK and MSVC Build Tools

Initialize the submodule:

```powershell
git submodule update --init --recursive
```

Build the debug DLL:

```powershell
py -m SCons platform=windows arch=x86_64 target=template_debug
```

Build the release DLL:

```powershell
py -m SCons platform=windows arch=x86_64 target=template_release
```

The compiled DLLs are copied into:

```text
addons/native_click_through/bin/windows/
```

## Troubleshooting

### Godot cannot find `NativeClickThrough`

Check that this file exists:

```text
addons/native_click_through/native_click_through.gdextension
```

Also verify that the DLL paths inside the `.gdextension` file match the actual filenames.

### The DLL cannot be loaded

Check that:

- The DLL architecture is x86_64.
- The Godot executable is x86_64.
- The DLL was built against a compatible Godot version.
- Required Visual C++ runtime components are available.
- Both the `.gdextension` file and DLL are inside the project folder.

### The entire window stays click-through

Check that:

- `enable()` has been called.
- At least one valid polygon has been registered.
- The polygon contains at least three points.
- Polygon points are in window client coordinates.
- `update()` is being called repeatedly.

### The entire window stays interactive

Check that:

- `update()` is running.
- The cursor is not accidentally inside a large polygon.
- Old polygons have been removed.
- Polygon coordinates are not incorrectly scaled.

### The clickable region does not follow the object

Call `set_polygon()` again after the object's transform changes.

Convert the original local polygon points with `to_global()` before sending them to the extension.

## License

This addon is released under the MIT License.

See [LICENSE](LICENSE) for details.

## Author

Khang Quach

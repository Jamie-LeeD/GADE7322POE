"""
Creates Shader 1 for Part 3: a custom material that displaces vertices (World Position Offset)
and applies it to the terrain ground tile materials.

Run via UnrealEditor-Cmd -ExecutePythonScript=...
"""

import unreal


def _expr(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def _connect(from_expr, from_pin, to_expr, to_pin):
    unreal.MaterialEditingLibrary.connect_material_expressions(from_expr, from_pin, to_expr, to_pin)


def build_waving_material(asset_path, asset_name, base_color, amplitude=5.0, wave_speed=1.75, frequency=0.025):
    full_path = f"{asset_path}/{asset_name}"

    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        unreal.EditorAssetLibrary.delete_asset(full_path)

    if not unreal.EditorAssetLibrary.does_directory_exist(asset_path):
        unreal.EditorAssetLibrary.make_directory(asset_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = asset_tools.create_asset(asset_name, asset_path, unreal.Material, unreal.MaterialFactoryNew())
    if not mat:
        unreal.log_error(f"Failed to create {full_path}")
        return None

    # --- Base Colour ---
    color_node = _expr(mat, unreal.MaterialExpressionConstant3Vector, -700, -250)
    color_node.set_editor_property(
        "constant",
        unreal.LinearColor(base_color[0], base_color[1], base_color[2], 1.0),
    )
    unreal.MaterialEditingLibrary.connect_material_property(color_node, "", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = _expr(mat, unreal.MaterialExpressionConstant, -700, -100)
    roughness.set_editor_property("r", 0.85)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # --- Vertex displacement (World Position Offset) ---
    # wave = sin(Time * speed + (WorldPos.X + WorldPos.Y) * frequency) * amplitude
    # offset = float3(0, 0, wave)
    time_node = _expr(mat, unreal.MaterialExpressionTime, -1100, 200)

    speed_node = _expr(mat, unreal.MaterialExpressionConstant, -900, 120)
    speed_node.set_editor_property("r", float(wave_speed))

    speed_mul = _expr(mat, unreal.MaterialExpressionMultiply, -700, 160)
    _connect(time_node, "", speed_mul, "A")
    _connect(speed_node, "", speed_mul, "B")

    world_pos = _expr(mat, unreal.MaterialExpressionWorldPosition, -1100, 420)

    mask_x = _expr(mat, unreal.MaterialExpressionComponentMask, -900, 360)
    mask_x.set_editor_property("r", True)
    mask_x.set_editor_property("g", False)
    mask_x.set_editor_property("b", False)
    mask_x.set_editor_property("a", False)
    _connect(world_pos, "", mask_x, "")

    mask_y = _expr(mat, unreal.MaterialExpressionComponentMask, -900, 480)
    mask_y.set_editor_property("r", False)
    mask_y.set_editor_property("g", True)
    mask_y.set_editor_property("b", False)
    mask_y.set_editor_property("a", False)
    _connect(world_pos, "", mask_y, "")

    add_xy = _expr(mat, unreal.MaterialExpressionAdd, -700, 420)
    _connect(mask_x, "", add_xy, "A")
    _connect(mask_y, "", add_xy, "B")

    freq_node = _expr(mat, unreal.MaterialExpressionConstant, -700, 540)
    freq_node.set_editor_property("r", float(frequency))

    freq_mul = _expr(mat, unreal.MaterialExpressionMultiply, -500, 420)
    _connect(add_xy, "", freq_mul, "A")
    _connect(freq_node, "", freq_mul, "B")

    phase = _expr(mat, unreal.MaterialExpressionAdd, -300, 260)
    _connect(speed_mul, "", phase, "A")
    _connect(freq_mul, "", phase, "B")

    sine = _expr(mat, unreal.MaterialExpressionSine, -100, 260)
    _connect(phase, "", sine, "")

    amp_node = _expr(mat, unreal.MaterialExpressionConstant, -100, 380)
    amp_node.set_editor_property("r", float(amplitude))

    wave = _expr(mat, unreal.MaterialExpressionMultiply, 100, 260)
    _connect(sine, "", wave, "A")
    _connect(amp_node, "", wave, "B")

    zero = _expr(mat, unreal.MaterialExpressionConstant, 100, 100)
    zero.set_editor_property("r", 0.0)

    append_xy = _expr(mat, unreal.MaterialExpressionAppendVector, 300, 140)
    _connect(zero, "", append_xy, "A")
    _connect(zero, "", append_xy, "B")

    append_xyz = _expr(mat, unreal.MaterialExpressionAppendVector, 500, 220)
    _connect(append_xy, "", append_xyz, "A")
    _connect(wave, "", append_xyz, "B")

    unreal.MaterialEditingLibrary.connect_material_property(
        append_xyz, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(full_path)
    unreal.log(f"Created waving material: {full_path}")
    return mat


def main():
    shaders_path = "/Game/Shaders"
    terrain_mats = "/Game/Terrain/Materials"

    # Named Part 3 shader asset (vertex displacement)
    build_waving_material(
        shaders_path,
        "M_WavingGrass",
        base_color=(0.18, 0.42, 0.12),
        amplitude=6.0,
        wave_speed=1.8,
        frequency=0.03,
    )

    # Replace ground tile materials in-place so existing terrain BPs pick them up
    build_waving_material(terrain_mats, "M_TileGroundA", (0.20, 0.45, 0.14), amplitude=5.0)
    build_waving_material(terrain_mats, "M_TileGroundB", (0.28, 0.40, 0.12), amplitude=5.5)
    build_waving_material(terrain_mats, "M_TileGroundC", (0.16, 0.36, 0.18), amplitude=4.5)

    unreal.log("Shader 1 (vertex displacement) ready: /Game/Shaders/M_WavingGrass + ground tiles")


if __name__ == "__main__":
    main()

"""
Rebuild Shader 1 from a duplicate of M_Stylized_Grass (keeps exact look),
then add WPO wave. Write full Materials (not MIs) back to M_TileGroundA/B/C
so BP_TerrainTile SetMaterial refs keep working with visible motion.
"""
import unreal

STYLIZED = "/Game/Stylized_Grass/Material/M_Stylized_Grass"
SHADER = "/Game/Shaders/M_WavingGrass"
TILES = (
    ("/Game/Terrain/Materials", "M_TileGroundA", 12.0),
    ("/Game/Terrain/Materials", "M_TileGroundB", 14.0),
    ("/Game/Terrain/Materials", "M_TileGroundC", 10.0),
)


def E(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def C(a, ap, b, bp):
    unreal.MaterialEditingLibrary.connect_material_expressions(a, ap, b, bp)


def P(expr, pin, prop):
    unreal.MaterialEditingLibrary.connect_material_property(expr, pin, prop)


def add_shader1_wpo(mat, amplitude, speed=2.5, frequency=0.06):
    """Append Shader 1 vertex displacement; does not clear existing stylized graph."""
    time_n = E(mat, unreal.MaterialExpressionTime, -1600, 900)
    speed_n = E(mat, unreal.MaterialExpressionConstant, -1400, 840)
    speed_n.set_editor_property("r", float(speed))
    speed_mul = E(mat, unreal.MaterialExpressionMultiply, -1200, 880)
    C(time_n, "", speed_mul, "A")
    C(speed_n, "", speed_mul, "B")

    wp = E(mat, unreal.MaterialExpressionWorldPosition, -1600, 1100)
    mx = E(mat, unreal.MaterialExpressionComponentMask, -1400, 1020)
    mx.set_editor_property("r", True)
    mx.set_editor_property("g", False)
    mx.set_editor_property("b", False)
    mx.set_editor_property("a", False)
    C(wp, "", mx, "")
    my = E(mat, unreal.MaterialExpressionComponentMask, -1400, 1140)
    my.set_editor_property("r", False)
    my.set_editor_property("g", True)
    my.set_editor_property("b", False)
    my.set_editor_property("a", False)
    C(wp, "", my, "")
    add_xy = E(mat, unreal.MaterialExpressionAdd, -1200, 1080)
    C(mx, "", add_xy, "A")
    C(my, "", add_xy, "B")

    freq_n = E(mat, unreal.MaterialExpressionConstant, -1200, 1220)
    freq_n.set_editor_property("r", float(frequency))
    freq_mul = E(mat, unreal.MaterialExpressionMultiply, -1000, 1080)
    C(add_xy, "", freq_mul, "A")
    C(freq_n, "", freq_mul, "B")

    phase = E(mat, unreal.MaterialExpressionAdd, -800, 920)
    C(speed_mul, "", phase, "A")
    C(freq_mul, "", phase, "B")
    sine = E(mat, unreal.MaterialExpressionSine, -600, 920)
    C(phase, "", sine, "")

    amp_n = E(mat, unreal.MaterialExpressionConstant, -600, 1040)
    amp_n.set_editor_property("r", float(amplitude))
    wave = E(mat, unreal.MaterialExpressionMultiply, -400, 920)
    C(sine, "", wave, "A")
    C(amp_n, "", wave, "B")

    # Slight XY sway for low-vert cubes
    sway_s = E(mat, unreal.MaterialExpressionConstant, -400, 800)
    sway_s.set_editor_property("r", 0.35)
    sway = E(mat, unreal.MaterialExpressionMultiply, -200, 800)
    C(wave, "", sway, "A")
    C(sway_s, "", sway, "B")
    append_xy = E(mat, unreal.MaterialExpressionAppendVector, 0, 840)
    C(sway, "", append_xy, "A")
    C(sway, "", append_xy, "B")
    append_xyz = E(mat, unreal.MaterialExpressionAppendVector, 220, 920)
    C(append_xy, "", append_xyz, "A")
    C(wave, "", append_xyz, "B")
    P(append_xyz, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)


def duplicate_stylized_to(dest_path, amplitude):
    if unreal.EditorAssetLibrary.does_asset_exist(dest_path):
        unreal.EditorAssetLibrary.delete_asset(dest_path)
    folder, name = dest_path.rsplit("/", 1)
    if not unreal.EditorAssetLibrary.does_directory_exist(folder):
        unreal.EditorAssetLibrary.make_directory(folder)

    dup = unreal.EditorAssetLibrary.duplicate_asset(STYLIZED, dest_path)
    if not dup:
        # Fallback: duplicate_asset sometimes wants path without asset name twice
        unreal.log_warning(f"duplicate_asset failed for {dest_path}, trying AssetTools")
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        src = unreal.EditorAssetLibrary.load_asset(STYLIZED)
        dup = asset_tools.duplicate_asset(name, folder, src)
    if not dup:
        unreal.log_error(f"Could not duplicate stylized grass to {dest_path}")
        return None

    mat = unreal.EditorAssetLibrary.load_asset(dest_path)
    add_shader1_wpo(mat, amplitude)
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(dest_path)
    unreal.log(f"Created {dest_path} = M_Stylized_Grass + Shader1 WPO (amp={amplitude})")
    return mat


def force_tile_mesh_wpo():
    """Enable Evaluate World Position Offset on BP_TerrainTile mesh via SCS editor API."""
    bp = unreal.EditorAssetLibrary.load_asset("/Game/Terrain/BP_TerrainTile")
    if not bp:
        return
    try:
        # UE5: SubobjectDataSubsystem
        sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
        handles = sds.k2_gather_subobject_data_for_blueprint(bp)
        for h in handles:
            data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
            obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
            if not obj or not isinstance(obj, unreal.StaticMeshComponent):
                # template objects may be the component archetype
                try:
                    if obj and "StaticMesh" in obj.get_class().get_name():
                        pass
                    else:
                        continue
                except Exception:
                    continue
            for prop, val in (
                ("evaluate_world_position_offset", True),
                ("world_position_offset_disable_distance", 0.0),
                ("evaluate_world_position_offset_in_ray_tracing", True),
            ):
                try:
                    obj.set_editor_property(prop, val)
                    unreal.log(f"Set {prop}={val} on {obj.get_name()}")
                except Exception as ex:
                    unreal.log_warning(f"{prop}: {ex}")
        unreal.EditorAssetLibrary.save_asset("/Game/Terrain/BP_TerrainTile")
    except Exception as ex:
        unreal.log_warning(f"BP WPO tweak failed: {ex}")


def bump_stylized_tiling_visible(mat):
    """If stylized has a tiling scalar, bump it so grass detail reads on cubes."""
    for e in unreal.MaterialEditingLibrary.get_material_expressions(mat):
        if e.get_class().get_name() != "MaterialExpressionScalarParameter":
            continue
        try:
            name = str(e.get_editor_property("parameter_name"))
            val = float(e.get_editor_property("default_value"))
            unreal.log(f"Stylized scalar '{name}' default={val}")
            # Common names: Tiling, Tile, UVScale, TextureTiling
            if name.lower() in ("tiling", "tile", "uvscale", "texturescale", "scale", "uv tiling"):
                e.set_editor_property("default_value", max(val, 1.0) if val > 0 else 1.0)
        except Exception:
            pass


def main():
    if not unreal.EditorAssetLibrary.does_asset_exist(STYLIZED):
        unreal.log_error(f"Missing {STYLIZED}")
        return

    # Named Part 3 shader asset
    duplicate_stylized_to(SHADER, amplitude=12.0)

    # Ground tiles as FULL materials (not MIs) — same stylized+wave graph
    for folder, name, amp in TILES:
        # Must delete MIC or Material at this path first
        full = f"{folder}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(full):
            unreal.EditorAssetLibrary.delete_asset(full)
        mat = duplicate_stylized_to(full, amplitude=amp)
        if mat:
            bump_stylized_tiling_visible(mat)
            unreal.MaterialEditingLibrary.recompile_material(mat)
            unreal.EditorAssetLibrary.save_asset(full)

    force_tile_mesh_wpo()
    unreal.log("DONE: tiles are real Materials = M_Stylized_Grass look + Shader 1 wave")


if __name__ == "__main__":
    main()

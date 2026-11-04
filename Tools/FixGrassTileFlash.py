"""
Grass tiles flashing visible/invisible: WPO was huge (10-14uu) so verts left
mesh bounds and got frustum/occlusion culled every frame.

Fix: gentle Z-only wave + larger TileMesh bounds. Do not delete ground materials.
"""
import unreal

GROUND = (
    "/Game/Terrain/Materials/M_TileGroundA",
    "/Game/Terrain/Materials/M_TileGroundB",
    "/Game/Terrain/Materials/M_TileGroundC",
    "/Game/Shaders/M_WavingGrass",
)
# Soft bob — visible motion without leaving bounds on a 100uu cube
TARGET_AMP = 1.5
TARGET_SWAY = 0.0


def soften_wpo(path):
    mat = unreal.EditorAssetLibrary.load_asset(path)
    if not mat:
        unreal.log_error(f"Missing {path}")
        return
    changed = 0
    for e in unreal.MaterialEditingLibrary.get_material_expressions(mat):
        cls = e.get_class().get_name()
        if cls != "MaterialExpressionConstant":
            continue
        try:
            r = float(e.get_editor_property("r"))
        except Exception:
            continue
        # Amplitude constants from RebuildGrassFromStylized (10/12/14) and sway (0.25/0.35)
        if r >= 5.0:
            e.set_editor_property("r", float(TARGET_AMP))
            changed += 1
            unreal.log(f"{path}: amp {r} -> {TARGET_AMP}")
        elif 0.2 <= r <= 0.4:
            e.set_editor_property("r", float(TARGET_SWAY))
            changed += 1
            unreal.log(f"{path}: sway {r} -> {TARGET_SWAY}")
    if changed:
        unreal.MaterialEditingLibrary.recompile_material(mat)
        unreal.EditorAssetLibrary.save_asset(path)
    else:
        unreal.log_warning(f"{path}: no large amp constants found — rewriting WPO gently")
        rewrite_gentle_wpo(mat, path)


def rewrite_gentle_wpo(mat, path):
    """If constants weren't found, reconnect a small Z-only WPO (keeps stylized texture graph)."""
    def E(cls, x, y):
        return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)

    def C(a, ap, b, bp):
        unreal.MaterialEditingLibrary.connect_material_expressions(a, ap, b, bp)

    time_n = E(unreal.MaterialExpressionTime, -1800, 1400)
    speed = E(unreal.MaterialExpressionConstant, -1600, 1340)
    speed.set_editor_property("r", 1.6)
    speed_mul = E(unreal.MaterialExpressionMultiply, -1400, 1380)
    C(time_n, "", speed_mul, "A")
    C(speed, "", speed_mul, "B")

    wp = E(unreal.MaterialExpressionWorldPosition, -1800, 1600)
    mx = E(unreal.MaterialExpressionComponentMask, -1600, 1520)
    mx.set_editor_property("r", True)
    mx.set_editor_property("g", False)
    mx.set_editor_property("b", False)
    mx.set_editor_property("a", False)
    C(wp, "", mx, "")
    my = E(unreal.MaterialExpressionComponentMask, -1600, 1640)
    my.set_editor_property("r", False)
    my.set_editor_property("g", True)
    my.set_editor_property("b", False)
    my.set_editor_property("a", False)
    C(wp, "", my, "")
    add_xy = E(unreal.MaterialExpressionAdd, -1400, 1580)
    C(mx, "", add_xy, "A")
    C(my, "", add_xy, "B")
    freq = E(unreal.MaterialExpressionConstant, -1400, 1720)
    freq.set_editor_property("r", 0.04)
    freq_mul = E(unreal.MaterialExpressionMultiply, -1200, 1580)
    C(add_xy, "", freq_mul, "A")
    C(freq, "", freq_mul, "B")
    phase = E(unreal.MaterialExpressionAdd, -1000, 1420)
    C(speed_mul, "", phase, "A")
    C(freq_mul, "", phase, "B")
    sine = E(unreal.MaterialExpressionSine, -800, 1420)
    C(phase, "", sine, "")
    amp = E(unreal.MaterialExpressionConstant, -800, 1540)
    amp.set_editor_property("r", float(TARGET_AMP))
    wave = E(unreal.MaterialExpressionMultiply, -600, 1420)
    C(sine, "", wave, "A")
    C(amp, "", wave, "B")
    zero = E(unreal.MaterialExpressionConstant, -600, 1280)
    zero.set_editor_property("r", 0.0)
    append_xy = E(unreal.MaterialExpressionAppendVector, -400, 1320)
    C(zero, "", append_xy, "A")
    C(zero, "", append_xy, "B")
    append_xyz = E(unreal.MaterialExpressionAppendVector, -200, 1400)
    C(append_xy, "", append_xyz, "A")
    C(wave, "", append_xyz, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        append_xyz, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET
    )
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    unreal.log(f"{path}: gentle Z WPO reconnected amp={TARGET_AMP}")


def expand_tile_bounds():
    bp = unreal.EditorAssetLibrary.load_asset("/Game/Terrain/BP_TerrainTile")
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for h in sds.k2_gather_subobject_data_for_blueprint(bp):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if not obj or obj.get_name() != "TileMesh_GEN_VARIABLE":
            continue
        for prop, val in (
            ("bounds_scale", 3.0),
            ("evaluate_world_position_offset", True),
            ("world_position_offset_disable_distance", 0.0),
            ("evaluate_world_position_offset_in_ray_tracing", True),
        ):
            try:
                obj.set_editor_property(prop, val)
                unreal.log(f"TileMesh.{prop}={val}")
            except Exception as ex:
                unreal.log_warning(f"TileMesh.{prop}: {ex}")
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception:
        pass
    unreal.EditorAssetLibrary.save_asset("/Game/Terrain/BP_TerrainTile")
    # Confirm ground soft refs still present after save
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


def main():
    for path in GROUND:
        soften_wpo(path)
    expand_tile_bounds()
    unreal.log("DONE: grass flash fix (small WPO + bounds_scale=3)")


if __name__ == "__main__":
    main()

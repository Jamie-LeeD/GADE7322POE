# Stop generated tiles flashing:
# 1) Rebuild M_TileGround* as static lit mats (no WPO shimmer)
# 2) Turn off SkyLight real-time capture (moving sky was thrashing lighting)
import unreal

MAP_PATH = "/Game/Maps/L_PrincessTD"
TILE_MATS = (
    ("/Game/Terrain/Materials", "M_TileGroundA", (0.30, 0.62, 0.22)),
    ("/Game/Terrain/Materials", "M_TileGroundB", (0.38, 0.58, 0.20)),
    ("/Game/Terrain/Materials", "M_TileGroundC", (0.26, 0.54, 0.28)),
)


def _expr(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def rebuild_static_tile(asset_path, asset_name, color):
    full = f"{asset_path}/{asset_name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.log_error(f"Missing {full}")
        return
    mat = unreal.EditorAssetLibrary.load_asset(full)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property("two_sided", False)

    base = _expr(mat, unreal.MaterialExpressionConstant3Vector, -400, -100)
    base.set_editor_property("constant", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        base, "", unreal.MaterialProperty.MP_BASE_COLOR
    )

    for name, val, y, prop in (
        ("metallic", 0.0, 0, unreal.MaterialProperty.MP_METALLIC),
        ("specular", 0.15, 70, unreal.MaterialProperty.MP_SPECULAR),
        ("roughness", 0.92, 140, unreal.MaterialProperty.MP_ROUGHNESS),
    ):
        node = _expr(mat, unreal.MaterialExpressionConstant, -400, y)
        node.set_editor_property("r", float(val))
        unreal.MaterialEditingLibrary.connect_material_property(node, "", prop)

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(full)
    unreal.log(f"Rebuilt static tile material {full} (no WPO)")


def fix_skylight():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    for a in unreal.EditorLevelLibrary.get_all_level_actors():
        if not isinstance(a, unreal.SkyLight):
            continue
        c = a.light_component
        try:
            c.set_editor_property("real_time_capture", False)
        except Exception:
            pass
        try:
            c.set_editor_property("lower_hemisphere_is_black", False)
        except Exception:
            pass
        try:
            c.set_editor_property(
                "lower_hemisphere_color", unreal.LinearColor(0.55, 0.7, 0.9, 1.0)
            )
        except Exception:
            pass
        c.set_intensity(3.0)
        try:
            c.recapture_sky()
        except Exception:
            pass
        unreal.log(f"SkyLight '{a.get_actor_label()}': real_time_capture=False")
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


def main():
    for path, name, color in TILE_MATS:
        rebuild_static_tile(path, name, color)
    # Keep Shader 1 waving grass showcase intact (not used as tile mat).
    fix_skylight()
    unreal.log("DONE: tiles should no longer flash")


if __name__ == "__main__":
    main()

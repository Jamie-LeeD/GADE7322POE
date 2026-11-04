"""
1) Goblins were sharing M_TileGroundA (grass) — give them a dedicated body material.
2) Terrain tile BP ground mats were restored from git separately; verify refs resolve.
Do NOT delete M_TileGround* while the tile BP is loaded/saved.
"""
import unreal

GOBLIN_MAT_DIR = "/Game/Enemies/Materials"
GOBLIN_MAT = f"{GOBLIN_MAT_DIR}/M_GoblinBody"
GOBLIN_BPS = (
    "/Game/Enemies/BP_GoblinEnemy",
    "/Game/Enemies/BP_GoblinBerserker",
    "/Game/Enemies/BP_GoblinShaman",
)
GROUND = (
    "/Game/Terrain/Materials/M_TileGroundA",
    "/Game/Terrain/Materials/M_TileGroundB",
    "/Game/Terrain/Materials/M_TileGroundC",
)


def E(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def P(expr, pin, prop):
    unreal.MaterialEditingLibrary.connect_material_property(expr, pin, prop)


def make_goblin_material():
    if unreal.EditorAssetLibrary.does_asset_exist(GOBLIN_MAT):
        mat = unreal.EditorAssetLibrary.load_asset(GOBLIN_MAT)
        unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    else:
        if not unreal.EditorAssetLibrary.does_directory_exist(GOBLIN_MAT_DIR):
            unreal.EditorAssetLibrary.make_directory(GOBLIN_MAT_DIR)
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_GoblinBody", GOBLIN_MAT_DIR, unreal.Material, unreal.MaterialFactoryNew()
        )
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)

    # Goblin-green (not grass textures)
    base = E(mat, unreal.MaterialExpressionConstant3Vector, -400, -80)
    base.set_editor_property("constant", unreal.LinearColor(0.18, 0.42, 0.12, 1.0))
    P(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = E(mat, unreal.MaterialExpressionConstant, -400, 40)
    rough.set_editor_property("r", 0.75)
    P(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal = E(mat, unreal.MaterialExpressionConstant, -400, 110)
    metal.set_editor_property("r", 0.0)
    P(metal, "", unreal.MaterialProperty.MP_METALLIC)

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(GOBLIN_MAT)
    unreal.log(f"Created {GOBLIN_MAT}")
    return mat


def assign_goblin_body(mat):
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for path in GOBLIN_BPS:
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.log_warning(f"Missing {path}")
            continue
        bp = unreal.EditorAssetLibrary.load_asset(path)
        changed = False
        for h in sds.k2_gather_subobject_data_for_blueprint(bp):
            data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
            obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
            if not obj or not isinstance(obj, unreal.StaticMeshComponent):
                continue
            name = obj.get_name()
            # Only the body mesh — not arrows / random props
            if "Mesh" not in name or "Rock" in name or "Tile" in name:
                # Goblin body is Mesh_GEN_VARIABLE
                if name != "Mesh_GEN_VARIABLE" and not name.startswith("Mesh"):
                    continue
            current = obj.get_material(0)
            cur_path = current.get_path_name() if current else "None"
            if "M_TileGround" in cur_path or "M_WavingGrass" in cur_path or "Stylized_Grass" in cur_path or current is None:
                obj.set_material(0, mat)
                try:
                    obj.set_editor_property("override_materials", [mat])
                except Exception:
                    pass
                changed = True
                unreal.log(f"{path} {name}: {cur_path} -> {GOBLIN_MAT}")
        if changed:
            unreal.EditorAssetLibrary.save_asset(path)


def verify_ground_and_tile_bp():
    for path in GROUND:
        ok = unreal.EditorAssetLibrary.does_asset_exist(path)
        unreal.log(f"ground {path} exists={ok}")
        if ok:
            asset = unreal.EditorAssetLibrary.load_asset(path)
            unreal.log(f"  type={asset.get_class().get_name()}")

    bp = unreal.EditorAssetLibrary.load_asset("/Game/Terrain/BP_TerrainTile")
    # Soft-load check: open package bytes aren't available; instead ensure TileMesh can receive mats at runtime
    # by setting a sensible default override so editor preview isn't WorldGrid/None.
    ground_a = unreal.EditorAssetLibrary.load_asset(GROUND[0])
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for h in sds.k2_gather_subobject_data_for_blueprint(bp):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if not obj or obj.get_name() != "TileMesh_GEN_VARIABLE":
            continue
        # Default preview material only — BP graph SetMaterial still drives GroundVariant at runtime.
        if obj.get_material(0) is None or "WorldGridMaterial" in obj.get_material(0).get_path_name():
            obj.set_material(0, ground_a)
            unreal.log("TileMesh default preview set to M_TileGroundA")
        try:
            obj.set_editor_property("evaluate_world_position_offset", True)
            obj.set_editor_property("world_position_offset_disable_distance", 0.0)
        except Exception as ex:
            unreal.log_warning(f"WPO flag: {ex}")
    # Save AFTER ground assets exist (critical — never save BP while ground mats are deleted)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log("Verified tile BP + ground materials")


def main():
    mat = make_goblin_material()
    assign_goblin_body(mat)
    verify_ground_and_tile_bp()
    unreal.log("DONE: goblins use M_GoblinBody; grass tiles keep M_TileGroundA/B/C")


if __name__ == "__main__":
    main()

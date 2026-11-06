"""Bright unlit emissive material for projectile trail/impact sparks."""
import unreal

VFX_DIR = "/Game/VFX"
PATH = f"{VFX_DIR}/M_ProjectileSpark"


def E(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def main():
    if not unreal.EditorAssetLibrary.does_directory_exist(VFX_DIR):
        unreal.EditorAssetLibrary.make_directory(VFX_DIR)

    if unreal.EditorAssetLibrary.does_asset_exist(PATH):
        mat = unreal.EditorAssetLibrary.load_asset(PATH)
        unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    else:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_ProjectileSpark", VFX_DIR, unreal.Material, unreal.MaterialFactoryNew()
        )

    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    try:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    except Exception:
        pass
    try:
        mat.set_editor_property("two_sided", True)
    except Exception:
        pass

    color = E(mat, unreal.MaterialExpressionVectorParameter, -300, 0)
    color.set_editor_property("parameter_name", "SparkColor")
    color.set_editor_property("default_value", unreal.LinearColor(2.0, 1.4, 0.2, 1.0))

    boost = E(mat, unreal.MaterialExpressionConstant, -300, 120)
    boost.set_editor_property("r", 3.0)
    mul = E(mat, unreal.MaterialExpressionMultiply, 0, 40)
    unreal.MaterialEditingLibrary.connect_material_expressions(color, "", mul, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(boost, "", mul, "B")

    unreal.MaterialEditingLibrary.connect_material_property(
        mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(PATH)
    unreal.log(f"DONE: {PATH}")


if __name__ == "__main__":
    main()

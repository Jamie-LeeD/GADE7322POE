"""
Shader 2: translucent illuminated pink upgrade aura (Fresnel wrap + emissive).
"""

import unreal


def _expr(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def _connect(a, a_pin, b, b_pin):
    unreal.MaterialEditingLibrary.connect_material_expressions(a, a_pin, b, b_pin)


def main():
    path = "/Game/Shaders"
    name = "M_UpgradeAura"
    full = f"{path}/{name}"

    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)

    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, path, unreal.Material, unreal.MaterialFactoryNew()
    )
    if not mat:
        unreal.log_error("Failed to create M_UpgradeAura")
        return

    # Translucent unlit so it reads as a glowing wrap, not a solid mesh.
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    try:
        mat.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_PER_VERTEX_NON_DIRECTIONAL)
    except Exception:
        pass

    pink = _expr(mat, unreal.MaterialExpressionConstant3Vector, -800, -100)
    pink.set_editor_property("constant", unreal.LinearColor(1.0, 0.25, 0.75, 1.0))

    # Base colour = illuminated pink (brief: change base colour)
    unreal.MaterialEditingLibrary.connect_material_property(pink, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Fresnel: stronger on the silhouette so it "wraps" the defender
    fresnel = _expr(mat, unreal.MaterialExpressionFresnel, -800, 200)
    try:
        fresnel.set_editor_property("exponent_in", 3.0)
        fresnel.set_editor_property("base_reflect_fraction_in", 0.04)
    except Exception:
        pass

    opacity_scale = _expr(mat, unreal.MaterialExpressionConstant, -600, 320)
    opacity_scale.set_editor_property("r", 0.55)

    opacity = _expr(mat, unreal.MaterialExpressionMultiply, -400, 240)
    _connect(fresnel, "", opacity, "A")
    _connect(opacity_scale, "", opacity, "B")
    unreal.MaterialEditingLibrary.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    # Emissive so it glows / feels illuminated
    emissive_strength = _expr(mat, unreal.MaterialExpressionConstant, -600, 0)
    emissive_strength.set_editor_property("r", 4.0)

    emissive = _expr(mat, unreal.MaterialExpressionMultiply, -400, -50)
    _connect(pink, "", emissive, "A")
    _connect(emissive_strength, "", emissive, "B")
    unreal.MaterialEditingLibrary.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(full)
    unreal.log(f"Created {full}")


if __name__ == "__main__":
    main()

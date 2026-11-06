"""Add AuraColor vector parameter to M_UpgradeAura so C++ can tint blue/pink by upgrade level."""
import unreal

FULL = "/Game/Shaders/M_UpgradeAura"


def E(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def C(a, ap, b, bp):
    unreal.MaterialEditingLibrary.connect_material_expressions(a, ap, b, bp)


def main():
    if not unreal.EditorAssetLibrary.does_asset_exist(FULL):
        unreal.log_error(f"Missing {FULL}")
        return
    mat = unreal.EditorAssetLibrary.load_asset(FULL)
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    try:
        mat.set_editor_property(
            "translucency_lighting_mode",
            unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_PER_VERTEX_NON_DIRECTIONAL,
        )
    except Exception:
        pass

    color = E(mat, unreal.MaterialExpressionVectorParameter, -800, -100)
    color.set_editor_property("parameter_name", "AuraColor")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.25, 0.75, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )

    fresnel = E(mat, unreal.MaterialExpressionFresnel, -800, 200)
    try:
        fresnel.set_editor_property("exponent_in", 3.0)
        fresnel.set_editor_property("base_reflect_fraction_in", 0.04)
    except Exception:
        pass
    opacity_scale = E(mat, unreal.MaterialExpressionConstant, -600, 320)
    opacity_scale.set_editor_property("r", 0.55)
    opacity = E(mat, unreal.MaterialExpressionMultiply, -400, 240)
    C(fresnel, "", opacity, "A")
    C(opacity_scale, "", opacity, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        opacity, "", unreal.MaterialProperty.MP_OPACITY
    )

    emissive_strength = E(mat, unreal.MaterialExpressionConstant, -600, 0)
    emissive_strength.set_editor_property("r", 4.0)
    emissive = E(mat, unreal.MaterialExpressionMultiply, -400, -50)
    C(color, "", emissive, "A")
    C(emissive_strength, "", emissive, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(FULL)
    unreal.log("M_UpgradeAura now has AuraColor parameter (default pink)")


if __name__ == "__main__":
    main()

"""
Shader 1 (M_WavingGrass) = M_Stylized_Grass look (textures) + vertex wave (WPO).
Grass tiles M_TileGroundA/B/C become instances of that material.
"""
import unreal

SHADER_PATH = "/Game/Shaders"
SHADER_NAME = "M_WavingGrass"
SHADER_FULL = f"{SHADER_PATH}/{SHADER_NAME}"

TEX = {
    "BaseColor": "/Game/Stylized_Grass/Textures/T_Grass_BC",
    "Normal": "/Game/Stylized_Grass/Textures/T_Grass_N",
    "Roughness": "/Game/Stylized_Grass/Textures/T_Grass_R",
    "AO": "/Game/Stylized_Grass/Textures/T_Grass_AO",
}

TILE_MIS = (
    ("/Game/Terrain/Materials", "M_TileGroundA", 1.00),
    ("/Game/Terrain/Materials", "M_TileGroundB", 0.92),
    ("/Game/Terrain/Materials", "M_TileGroundC", 1.08),
)


def _expr(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def _connect(a, ap, b, bp):
    unreal.MaterialEditingLibrary.connect_material_expressions(a, ap, b, bp)


def _load_tex(path):
    return unreal.EditorAssetLibrary.load_asset(path)


def build_shader1():
    if unreal.EditorAssetLibrary.does_asset_exist(SHADER_FULL):
        mat = unreal.EditorAssetLibrary.load_asset(SHADER_FULL)
        unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    else:
        if not unreal.EditorAssetLibrary.does_directory_exist(SHADER_PATH):
            unreal.EditorAssetLibrary.make_directory(SHADER_PATH)
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            SHADER_NAME, SHADER_PATH, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not mat:
        unreal.log_error("Could not create/load M_WavingGrass")
        return None

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property("two_sided", False)

    # --- UVs ---
    uv = _expr(mat, unreal.MaterialExpressionTextureCoordinate, -1400, -200)
    tiling = _expr(mat, unreal.MaterialExpressionScalarParameter, -1400, -80)
    tiling.set_editor_property("parameter_name", "Tiling")
    tiling.set_editor_property("default_value", 4.0)
    tiled_uv = _expr(mat, unreal.MaterialExpressionMultiply, -1200, -160)
    _connect(uv, "", tiled_uv, "A")
    _connect(tiling, "", tiled_uv, "B")

    # --- Stylized grass textures ---
    bc = _expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, -280)
    bc.set_editor_property("parameter_name", "BaseColor")
    bc.set_editor_property("texture", _load_tex(TEX["BaseColor"]))
    _connect(tiled_uv, "", bc, "Coordinates")

    tint = _expr(mat, unreal.MaterialExpressionVectorParameter, -1000, -60)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    tinted = _expr(mat, unreal.MaterialExpressionMultiply, -700, -200)
    _connect(bc, "RGB", tinted, "A")
    _connect(tint, "", tinted, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        tinted, "", unreal.MaterialProperty.MP_BASE_COLOR
    )

    nrm = _expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 80)
    nrm.set_editor_property("parameter_name", "Normal")
    nrm.set_editor_property("texture", _load_tex(TEX["Normal"]))
    try:
        nrm.set_editor_property(
            "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
        )
    except Exception:
        pass
    _connect(tiled_uv, "", nrm, "Coordinates")
    unreal.MaterialEditingLibrary.connect_material_property(
        nrm, "RGB", unreal.MaterialProperty.MP_NORMAL
    )

    rough = _expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 320)
    rough.set_editor_property("parameter_name", "RoughnessMap")
    rough.set_editor_property("texture", _load_tex(TEX["Roughness"]))
    try:
        rough.set_editor_property(
            "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
        )
    except Exception:
        pass
    _connect(tiled_uv, "", rough, "Coordinates")
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "R", unreal.MaterialProperty.MP_ROUGHNESS
    )

    ao = _expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1000, 560)
    ao.set_editor_property("parameter_name", "AmbientOcclusion")
    ao.set_editor_property("texture", _load_tex(TEX["AO"]))
    try:
        ao.set_editor_property(
            "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
        )
    except Exception:
        pass
    _connect(tiled_uv, "", ao, "Coordinates")
    unreal.MaterialEditingLibrary.connect_material_property(
        ao, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION
    )

    metallic = _expr(mat, unreal.MaterialExpressionConstant, -700, 40)
    metallic.set_editor_property("r", 0.0)
    unreal.MaterialEditingLibrary.connect_material_property(
        metallic, "", unreal.MaterialProperty.MP_METALLIC
    )

    # --- Shader 1: vertex displacement (WPO) ---
    # wave = sin(Time * Speed + (WorldPos.X + WorldPos.Y) * Freq) * Amp
    time_node = _expr(mat, unreal.MaterialExpressionTime, -1400, 800)
    speed = _expr(mat, unreal.MaterialExpressionScalarParameter, -1200, 740)
    speed.set_editor_property("parameter_name", "WaveSpeed")
    speed.set_editor_property("default_value", 1.8)
    speed_mul = _expr(mat, unreal.MaterialExpressionMultiply, -1000, 780)
    _connect(time_node, "", speed_mul, "A")
    _connect(speed, "", speed_mul, "B")

    wp = _expr(mat, unreal.MaterialExpressionWorldPosition, -1400, 980)
    mask_x = _expr(mat, unreal.MaterialExpressionComponentMask, -1200, 920)
    mask_x.set_editor_property("r", True)
    mask_x.set_editor_property("g", False)
    mask_x.set_editor_property("b", False)
    mask_x.set_editor_property("a", False)
    _connect(wp, "", mask_x, "")
    mask_y = _expr(mat, unreal.MaterialExpressionComponentMask, -1200, 1040)
    mask_y.set_editor_property("r", False)
    mask_y.set_editor_property("g", True)
    mask_y.set_editor_property("b", False)
    mask_y.set_editor_property("a", False)
    _connect(wp, "", mask_y, "")
    add_xy = _expr(mat, unreal.MaterialExpressionAdd, -1000, 980)
    _connect(mask_x, "", add_xy, "A")
    _connect(mask_y, "", add_xy, "B")

    freq = _expr(mat, unreal.MaterialExpressionScalarParameter, -1000, 1100)
    freq.set_editor_property("parameter_name", "WaveFrequency")
    freq.set_editor_property("default_value", 0.03)
    freq_mul = _expr(mat, unreal.MaterialExpressionMultiply, -800, 980)
    _connect(add_xy, "", freq_mul, "A")
    _connect(freq, "", freq_mul, "B")

    phase = _expr(mat, unreal.MaterialExpressionAdd, -600, 820)
    _connect(speed_mul, "", phase, "A")
    _connect(freq_mul, "", phase, "B")
    sine = _expr(mat, unreal.MaterialExpressionSine, -400, 820)
    _connect(phase, "", sine, "")

    amp = _expr(mat, unreal.MaterialExpressionScalarParameter, -400, 940)
    amp.set_editor_property("parameter_name", "WaveAmplitude")
    amp.set_editor_property("default_value", 4.0)
    wave = _expr(mat, unreal.MaterialExpressionMultiply, -200, 820)
    _connect(sine, "", wave, "A")
    _connect(amp, "", wave, "B")

    zero = _expr(mat, unreal.MaterialExpressionConstant, -200, 680)
    zero.set_editor_property("r", 0.0)
    append_xy = _expr(mat, unreal.MaterialExpressionAppendVector, 0, 720)
    _connect(zero, "", append_xy, "A")
    _connect(zero, "", append_xy, "B")
    append_xyz = _expr(mat, unreal.MaterialExpressionAppendVector, 200, 800)
    _connect(append_xy, "", append_xyz, "A")
    _connect(wave, "", append_xyz, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        append_xyz, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(SHADER_FULL)
    unreal.log(f"Shader 1 ready: {SHADER_FULL} (stylized grass textures + WPO)")
    return mat


def make_tile_mi(parent, folder, name, tint_scale, amplitude):
    full = f"{folder}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        # Replace material asset with a MIC of the same name so BP refs keep working.
        unreal.EditorAssetLibrary.delete_asset(full)

    if not unreal.EditorAssetLibrary.does_directory_exist(folder):
        unreal.EditorAssetLibrary.make_directory(folder)

    factory = unreal.MaterialInstanceConstantFactoryNew()
    try:
        factory.set_editor_property("initial_parent", parent)
    except Exception:
        pass
    mi = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.MaterialInstanceConstant, factory
    )
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, parent)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        mi, "WaveAmplitude", float(amplitude)
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        mi, "Tiling", 4.0
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        mi,
        "Tint",
        unreal.LinearColor(tint_scale, tint_scale, tint_scale * 0.95, 1.0),
    )
    unreal.EditorAssetLibrary.save_asset(full)
    unreal.log(f"Tile MI {full} -> parent M_WavingGrass (amp={amplitude})")
    return mi


def main():
    parent = build_shader1()
    if not parent:
        return
    amps = (3.5, 4.0, 3.0)
    for (folder, name, tint), amp in zip(TILE_MIS, amps):
        make_tile_mi(parent, folder, name, tint, amp)
    unreal.log(
        "DONE: grass tiles use stylized grass look + Shader 1 wave "
        "(M_TileGroundA/B/C are MIs of M_WavingGrass)"
    )


if __name__ == "__main__":
    main()

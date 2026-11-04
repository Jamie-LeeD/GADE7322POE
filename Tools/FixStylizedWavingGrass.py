"""
Fix grass: solid green + no motion.
- Rebuild M_WavingGrass with valid float2 UV tiling, stylized textures, strong WPO
- Force tile MIs to that parent and enable Evaluate World Position Offset on tile meshes
"""
import unreal

SHADER = "/Game/Shaders/M_WavingGrass"
TEX_BC = "/Game/Stylized_Grass/Textures/T_Grass_BC"
TEX_N = "/Game/Stylized_Grass/Textures/T_Grass_N"
TEX_R = "/Game/Stylized_Grass/Textures/T_Grass_R"
TEX_AO = "/Game/Stylized_Grass/Textures/T_Grass_AO"
TILES = (
    ("/Game/Terrain/Materials/M_TileGroundA", 8.0, (1.0, 1.0, 0.95)),
    ("/Game/Terrain/Materials/M_TileGroundB", 9.0, (0.95, 1.0, 0.9)),
    ("/Game/Terrain/Materials/M_TileGroundC", 7.0, (1.0, 0.95, 0.9)),
)


def E(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def C(a, ap, b, bp):
    unreal.MaterialEditingLibrary.connect_material_expressions(a, ap, b, bp)


def P(expr, pin, prop):
    unreal.MaterialEditingLibrary.connect_material_property(expr, pin, prop)


def rebuild_parent():
    for path in (TEX_BC, TEX_N, TEX_R, TEX_AO):
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.log_error(f"Missing texture {path}")
            return None

    if unreal.EditorAssetLibrary.does_asset_exist(SHADER):
        # Delete and recreate so no stale Constant3Vector / broken pins remain
        unreal.EditorAssetLibrary.delete_asset(SHADER)

    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_WavingGrass", "/Game/Shaders", unreal.Material, unreal.MaterialFactoryNew()
    )
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property("two_sided", False)
    # Ensure WPO is allowed on default mesh usage
    try:
        mat.set_editor_property("usage_flag_force_fully_rough", False)
    except Exception:
        pass

    # UV: float2 * float2 (scalar*float2 via Python often fails silently → solid color)
    uv = E(mat, unreal.MaterialExpressionTextureCoordinate, -1600, -200)
    tiling = E(mat, unreal.MaterialExpressionScalarParameter, -1600, -40)
    tiling.set_editor_property("parameter_name", "Tiling")
    tiling.set_editor_property("default_value", 2.0)
    tiling2 = E(mat, unreal.MaterialExpressionAppendVector, -1400, -80)
    C(tiling, "", tiling2, "A")
    C(tiling, "", tiling2, "B")
    tiled_uv = E(mat, unreal.MaterialExpressionMultiply, -1200, -160)
    C(uv, "", tiled_uv, "A")
    C(tiling2, "", tiled_uv, "B")

    def tex_param(name, path, y, sampler=None):
        node = E(mat, unreal.MaterialExpressionTextureSampleParameter2D, -950, y)
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("texture", unreal.EditorAssetLibrary.load_asset(path))
        if sampler is not None:
            try:
                node.set_editor_property("sampler_type", sampler)
            except Exception:
                pass
        C(tiled_uv, "", node, "Coordinates")
        return node

    bc = tex_param(
        "BaseColor", TEX_BC, -300, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
    )
    tint = E(mat, unreal.MaterialExpressionVectorParameter, -950, -40)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    tinted = E(mat, unreal.MaterialExpressionMultiply, -650, -200)
    C(bc, "RGB", tinted, "A")
    C(tint, "", tinted, "B")
    P(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)

    nrm = tex_param(
        "Normal", TEX_N, 40, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
    )
    P(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)

    rough = tex_param(
        "RoughnessMap", TEX_R, 320, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
    )
    P(rough, "R", unreal.MaterialProperty.MP_ROUGHNESS)

    ao = tex_param(
        "AmbientOcclusion", TEX_AO, 560, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
    )
    P(ao, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    metal = E(mat, unreal.MaterialExpressionConstant, -650, 40)
    metal.set_editor_property("r", 0.0)
    P(metal, "", unreal.MaterialProperty.MP_METALLIC)

    # Shader 1 WPO — stronger Z bob + slight XY sway (visible on low-vert cubes)
    time_n = E(mat, unreal.MaterialExpressionTime, -1600, 850)
    speed = E(mat, unreal.MaterialExpressionScalarParameter, -1400, 780)
    speed.set_editor_property("parameter_name", "WaveSpeed")
    speed.set_editor_property("default_value", 2.2)
    speed_mul = E(mat, unreal.MaterialExpressionMultiply, -1200, 820)
    C(time_n, "", speed_mul, "A")
    C(speed, "", speed_mul, "B")

    wp = E(mat, unreal.MaterialExpressionWorldPosition, -1600, 1050)
    mx = E(mat, unreal.MaterialExpressionComponentMask, -1400, 980)
    mx.set_editor_property("r", True)
    mx.set_editor_property("g", False)
    mx.set_editor_property("b", False)
    mx.set_editor_property("a", False)
    C(wp, "", mx, "")
    my = E(mat, unreal.MaterialExpressionComponentMask, -1400, 1100)
    my.set_editor_property("r", False)
    my.set_editor_property("g", True)
    my.set_editor_property("b", False)
    my.set_editor_property("a", False)
    C(wp, "", my, "")
    add_xy = E(mat, unreal.MaterialExpressionAdd, -1200, 1040)
    C(mx, "", add_xy, "A")
    C(my, "", add_xy, "B")

    freq = E(mat, unreal.MaterialExpressionScalarParameter, -1200, 1180)
    freq.set_editor_property("parameter_name", "WaveFrequency")
    freq.set_editor_property("default_value", 0.05)
    freq_mul = E(mat, unreal.MaterialExpressionMultiply, -1000, 1040)
    C(add_xy, "", freq_mul, "A")
    C(freq, "", freq_mul, "B")

    phase = E(mat, unreal.MaterialExpressionAdd, -800, 880)
    C(speed_mul, "", phase, "A")
    C(freq_mul, "", phase, "B")
    sine = E(mat, unreal.MaterialExpressionSine, -600, 880)
    C(phase, "", sine, "")

    amp = E(mat, unreal.MaterialExpressionScalarParameter, -600, 1000)
    amp.set_editor_property("parameter_name", "WaveAmplitude")
    amp.set_editor_property("default_value", 10.0)
    wave = E(mat, unreal.MaterialExpressionMultiply, -400, 880)
    C(sine, "", wave, "A")
    C(amp, "", wave, "B")

    # XY sway = wave * 0.25 so tiles clearly "breathe"
    sway_scale = E(mat, unreal.MaterialExpressionConstant, -400, 760)
    sway_scale.set_editor_property("r", 0.25)
    sway = E(mat, unreal.MaterialExpressionMultiply, -200, 760)
    C(wave, "", sway, "A")
    C(sway_scale, "", sway, "B")

    append_xy = E(mat, unreal.MaterialExpressionAppendVector, 0, 800)
    C(sway, "", append_xy, "A")
    C(sway, "", append_xy, "B")
    append_xyz = E(mat, unreal.MaterialExpressionAppendVector, 200, 880)
    C(append_xy, "", append_xyz, "A")
    C(wave, "", append_xyz, "B")
    P(append_xyz, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(SHADER)
    unreal.log(f"Rebuilt {SHADER}")
    return mat


def rebuild_tile_mis(parent):
    for full, amp, tint in TILES:
        folder = full.rsplit("/", 1)[0]
        name = full.rsplit("/", 1)[1]
        if unreal.EditorAssetLibrary.does_asset_exist(full):
            unreal.EditorAssetLibrary.delete_asset(full)
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
            mi, "WaveAmplitude", float(amp)
        )
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            mi, "WaveSpeed", 2.2
        )
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            mi, "WaveFrequency", 0.05
        )
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            mi, "Tiling", 2.0
        )
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            mi, "Tint", unreal.LinearColor(tint[0], tint[1], tint[2], 1.0)
        )
        # Re-bind textures on the MI so they never fall back to default green
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
            mi, "BaseColor", unreal.EditorAssetLibrary.load_asset(TEX_BC)
        )
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
            mi, "Normal", unreal.EditorAssetLibrary.load_asset(TEX_N)
        )
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
            mi, "RoughnessMap", unreal.EditorAssetLibrary.load_asset(TEX_R)
        )
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
            mi, "AmbientOcclusion", unreal.EditorAssetLibrary.load_asset(TEX_AO)
        )
        unreal.EditorAssetLibrary.save_asset(full)
        unreal.log(f"Tile MI {full} amp={amp}")


def enable_wpo_on_tile_bp():
    """Turn on Evaluate World Position Offset for meshes in BP_TerrainTile if present."""
    bp_path = "/Game/Terrain/BP_TerrainTile"
    if not unreal.EditorAssetLibrary.does_asset_exist(bp_path):
        unreal.log_warning("BP_TerrainTile missing")
        return
    bp = unreal.EditorAssetLibrary.load_asset(bp_path)
    try:
        # BlueprintEditorLibrary / SubobjectData — best-effort via generated class CDO
        gen = bp.generated_class()
        cdo = unreal.get_default_object(gen)
        comps = cdo.get_components_by_class(unreal.StaticMeshComponent.static_class())
        for comp in comps:
            for prop in (
                "evaluate_world_position_offset",
                "world_position_offset_disable_distance",
                "evaluate_world_position_offset_in_ray_tracing",
            ):
                try:
                    if prop == "evaluate_world_position_offset":
                        comp.set_editor_property(prop, True)
                    elif prop == "world_position_offset_disable_distance":
                        comp.set_editor_property(prop, 0)
                    elif prop == "evaluate_world_position_offset_in_ray_tracing":
                        comp.set_editor_property(prop, True)
                except Exception:
                    pass
            unreal.log(f"Enabled WPO on component {comp.get_name()}")
        unreal.EditorAssetLibrary.save_asset(bp_path)
    except Exception as exc:
        unreal.log_warning(f"Could not tweak BP_TerrainTile WPO flags: {exc}")


def main():
    parent = rebuild_parent()
    if not parent:
        return
    rebuild_tile_mis(parent)
    enable_wpo_on_tile_bp()
    unreal.log("DONE: stylized grass textures + visible Shader 1 wave on ground tiles")


if __name__ == "__main__":
    main()

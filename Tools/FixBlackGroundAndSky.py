"""
Fix black ground + ensure cartoon sky wraps like the preview.

Root causes this addresses:
1) SkyLight lower hemisphere black / weak lighting after removing SkyAtmosphere
2) Post-process exposure lock crushing the scene
3) Ground materials possibly too dark / odd values
4) Sky sphere setup so IsSky material can render as background
"""

import unreal


MAP_PATH = "/Game/Maps/L_PrincessTD"
SKY_MI = "/Game/QMS_Cartoon_Skybox_Free/Material_Instances/MI_Sky0002_Day.MI_Sky0002_Day"
SPHERE_MESH = "/Engine/BasicShapes/Sphere.Sphere"
SKY_LABEL = "CartoonSkybox_Day0002"


def _expr(mat, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)


def _connect(a, ap, b, bp):
    unreal.MaterialEditingLibrary.connect_material_expressions(a, ap, b, bp)


def rebuild_ground_material(asset_path, asset_name, color, amplitude=2.0):
    """Bright Default Lit ground with subtle vertex wave (keeps Shader 1 behaviour)."""
    full = f"{asset_path}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        mat = unreal.EditorAssetLibrary.load_asset(full)
        if mat:
            unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    else:
        if not unreal.EditorAssetLibrary.does_directory_exist(asset_path):
            unreal.EditorAssetLibrary.make_directory(asset_path)
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, asset_path, unreal.Material, unreal.MaterialFactoryNew()
        )

    if not mat:
        unreal.log_error(f"Could not create/load material {full}")
        return None

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property("two_sided", False)

    base = _expr(mat, unreal.MaterialExpressionConstant3Vector, -600, -200)
    base.set_editor_property("constant", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    metallic = _expr(mat, unreal.MaterialExpressionConstant, -600, -50)
    metallic.set_editor_property("r", 0.0)
    unreal.MaterialEditingLibrary.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)

    spec = _expr(mat, unreal.MaterialExpressionConstant, -600, 20)
    spec.set_editor_property("r", 0.2)
    unreal.MaterialEditingLibrary.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    rough = _expr(mat, unreal.MaterialExpressionConstant, -600, 90)
    rough.set_editor_property("r", 0.9)
    unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # Subtle WPO so Shader 1 still counts, without destroying lighting/readability
    time_node = _expr(mat, unreal.MaterialExpressionTime, -1000, 300)
    speed = _expr(mat, unreal.MaterialExpressionConstant, -800, 250)
    speed.set_editor_property("r", 1.2)
    speed_mul = _expr(mat, unreal.MaterialExpressionMultiply, -600, 280)
    _connect(time_node, "", speed_mul, "A")
    _connect(speed, "", speed_mul, "B")

    wp = _expr(mat, unreal.MaterialExpressionWorldPosition, -1000, 450)
    mask_x = _expr(mat, unreal.MaterialExpressionComponentMask, -800, 400)
    mask_x.set_editor_property("r", True)
    mask_x.set_editor_property("g", False)
    mask_x.set_editor_property("b", False)
    mask_x.set_editor_property("a", False)
    _connect(wp, "", mask_x, "")
    mask_y = _expr(mat, unreal.MaterialExpressionComponentMask, -800, 500)
    mask_y.set_editor_property("r", False)
    mask_y.set_editor_property("g", True)
    mask_y.set_editor_property("b", False)
    mask_y.set_editor_property("a", False)
    _connect(wp, "", mask_y, "")
    add_xy = _expr(mat, unreal.MaterialExpressionAdd, -600, 450)
    _connect(mask_x, "", add_xy, "A")
    _connect(mask_y, "", add_xy, "B")
    freq = _expr(mat, unreal.MaterialExpressionConstant, -600, 560)
    freq.set_editor_property("r", 0.04)
    freq_mul = _expr(mat, unreal.MaterialExpressionMultiply, -400, 450)
    _connect(add_xy, "", freq_mul, "A")
    _connect(freq, "", freq_mul, "B")
    phase = _expr(mat, unreal.MaterialExpressionAdd, -200, 330)
    _connect(speed_mul, "", phase, "A")
    _connect(freq_mul, "", phase, "B")
    sine = _expr(mat, unreal.MaterialExpressionSine, 0, 330)
    _connect(phase, "", sine, "")
    amp = _expr(mat, unreal.MaterialExpressionConstant, 0, 430)
    amp.set_editor_property("r", float(amplitude))
    wave = _expr(mat, unreal.MaterialExpressionMultiply, 200, 330)
    _connect(sine, "", wave, "A")
    _connect(amp, "", wave, "B")
    zero = _expr(mat, unreal.MaterialExpressionConstant, 200, 200)
    zero.set_editor_property("r", 0.0)
    append_xy = _expr(mat, unreal.MaterialExpressionAppendVector, 400, 230)
    _connect(zero, "", append_xy, "A")
    _connect(zero, "", append_xy, "B")
    append_xyz = _expr(mat, unreal.MaterialExpressionAppendVector, 600, 300)
    _connect(append_xy, "", append_xyz, "A")
    _connect(wave, "", append_xyz, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        append_xyz, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(mat)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(full)
    unreal.log(f"Rebuilt ground material {full}")
    return mat


def rebuild_waving_grass_showcase():
    # Keep /Game/Shaders/M_WavingGrass as the named Part 3 shader asset
    rebuild_ground_material("/Game/Shaders", "M_WavingGrass", (0.25, 0.55, 0.18), amplitude=3.0)


def fix_map_lighting_and_sky():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)

    for actor in list(unreal.EditorLevelLibrary.get_all_level_actors()):
        cls = actor.get_class().get_name()
        label = actor.get_actor_label()
        if cls in ("SkyAtmosphere", "VolumetricCloud"):
            unreal.EditorLevelLibrary.destroy_actor(actor)
        if label == "PP_CartoonDayExposure":
            unreal.EditorLevelLibrary.destroy_actor(actor)
            unreal.log("Removed crushing exposure volume")

    mi = unreal.EditorAssetLibrary.load_asset(SKY_MI)
    sphere = unreal.EditorAssetLibrary.load_asset(SPHERE_MESH)

    sky = None
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() == SKY_LABEL:
            sky = actor
            break
    if not sky:
        sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor.static_class(),
            unreal.Vector(0.0, 0.0, 0.0),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        sky.set_actor_label(SKY_LABEL)

    sky.set_folder_path("Atmosphere")
    sky.set_actor_location(unreal.Vector(0.0, 0.0, 0.0), False, True)
    # Positive huge scale; parent material is IsSky + TwoSided so it wraps like the preview.
    sky.set_actor_scale3d(unreal.Vector(100000.0, 100000.0, 100000.0))
    sky.set_actor_hidden_in_game(False)

    mesh = sky.static_mesh_component
    mesh.set_static_mesh(sphere)
    mesh.set_material(0, mi)
    mesh.set_visibility(True)
    mesh.set_hidden_in_game(False)
    mesh.set_cast_shadow(False)
    mesh.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    for prop, value in (
        ("cast_contact_shadow", False),
        ("cast_dynamic_shadow", False),
        ("cast_static_shadow", False),
        ("affect_distance_field_lighting", False),
        ("receives_decals", False),
    ):
        try:
            mesh.set_editor_property(prop, value)
        except Exception:
            pass

    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if isinstance(actor, unreal.DirectionalLight):
            light = actor.light_component
            light.set_intensity(20.0)
            light.set_light_color(unreal.LinearColor(1.0, 0.96, 0.88, 1.0))
            try:
                light.set_editor_property("cast_shadows", True)
            except Exception:
                pass
            try:
                light.set_editor_property("atmosphere_sun_light", False)
            except Exception:
                pass
            actor.set_actor_rotation(unreal.Rotator(-55.0, -25.0, 0.0), False)
            unreal.log("DirectionalLight intensity=20")
        elif isinstance(actor, unreal.SkyLight):
            light = actor.light_component
            try:
                light.set_editor_property("real_time_capture", True)
            except Exception:
                pass
            try:
                light.set_editor_property("lower_hemisphere_is_black", False)
            except Exception:
                pass
            try:
                light.set_editor_property("lower_hemisphere_color", unreal.LinearColor(0.45, 0.65, 0.35, 1.0))
            except Exception:
                pass
            light.set_intensity(4.0)
            try:
                light.recapture_sky()
            except Exception:
                pass
            unreal.log("SkyLight intensity=4, lower hemisphere not black")
        elif isinstance(actor, unreal.ExponentialHeightFog):
            comps = actor.get_components_by_class(unreal.ExponentialHeightFogComponent.static_class())
            if comps:
                try:
                    comps[0].set_editor_property("fog_density", 0.002)
                except Exception:
                    pass

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log("Map lighting/sky fix saved")


def main():
    rebuild_waving_grass_showcase()
    rebuild_ground_material("/Game/Terrain/Materials", "M_TileGroundA", (0.30, 0.62, 0.22), 2.0)
    rebuild_ground_material("/Game/Terrain/Materials", "M_TileGroundB", (0.38, 0.58, 0.20), 2.0)
    rebuild_ground_material("/Game/Terrain/Materials", "M_TileGroundC", (0.26, 0.54, 0.28), 2.0)
    fix_map_lighting_and_sky()
    unreal.log("DONE: bright ground materials + sky lighting fix")


if __name__ == "__main__":
    main()

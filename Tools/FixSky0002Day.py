"""
Fix cartoon skybox so it wraps 360 like the preview, and restore ground lighting.
"""

import unreal

MAP_PATH = "/Game/Maps/L_PrincessTD"
SKY_MI = "/Game/QMS_Cartoon_Skybox_Free/Material_Instances/MI_Sky0002_Day.MI_Sky0002_Day"
SPHERE_MESH = "/Engine/BasicShapes/Sphere.Sphere"
SKY_LABEL = "CartoonSkybox_Day0002"


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    actors = list(unreal.EditorLevelLibrary.get_all_level_actors())

    # Remove engine atmosphere again if it came back
    for actor in list(actors):
        cls = actor.get_class().get_name()
        if cls in ("SkyAtmosphere", "VolumetricCloud"):
            unreal.EditorLevelLibrary.destroy_actor(actor)
            unreal.log(f"Removed {cls}")

    actors = list(unreal.EditorLevelLibrary.get_all_level_actors())
    mi = unreal.EditorAssetLibrary.load_asset(SKY_MI)
    sphere = unreal.EditorAssetLibrary.load_asset(SPHERE_MESH)

    sky = None
    for actor in actors:
        if actor.get_actor_label() == SKY_LABEL or (
            isinstance(actor, unreal.StaticMeshActor)
            and actor.static_mesh_component
            and actor.static_mesh_component.get_material(0)
            and "MI_Sky0002_Day" in actor.static_mesh_component.get_material(0).get_path_name()
        ):
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
    # Negative scale flips normals so the painted sky wraps around you (inside of sphere).
    sky.set_actor_scale3d(unreal.Vector(-10000.0, -10000.0, -10000.0))
    sky.set_actor_hidden_in_game(False)

    mesh = sky.static_mesh_component
    mesh.set_static_mesh(sphere)
    mesh.set_material(0, mi)
    mesh.set_visibility(True)
    mesh.set_hidden_in_game(False)
    mesh.set_cast_shadow(False)
    mesh.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    try:
        mesh.set_editor_property("affect_distance_field_lighting", False)
        mesh.set_editor_property("cast_contact_shadow", False)
        mesh.set_editor_property("cast_dynamic_shadow", False)
        mesh.set_editor_property("cast_static_shadow", False)
        mesh.set_editor_property("visible_in_ray_tracing", True)
    except Exception:
        pass

    # Strong day lighting so Default Lit ground materials are not black without SkyAtmosphere.
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if isinstance(actor, unreal.DirectionalLight):
            light = actor.light_component
            light.set_intensity(12.0)
            light.set_light_color(unreal.LinearColor(1.0, 0.95, 0.85, 1.0))
            try:
                light.set_editor_property("atmosphere_sun_light", False)
            except Exception:
                pass
            actor.set_actor_rotation(unreal.Rotator(-50.0, -35.0, 0.0), False)
            unreal.log("Boosted DirectionalLight")
        elif isinstance(actor, unreal.SkyLight):
            light = actor.light_component
            try:
                light.set_editor_property("real_time_capture", True)
            except Exception:
                pass
            light.set_intensity(2.5)
            try:
                light.set_editor_property("lower_hemisphere_is_black", False)
            except Exception:
                pass
            try:
                light.recapture_sky()
            except Exception:
                pass
            unreal.log("Boosted SkyLight + recapture")
        elif isinstance(actor, unreal.ExponentialHeightFog):
            comps = actor.get_components_by_class(unreal.ExponentialHeightFogComponent.static_class())
            if comps:
                fog = comps[0]
                try:
                    fog.set_editor_property("fog_density", 0.005)
                except Exception:
                    pass
                try:
                    fog.set_editor_property("fog_inscattering_color", unreal.LinearColor(0.55, 0.7, 0.95, 1.0))
                except Exception:
                    pass

    # Stable exposure so the scene does not crush to black.
    pp = None
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if isinstance(actor, unreal.PostProcessVolume):
            pp = actor
            break
    if not pp:
        pp = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.PostProcessVolume.static_class(),
            unreal.Vector(0.0, 0.0, 0.0),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        pp.set_actor_label("PP_CartoonDayExposure")
        pp.set_folder_path("Atmosphere")

    try:
        pp.set_editor_property("unbound", True)
    except Exception:
        pass
    try:
        settings = pp.get_editor_property("settings")
        settings.set_editor_property("override_auto_exposure_method", True)
        settings.set_editor_property("override_auto_exposure_min_brightness", True)
        settings.set_editor_property("override_auto_exposure_max_brightness", True)
        settings.set_editor_property("auto_exposure_min_brightness", 1.0)
        settings.set_editor_property("auto_exposure_max_brightness", 1.0)
        pp.set_editor_property("settings", settings)
        unreal.log("Locked exposure on PostProcessVolume")
    except Exception as exc:
        unreal.log_warning(f"Could not set PP exposure: {exc}")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log("Sky wrap-around + ground lighting fix saved to L_PrincessTD")


if __name__ == "__main__":
    main()

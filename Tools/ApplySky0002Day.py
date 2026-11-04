"""
Make L_PrincessTD use the QMS Preview_Sky0002_Day look:
- Large sky sphere with MI_Sky0002_Day
- Disable SkyAtmosphere / VolumetricCloud that wash out the cartoon skybox
- Tweak DirectionalLight + SkyLight for a clear day
"""

import unreal


MAP_PATH = "/Game/Maps/L_PrincessTD"
SKY_MI = "/Game/QMS_Cartoon_Skybox_Free/Material_Instances/MI_Sky0002_Day.MI_Sky0002_Day"
SPHERE_MESH = "/Engine/BasicShapes/Sphere.Sphere"
SKY_ACTOR_LABEL = "CartoonSkybox_Day0002"


def _load_map():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)


def _get_actors():
    return list(unreal.EditorLevelLibrary.get_all_level_actors())


def _disable_engine_sky(actors):
    removed = 0
    for actor in actors:
        name = actor.get_class().get_name()
        label = actor.get_actor_label()
        # Preview map does not use these; they override the painted skybox.
        if name in ("SkyAtmosphere", "VolumetricCloud") or label.startswith(("SkyAtmosphere", "VolumetricCloud")):
            unreal.EditorLevelLibrary.destroy_actor(actor)
            removed += 1
            unreal.log(f"Removed conflicting sky actor: {label} ({name})")
    return removed


def _find_sky_actor(actors):
    for actor in actors:
        label = actor.get_actor_label()
        if label == SKY_ACTOR_LABEL or label in ("CartoonSkybox", "SM_SkySphere", "SkySphere"):
            return actor
        # Existing sky sphere mesh actors that already reference the MI
        if isinstance(actor, unreal.StaticMeshActor):
            mesh_comp = actor.static_mesh_component
            if mesh_comp:
                mat = mesh_comp.get_material(0)
                if mat and "MI_Sky0002_Day" in mat.get_path_name():
                    return actor
    return None


def _ensure_sky_sphere():
    actors = _get_actors()
    sky = _find_sky_actor(actors)
    mi = unreal.EditorAssetLibrary.load_asset(SKY_MI)
    sphere = unreal.EditorAssetLibrary.load_asset(SPHERE_MESH)
    if not mi or not sphere:
        unreal.log_error("Missing sky material or sphere mesh")
        return None

    if not sky:
        sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor.static_class(),
            unreal.Vector(0.0, 0.0, 0.0),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        unreal.log("Spawned new CartoonSkybox_Day0002")

    sky.set_actor_label(SKY_ACTOR_LABEL)
    sky.set_folder_path("Atmosphere")
    sky.set_actor_location(unreal.Vector(0.0, 0.0, 0.0), False, True)
    sky.set_actor_scale3d(unreal.Vector(5000.0, 5000.0, 5000.0))  # huge inverted-looking dome via material

    mesh_comp = sky.static_mesh_component
    mesh_comp.set_static_mesh(sphere)
    mesh_comp.set_material(0, mi)
    mesh_comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    mesh_comp.set_cast_shadow(False)
    mesh_comp.set_editor_property("affect_distance_field_lighting", False)
    # Render behind everything
    try:
        mesh_comp.set_editor_property("translucency_sort_priority", -100)
    except Exception:
        pass

    return sky


def _configure_lights(actors):
    for actor in actors:
        if isinstance(actor, unreal.DirectionalLight):
            # Warm daylight, similar to a clear cartoon day
            light = actor.light_component
            light.set_editor_property("intensity", 5.0)
            light.set_editor_property("light_color", unreal.Color(255, 244, 214, 255))
            actor.set_actor_rotation(unreal.Rotator(-40.0, -30.0, 0.0), False)
            unreal.log(f"Configured DirectionalLight: {actor.get_actor_label()}")
        elif isinstance(actor, unreal.SkyLight):
            light = actor.light_component
            # Recapture scene so the sky sphere feeds the skylight
            try:
                light.set_editor_property("real_time_capture", True)
            except Exception:
                pass
            light.set_editor_property("intensity", 1.0)
            try:
                light.recapture_sky()
            except Exception:
                pass
            unreal.log(f"Configured SkyLight: {actor.get_actor_label()}")
        elif isinstance(actor, unreal.ExponentialHeightFog):
            # Keep a little depth, but don't milky-out the cartoon sky
            fog = None
            for attr in ("height_fog_component", "root_component", "root"):
                fog = getattr(actor, attr, None)
                if fog:
                    break
            if fog is None:
                comps = actor.get_components_by_class(unreal.ExponentialHeightFogComponent.static_class())
                fog = comps[0] if comps else None
            if fog:
                try:
                    fog.set_editor_property("fog_density", 0.01)
                except Exception:
                    pass
                try:
                    fog.set_editor_property("fog_height_falloff", 0.2)
                except Exception:
                    pass
                unreal.log(f"Softened ExponentialHeightFog: {actor.get_actor_label()}")


def main():
    _load_map()
    actors = _get_actors()
    removed = _disable_engine_sky(actors)
    sky = _ensure_sky_sphere()
    _configure_lights(_get_actors())

    # Save current level
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(
        f"L_PrincessTD sky set to Preview_Sky0002_Day style "
        f"(removed {removed} atmosphere actors, sky={sky.get_actor_label() if sky else 'NONE'})"
    )


if __name__ == "__main__":
    main()

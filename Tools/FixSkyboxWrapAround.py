# Full-wrap cartoon sky like Preview_Sky0002_Day:
# remove height fog (blacks out lower sky), replace static sphere with camera-following actor.
import unreal

MAP_PATH = "/Game/Maps/L_PrincessTD"
SKY_LABEL = "CartoonSkybox_Day0002"
OLD_LABELS = ("CartoonSkybox_Day0002", "CartoonSkybox", "SM_SkySphere", "SkySphere")
PREVIEW_SCALE = 100.0


def level_center(world):
    skip = (
        unreal.DirectionalLight,
        unreal.SkyLight,
        unreal.PointLight,
        unreal.SpotLight,
        unreal.RectLight,
        unreal.ExponentialHeightFog,
        unreal.PostProcessVolume,
        unreal.PlayerStart,
        unreal.CameraActor,
        unreal.SkyAtmosphere,
        unreal.VolumetricCloud,
    )
    mins = [1e12, 1e12, 1e12]
    maxs = [-1e12, -1e12, -1e12]
    n = 0
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if isinstance(a, skip):
            continue
        if a.get_actor_label() in OLD_LABELS:
            continue
        loc = a.get_actor_location()
        if abs(loc.x) > 1e7 or abs(loc.y) > 1e7 or abs(loc.z) > 1e7:
            continue
        mins[0] = min(mins[0], loc.x)
        mins[1] = min(mins[1], loc.y)
        mins[2] = min(mins[2], loc.z)
        maxs[0] = max(maxs[0], loc.x)
        maxs[1] = max(maxs[1], loc.y)
        maxs[2] = max(maxs[2], loc.z)
        n += 1
    if n == 0:
        return unreal.Vector(0.0, 0.0, 0.0)
    return unreal.Vector(
        (mins[0] + maxs[0]) * 0.5,
        (mins[1] + maxs[1]) * 0.5,
        (mins[2] + maxs[2]) * 0.5,
    )


def destroy_black_sky_culprits(world):
    removed = 0
    for a in list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)):
        cls = a.get_class().get_name()
        label = a.get_actor_label()
        if cls in ("ExponentialHeightFog", "SkyAtmosphere", "VolumetricCloud"):
            unreal.EditorLevelLibrary.destroy_actor(a)
            removed += 1
            unreal.log(f"Removed {cls} ({label})")
        elif label == "PP_CartoonDayExposure":
            unreal.EditorLevelLibrary.destroy_actor(a)
            removed += 1
    return removed


def remove_old_sky_actors(world):
    removed = 0
    for a in list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)):
        label = a.get_actor_label()
        cls = a.get_class().get_name()
        if label in OLD_LABELS or cls == "CartoonSkyboxActor":
            unreal.EditorLevelLibrary.destroy_actor(a)
            removed += 1
            unreal.log(f"Removed old sky actor {label} ({cls})")
    return removed


def spawn_following_sky(world, center):
    cls = unreal.load_class(None, "/Script/GADE7322POE.CartoonSkyboxActor")
    if not cls:
        unreal.log_error("CartoonSkyboxActor missing — compile the GADE7322POE module first")
        return None

    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(cls, center, unreal.Rotator(0, 0, 0))
    sky.set_actor_label(SKY_LABEL)
    sky.set_folder_path("Atmosphere")
    # spawn_actor_from_class often ignores the location arg — force it.
    sky.set_actor_location(center, False, True)
    sky.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
    try:
        sky.set_editor_property("sphere_scale", PREVIEW_SCALE)
    except Exception:
        pass
    sky.set_actor_scale3d(unreal.Vector(PREVIEW_SCALE, PREVIEW_SCALE, PREVIEW_SCALE))
    sky.set_actor_hidden_in_game(False)
    loc = sky.get_actor_location()
    unreal.log(f"Spawned CartoonSkyboxActor at {loc} scale={PREVIEW_SCALE}")
    return sky


def fix_skylight(world):
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SkyLight):
        c = a.light_component
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
        try:
            c.set_editor_property("real_time_capture", True)
        except Exception:
            pass
        c.set_intensity(3.0)
        try:
            c.recapture_sky()
        except Exception:
            pass
        unreal.log(f"SkyLight '{a.get_actor_label()}': lower hemisphere NOT black")


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    world = unreal.EditorLevelLibrary.get_editor_world()
    fog_removed = destroy_black_sky_culprits(world)
    old_removed = remove_old_sky_actors(world)
    center = level_center(world)
    sky = spawn_following_sky(world, center)
    fix_skylight(world)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(
        f"OK: full-wrap sky (fog_removed={fog_removed}, old_sky_removed={old_removed}, "
        f"sky={'yes' if sky else 'NO'})"
    )


if __name__ == "__main__":
    main()

"""Dump SCS / spawned components for defender pad actors."""
import unreal

def log_scs(path: str) -> None:
    bp = unreal.EditorAssetLibrary.load_asset(path)
    unreal.log(f"==== SCS {path} type={bp.get_class().get_name() if bp else None} ====")
    if not bp:
        return
    scs = None
    for getter in ("simple_construction_script", "get_editor_property"):
        try:
            if getter == "simple_construction_script":
                scs = bp.simple_construction_script
            else:
                scs = bp.get_editor_property("simple_construction_script")
            break
        except Exception:
            continue
    if not scs:
        unreal.log("  no SCS")
        return
    try:
        nodes = scs.get_all_nodes()
    except Exception as exc:
        unreal.log(f"  get_all_nodes failed: {exc}")
        return
    for node in nodes:
        try:
            tmpl = node.component_template
        except Exception:
            tmpl = None
        name = node.get_variable_name() if hasattr(node, "get_variable_name") else node.get_name()
        line = f"  node {name} class={node.get_class().get_name()}"
        if tmpl:
            line += f" tmpl={tmpl.get_class().get_name()} '{tmpl.get_name()}'"
            if isinstance(tmpl, unreal.StaticMeshComponent):
                mesh = tmpl.static_mesh
                line += f" mesh={mesh.get_path_name() if mesh else None}"
                try:
                    line += f" loc={tmpl.get_editor_property('relative_location')} scale={tmpl.get_editor_property('relative_scale3d')}"
                except Exception:
                    pass
                try:
                    mats = tmpl.get_materials()
                    line += f" mats={[m.get_name() if m else None for m in mats]}"
                except Exception:
                    pass
        unreal.log(line)


def spawn_dump(path: str) -> None:
    short = path.split("/")[-1]
    cls = unreal.load_class(None, f"{path}.{short}_C")
    if not cls:
        return
    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        unreal.log("  no editor world")
        return
    actor = world.spawn_actor(cls, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if not actor:
        unreal.log(f"  spawn failed {path}")
        return
    unreal.log(f"==== SPAWNED {path} ====")
    comps = actor.get_components_by_class(unreal.SceneComponent)
    for c in comps:
        extra = ""
        if isinstance(c, unreal.StaticMeshComponent):
            mesh = c.static_mesh
            extra = f" mesh={mesh.get_name() if mesh else None}"
            try:
                extra += f" loc={c.get_editor_property('relative_location')} scale={c.get_editor_property('relative_scale3d')}"
                mats = c.get_materials()
                extra += f" mats={[m.get_name() if m else None for m in mats]}"
            except Exception:
                pass
        unreal.log(f"  {c.get_class().get_name()} '{c.get_name()}'{extra}")
    actor.destroy_actor()


def main():
    for p in ["/Game/Defenders/BP_DefenderSlot", "/Game/Terrain/BP_TerrainTile"]:
        log_scs(p)
        spawn_dump(p)
    unreal.log("DONE inspect slots")


if __name__ == "__main__":
    main()

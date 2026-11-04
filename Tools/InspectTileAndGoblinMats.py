import unreal

def dump_bp(path):
    unreal.log(f"==== {path}")
    bp = unreal.EditorAssetLibrary.load_asset(path)
    if not bp:
        unreal.log("missing")
        return
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = sds.k2_gather_subobject_data_for_blueprint(bp)
    for h in handles:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if not obj:
            continue
        cls = obj.get_class().get_name()
        name = obj.get_name()
        unreal.log(f"comp {name} ({cls})")
        if isinstance(obj, unreal.MeshComponent) or "Mesh" in cls:
            try:
                n = obj.get_num_materials()
            except Exception:
                n = -1
            unreal.log(f"  num_materials={n}")
            for i in range(max(n, 0)):
                try:
                    unreal.log(f"  slot{i}={obj.get_material(i)}")
                except Exception as ex:
                    unreal.log(f"  slot{i} err {ex}")
            for prop in ("static_mesh", "override_materials", "evaluate_world_position_offset"):
                try:
                    unreal.log(f"  {prop}={obj.get_editor_property(prop)}")
                except Exception:
                    pass

    # CDO variable properties that look like materials
    gen = bp.generated_class()
    cdo = unreal.get_default_object(gen)
    for prop in cdo.get_class().properties:
        try:
            pname = prop.get_name()
            if any(k in pname.lower() for k in ("mat", "ground", "tile", "grass", "mesh", "color")):
                val = cdo.get_editor_property(pname) if False else None
        except Exception:
            pass
    # iterate with reflection via unreal
    for pname in dir(cdo):
        pass
    try:
        for u_prop in gen.properties:
            name = u_prop.get_name()
            lname = name.lower()
            if any(k in lname for k in ("material", "ground", "tile", "mat", "grass")):
                try:
                    val = cdo.get_editor_property(name)
                    unreal.log(f"CDO.{name} = {val}")
                except Exception as ex:
                    unreal.log(f"CDO.{name} err {ex}")
    except Exception as ex:
        unreal.log(f"prop walk failed {ex}")


dump_bp("/Game/Terrain/BP_TerrainTile")
dump_bp("/Game/Enemies/BP_GoblinEnemy")

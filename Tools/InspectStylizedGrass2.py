import unreal

mat = unreal.EditorAssetLibrary.load_asset("/Game/Shaders/M_WavingGrass")
for e in unreal.MaterialEditingLibrary.get_material_expressions(mat):
    cls = e.get_class().get_name()
    info = cls
    for prop in ("parameter_name", "texture", "default_value", "r", "constant"):
        try:
            val = e.get_editor_property(prop)
            info += f" {prop}={val}"
        except Exception:
            pass
    unreal.log(info)

# SCS components on BP
bp = unreal.EditorAssetLibrary.load_asset("/Game/Terrain/BP_TerrainTile")
try:
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
except Exception:
    subsystem = None

# Fallback: simple construction script components via EditorAssetLibrary
try:
    handles = unreal.SubobjectDataBlueprintFunctionLibrary.get_blueprint_subobjects(bp)  # may not exist
    unreal.log(f"handles={handles}")
except Exception as ex:
    unreal.log(f"subobjects fail: {ex}")

# Try loading generated class and iterating Actor's default subobjects
gen = bp.generated_class()
cdo = gen.get_default_object()
unreal.log(f"root={cdo.root_component}")
if cdo.root_component:
    unreal.log(f"root class={cdo.root_component.get_class().get_name()} name={cdo.root_component.get_name()}")
    try:
        unreal.log(f"eval_wpo={cdo.root_component.get_editor_property('evaluate_world_position_offset')}")
    except Exception as ex:
        unreal.log(f"eval err {ex}")
    try:
        mesh = cdo.root_component.get_editor_property("static_mesh")
        unreal.log(f"mesh={mesh}")
        mats = cdo.root_component.get_materials()
        unreal.log(f"materials={mats}")
    except Exception as ex:
        unreal.log(f"mesh err {ex}")

# Check material editor connections to BaseColor / WPO by compiling stats
unreal.log(f"mat blend={mat.get_editor_property('blend_mode')} shading={mat.get_editor_property('shading_model')}")
try:
    unreal.log(f"bUsedWithStaticLighting={mat.get_editor_property('used_with_static_lighting')}")
except Exception:
    pass
for flag in (
    "bUsedWithSkeletalMesh",
    "bUsedWithParticleSprites",
    "bUsedWithMeshParticles",
    "bUsedWithNiagaraMeshParticles",
    "bUsedWithNanite",
):
    try:
        unreal.log(f"{flag}={mat.get_editor_property(flag)}")
    except Exception:
        try:
            # UE5 naming without b prefix
            name = flag[1].lower() + flag[2:] if flag.startswith("b") else flag
            name = flag.replace("bUsed", "used") if flag.startswith("bUsed") else flag
            unreal.log(f"alt {flag}")
        except Exception:
            pass

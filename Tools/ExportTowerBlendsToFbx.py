"""Blender batch: export each .blend in Content/Imports/Towers to FBX (Unreal-friendly)."""
import os

import bpy

TOWERS_DIR = r"c:\Users\jamdl\OneDrive\Documents\Unreal Projects\GADE7322POE\Content\Imports\Towers"


def export_one(blend_path: str, fbx_path: str) -> None:
    bpy.ops.wm.read_homefile(use_empty=True)
    bpy.ops.wm.open_mainfile(filepath=blend_path)

    # Prefer WORKBENCH if EEVEE missing (version mismatch).
    try:
        bpy.context.scene.render.engine = "BLENDER_WORKBENCH"
    except Exception:
        pass

    meshes = [obj for obj in bpy.data.objects if obj.type == "MESH"]
    if not meshes:
        # Sometimes meshes are only linked into the view layer after update
        bpy.context.view_layer.update()
        meshes = [obj for obj in bpy.data.objects if obj.type == "MESH"]
    if not meshes:
        print(f"WARN: no meshes in {blend_path}. Objects={[o.name+':'+o.type for o in bpy.data.objects]}")
        return

    # Deselect everything without ops
    for obj in bpy.data.objects:
        obj.select_set(False)

    for obj in meshes:
        obj.hide_set(False)
        obj.hide_viewport = False
        obj.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]

    # Join into one mesh for a single StaticMesh asset (optional but simpler for towers)
    if len(meshes) > 1:
        try:
            bpy.ops.object.join()
        except Exception as ex:
            print(f"join skipped: {ex}")

    bpy.ops.export_scene.fbx(
        filepath=fbx_path,
        use_selection=True,
        object_types={"MESH"},
        apply_scale_options="FBX_SCALE_ALL",
        axis_forward="-Y",
        axis_up="Z",
        apply_unit_scale=True,
        bake_space_transform=True,
        mesh_smooth_type="FACE",
        use_mesh_modifiers=True,
        add_leaf_bones=False,
        path_mode="COPY",
        embed_textures=True,
    )
    print(f"Exported {fbx_path} ({len(meshes)} source meshes)")


def main():
    for name in sorted(os.listdir(TOWERS_DIR)):
        if not name.lower().endswith(".blend"):
            continue
        blend = os.path.join(TOWERS_DIR, name)
        fbx = os.path.join(TOWERS_DIR, os.path.splitext(name)[0] + ".fbx")
        try:
            export_one(blend, fbx)
        except Exception as ex:
            print(f"ERROR exporting {name}: {ex}")
            import traceback

            traceback.print_exc()
    print("DONE blender export")


if __name__ == "__main__":
    main()

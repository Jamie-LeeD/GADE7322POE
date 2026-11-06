"""Import tower FBX from Content/Imports/Towers into /Game/Imports/Towers as StaticMesh."""
import os
import unreal

DISK = r"c:\Users\jamdl\OneDrive\Documents\Unreal Projects\GADE7322POE\Content\Imports\Towers"
DEST = "/Game/Imports/Towers"


def main():
    if not unreal.EditorAssetLibrary.does_directory_exist(DEST):
        unreal.EditorAssetLibrary.make_directory(DEST)

    tasks = []
    for name in sorted(os.listdir(DISK)):
        if not name.lower().endswith(".fbx"):
            continue
        src = os.path.join(DISK, name)
        dest_name = os.path.splitext(name)[0]
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", src)
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("destination_name", dest_name)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)

        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_animations", False)
        options.set_editor_property("import_materials", True)
        options.set_editor_property("import_textures", True)
        options.set_editor_property("automated_import_should_detect_type", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        try:
            sm = options.get_editor_property("static_mesh_import_data")
            sm.set_editor_property("combine_meshes", True)
            sm.set_editor_property("auto_generate_collision", True)
            sm.set_editor_property("import_uniform_scale", 1.0)
        except Exception as ex:
            unreal.log_warning(f"static_mesh_import_data: {ex}")

        task.set_editor_property("options", options)
        tasks.append(task)
        unreal.log(f"Queued {src}")

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    for name in ("ArcherTower", "IceTower", "PaladinTower", "PrincessTower"):
        path = f"{DEST}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.log(f"OK {path}")
        else:
            # Sometimes FBX creates SM_ prefix or nested
            assets = unreal.EditorAssetLibrary.list_assets(DEST, recursive=True)
            matches = [a for a in assets if name.lower() in a.lower()]
            unreal.log_warning(f"Missing {path}; matches={matches}")

    unreal.log("DONE: tower mesh import")


if __name__ == "__main__":
    main()

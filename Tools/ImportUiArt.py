"""Import Content/UI/Art/*.png as UI Texture2D assets."""
import unreal

ART_DIR = "/Game/UI/Art"
DISK_DIR = r"c:\Users\jamdl\OneDrive\Documents\Unreal Projects\GADE7322POE\Content\UI\Art"


def main():
    if not unreal.EditorAssetLibrary.does_directory_exist(ART_DIR):
        unreal.EditorAssetLibrary.make_directory(ART_DIR)

    import_tasks = []
    import os

    for name in sorted(os.listdir(DISK_DIR)):
        if not name.lower().endswith(".png"):
            continue
        src = os.path.join(DISK_DIR, name)
        dest_name = os.path.splitext(name)[0]
        # Unreal asset names: no spaces
        dest_name = dest_name.replace(" ", "_")
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", src)
        task.set_editor_property("destination_path", ART_DIR)
        task.set_editor_property("destination_name", dest_name)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)

        factory = unreal.TextureFactory()
        task.set_editor_property("factory", factory)
        import_tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(import_tasks)

    # Configure as UI textures
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(ART_DIR, recursive=False)
    count = 0
    for data in assets:
        path = f"{data.package_path}/{data.asset_name}"
        tex = unreal.EditorAssetLibrary.load_asset(str(path))
        if not isinstance(tex, unreal.Texture2D):
            continue
        try:
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        except Exception:
            pass
        try:
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        except Exception:
            try:
                tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
            except Exception:
                pass
        try:
            tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        except Exception:
            pass
        try:
            tex.set_editor_property("srgb", True)
        except Exception:
            pass
        try:
            tex.set_editor_property("never_stream", True)
        except Exception:
            pass
        unreal.EditorAssetLibrary.save_asset(str(path))
        count += 1
        unreal.log(f"Configured UI texture {path}")

    unreal.log(f"DONE: imported/configured {count} UI textures in {ART_DIR}")


if __name__ == "__main__":
    main()

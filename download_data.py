import json
import os
import sys
from tqdm import tqdm

def safe_mkdir(dir_path):
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)

def download_file(remote_root, local_root, file_name):
    cmd = f"wget -q {remote_root}{file_name} -O {local_root}{file_name}"
    os.system(cmd)

if __name__ == "__main__":
    scene_list = [
        ["lego", 750],
        ["chair", 750],
        ["drums", 750],
        ["hotdog", 750],
        ["ship", 750],
        ["ficus", 750],
        ["materials", 750],
        ["spheres", "real_1000"],
        ["vasedeck", "real_1000"],
        ["pinecone", "real_1000"],
        ["toycar", "real_1000"]
    ]

    for scene in scene_list:
        scene_name = scene[0]
        scene_size = scene[1]
        print(f"Downloading {scene_name}...")
        scene_url_root = f"https://storage.googleapis.com/snerg/{scene_size}/{scene_name}/"
        local_root = f"models/{scene_name}/"
        
        # load the json file
        safe_mkdir(local_root)
        download_file(scene_url_root, local_root, "scene_params.json")
        scene_params = json.load(open(f"{local_root}scene_params.json"))
        n_slides = scene_params["num_slices"]

        # load images
        download_file(scene_url_root, local_root, "atlas_indices.png")
        for i in tqdm(range(n_slides)):
            feat_name = f"feature_{i:03d}.png"
            rgba_name = f"rgba_{i:03d}.png"
            download_file(scene_url_root, local_root, feat_name)
            download_file(scene_url_root, local_root, rgba_name)
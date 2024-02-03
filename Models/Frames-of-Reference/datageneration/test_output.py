import json
from dataclasses import dataclass, asdict
import sys

import numpy as np
from scipy.spatial.distance import cosine

from scene import BaseObject, OrientedObject, Scene
from my_data import MyInput
import constants as const

# Function to parse JSON data into MyInput objects
def parse_json_to_myinput(json_data):
    myinput_objects = []

    for item in json_data:
        # Parsing the nested data into appropriate classes
        figure = BaseObject(position=np.array(item['scene']['figure']['position']))
        ground = OrientedObject(
            position=np.array(item['scene']['ground']['position']),
            forward=np.array(item['scene']['ground']['forward']),
            upward=np.array(item['scene']['ground']['upward']),
            is_participant=item['scene']['ground']['is_participant'],
            body_type=item['scene']['ground']['body_type']
        )
        speaker = OrientedObject(
            position=np.array(item['scene']['speaker']['position']),
            forward=np.array(item['scene']['speaker']['forward']),
            upward=np.array(item['scene']['speaker']['upward']),
            is_participant=item['scene']['speaker']['is_participant'],
            body_type=item['scene']['speaker']['body_type']
        )

        scene = Scene(speaker=speaker, ground=ground, figure=figure)
        myinput = MyInput(scene=scene, description=item['description'], flip_results=item['flip_results'])

        myinput_objects.append(myinput)
    
    return myinput_objects

def print_input_object(input_obj):
    description = input_obj.description
    print(description)

    flip_results = input_obj.flip_results
    direct_status = "Direct" if flip_results["scene_is_direct"] else "Nondirect"
    int_or_rel = "Intrinsic" if flip_results["description_is_intrinsic"] else "Relative"
    ground = input_obj.scene.ground
    body_type = ground.body_type
    print(f"{direct_status} - {int_or_rel} - {body_type}")
    print(f"G: {ground.forward}")

    figure = input_obj.scene.figure
    print(f"F: {figure.position}")

    g_to_f = input_obj.scene.ground_figure_displacement()

    print("Ground")
    up_down_g = 1-cosine(g_to_f, ground.upward)
    front_back_g = 1-cosine(g_to_f, ground.forward)
    left_right_g = 1-cosine(g_to_f, ground.rightward)
    print(f"Up/Down: {up_down_g}")
    print(f"Front/Back: {front_back_g}")
    print(f"Left/Right: {left_right_g}")
    
    if direct_status == "Nondirect" and int_or_rel == "Relative":
        print("Speaker")
        speaker = input_obj.scene.speaker
        up_down_s = 1-cosine(g_to_f, speaker.upward)
        front_back_s = 1-cosine(g_to_f, speaker.forward)
        left_right_s = 1-cosine(g_to_f, speaker.rightward)
        print(f"Up/Down: {up_down_s}")
        print(f"Front/Back: {front_back_s}")
        print(f"Left/Right: {left_right_s}")

    print()

def matches_filters(input_obj, filters):
    flip_results = input_obj.flip_results
    for flip_label, flip_value in filters:
        if flip_label not in flip_results or flip_results[flip_label] != flip_value:
            return False
    return True

def main(file_path, filters):
    with open(file_path, 'r') as file:
        data = json.load(file)

    input_objects = parse_json_to_myinput(data)
    for input_obj in input_objects:
        if matches_filters(input_obj, filters):
            print_input_object(input_obj)

if __name__ == "__main__":
    import sys

    if len(sys.argv) != 2:
        print("Usage: python script_name.py <path_to_json_file>")
        sys.exit(1)

    json_file_path = sys.argv[1]
    filters = [
        ("description_is_angular", True),
        ("ground_is_canonical", False)
    ]
    main(json_file_path, filters)
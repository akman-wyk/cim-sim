import json
import argparse

def load_json_data(filepath):
    """Loads JSON data from the specified file."""
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Error: Input file '{filepath}' not found.")
        raise
    except json.JSONDecodeError as e:
        print(f"Error: Could not decode JSON from '{filepath}'. Malformed JSON? Details: {e}")
        raise

def save_json_data(data, filepath):
    """
    Saves the given data as JSON to the specified file,
    with each instruction object on a new line and formatted compactly.
    """
    try:
        with open(filepath, 'w') as f:
            f.write("{\n") # Start of the main JSON object
            num_cores = len(data)
            for core_idx, (core_id, instructions_list) in enumerate(data.items()):
                # Write the core ID key, indented
                f.write(f'  "{core_id}": [') # Start of the core's instruction list

                if not instructions_list:
                    f.write("]\n") # Close empty list and add newline
                else:
                    f.write("\n") # Newline after '[' before the first instruction
                    num_instructions = len(instructions_list)
                    for i, instruction in enumerate(instructions_list):
                        # Convert each instruction dictionary to a compact JSON string
                        # separators=(',', ':') ensures no extra spaces around delimiters
                        instruction_str = json.dumps(instruction, separators=(',', ':'))
                        # Indent for each instruction within the list
                        f.write(f'    {instruction_str}')
                        if i < num_instructions - 1:
                            f.write(',') # Add a comma if it's not the last instruction
                        f.write('\n') # Newline after each instruction
                    # Closing bracket for the list, indented to match the core key line
                    f.write('  ]') 

                if core_idx < num_cores - 1:
                    f.write(',') # Add a comma if it's not the last core
                f.write('\n') # Newline after the core's block (either after ']' or '],')

            f.write("}\n") # End of the main JSON object
        print(f"Successfully saved modified data to '{filepath}' with custom one-instruction-per-line formatting.")
    except IOError:
        print(f"Error: Could not write to output file '{filepath}'.")
        raise

def add_instruction_tags(data, target_core_id, start_index, num_instructions_to_tag, portion_name):
    """
    Adds 'inst_group_tag' to a specified portion of instructions for a given core.

    Args:
        data (dict): The loaded JSON data.
        target_core_id (str): The ID of the core whose instructions are to be tagged (e.g., "0").
        start_index (int): The starting index of the instruction list to begin tagging.
        num_instructions_to_tag (int): The number of instructions to tag from the start_index.
        portion_name (str): The custom name for this portion (e.g., "Init", "Loop1").

    Returns:
        dict: The modified data. (Modifies data in-place)
    """
    if target_core_id not in data:
        print(f"Warning: Core ID '{target_core_id}' not found in the JSON data. No changes made for this target.")
        return data

    instructions_list = data.get(target_core_id)

    if not isinstance(instructions_list, list):
        print(f"Warning: Data for Core ID '{target_core_id}' is not a list. Expected a list of instructions. Skipping this core.")
        return data

    num_total_instructions = len(instructions_list)
    effective_start_index = start_index 

    if effective_start_index < 0:
        print(f"Info: Start index ({effective_start_index}) for Core ID '{target_core_id}' is negative. Treating as 0.")
        effective_start_index = 0
        
    if num_instructions_to_tag <= 0:
        print(f"Info: Number of instructions to tag ({num_instructions_to_tag}) is zero or negative. No instructions will be tagged for Core ID '{target_core_id}'.")
        return data

    if effective_start_index >= num_total_instructions:
        print(f"Warning: Start index ({effective_start_index}) is out of bounds for Core ID '{target_core_id}' (total instructions: {num_total_instructions}). No instructions will be tagged for this core.")
        return data

    actual_end_index_exclusive = min(effective_start_index + num_instructions_to_tag, num_total_instructions)
    
    tag_value = f"CoreID{target_core_id}.Portion{portion_name}"
    
    tagged_count = 0
    for i in range(effective_start_index, actual_end_index_exclusive):
        instruction = instructions_list[i]
        if isinstance(instruction, dict):
            instruction["inst_group_tag"] = tag_value
            tagged_count += 1
        else:
            print(f"Warning: Item at index {i} for Core ID '{target_core_id}' is not a dictionary (instruction object). Skipping this item: {instruction}")

    if tagged_count > 0:
        print(f"Tagged {tagged_count} instructions in Core ID '{target_core_id}' (indices {effective_start_index} to {actual_end_index_exclusive - 1}) with '{tag_value}'.")
    elif effective_start_index < num_total_instructions: 
        print(f"No instructions were actually tagged for Core ID '{target_core_id}' in the range {effective_start_index} to {actual_end_index_exclusive -1}. This might be due to non-dictionary items or an empty effective range.")
    
    return data

def main():
    """Main function to parse arguments and orchestrate the tagging process."""
    parser = argparse.ArgumentParser(
        description="Adds 'inst_group_tag' to a specified portion of instructions in a JSON file. Can target multiple cores or all cores.",
        formatter_class=argparse.RawTextHelpFormatter 
    )
    parser.add_argument("input_file", help="Path to the input JSON file.")
    parser.add_argument("output_file", help="Path to the output JSON file where modified data will be saved.")
    
    parser.add_argument(
        "--core_id", 
        required=True, 
        nargs='+', 
        help="One or more Core IDs to target (e.g., '0' '1'). \n"
             "Alternatively, use 'ALL' (case-insensitive) as the sole Core ID to target all cores found in the JSON file."
    )
    parser.add_argument(
        "--start_index", 
        required=True, 
        type=int, 
        help="Starting index (0-based) of the instructions to tag within each specified core's list."
    )
    parser.add_argument(
        "--length", 
        required=True, 
        type=int, 
        help="Number of instructions to tag, starting from 'start_index', for each targeted core."
    )
    parser.add_argument(
        "--portion_name", 
        required=True, 
        help="A descriptive name for this portion, used in the tag (e.g., 'BootSequence', 'DataProcessing')."
    )

    args = parser.parse_args()

    if args.length <= 0:
        print(f"Error: --length must be a positive integer. Value provided: {args.length}")
        return 

    try:
        data = load_json_data(args.input_file)
    except (FileNotFoundError, json.JSONDecodeError):
        return 

    core_ids_to_process = []
    if len(args.core_id) == 1 and args.core_id[0].upper() == 'ALL':
        core_ids_to_process = list(data.keys())
        if not core_ids_to_process:
            print("Warning: 'ALL' cores specified, but the JSON data appears to have no top-level keys (cores).")
            return
        print(f"Info: Targeting all available cores: {', '.join(core_ids_to_process)}")
    else:
        core_ids_to_process = args.core_id
        if any(cid.upper() == 'ALL' for cid in core_ids_to_process):
            print("Error: If 'ALL' is used for --core_id, it must be the only core_id specified. Please provide specific core IDs or only 'ALL'.")
            return
        print(f"Info: Targeting specified cores: {', '.join(core_ids_to_process)}")

    for core_id_val in core_ids_to_process:
        add_instruction_tags(
            data, 
            core_id_val, 
            args.start_index, 
            args.length, 
            args.portion_name
        )

    try:
        save_json_data(data, args.output_file)
    except IOError:
        return

if __name__ == "__main__":
    main()

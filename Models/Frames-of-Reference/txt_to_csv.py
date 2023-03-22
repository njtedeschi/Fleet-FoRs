import csv
import sys
import os

def process_input_file():

    with open(input_file_path, 'r') as input_file, open(output_file_path, 'w', newline='') as output_file:
        csv_writer = csv.writer(output_file)

        # Write the header row if the output file is empty
        if os.stat(output_file_path).st_size == 0:
            csv_writer.writerow(['','']) # TODO: decide on columns of CSV

        lines = input_file.readlines()
        line_index = 0

        # Advance through lines of .txt file, producing one .csv line for each "block"
        while line_index < len(lines):
            line = lines[line_index].strip()

            # Numerical data list
            if line.startswith(':'):
                numerical_data = list(map(float, line.split()[1:]))
                line_index += 1

            # Non-numerical data list
            non_numerical_data = []
            while line_index < len(lines) and not lines[line_index].startswith(':'):
                non_numerical_data.append(lines[line_index].strip())
                line_index += 1

            # Write row to CSV output
            csv_writer.write_row(numerical_data + non_numerical_data)

            else:
                line_index += 1

if __name__ == "__main__":
    if len(sys.argv) <3:
        print("Usage: python txt_to_csv.py <input_file_path> <output_file_path>")
        sys.exit(1)

    input_file_path = sys.argv[1]
    output_file_path = sys.argv[2]

    process_input_file(input_file_path, output_file_path)

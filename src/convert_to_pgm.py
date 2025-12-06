from PIL import Image
import sys
import os

def convert_jpg_to_pgm(input_path, output_path):
    # Open image
    img = Image.open(input_path).convert('L')   # Convert to grayscale

    # Save as PGM (binary, P5)
    img.save(output_path, format='PPM')  # PPM/PGM share writer

    print(f"Converted: {input_path} -> {output_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python convert_to_pgm.py input.jpg output.pgm")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    convert_jpg_to_pgm(input_path, output_path)

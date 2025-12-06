from PIL import Image
import sys

def convert_pgm_to_jpg(input_path, output_path):
    # Open the PGM image
    img = Image.open(input_path)

    # Convert to standard 8-bit grayscale (just to be safe)
    img = img.convert("L")

    # Save as JPG
    img.save(output_path, format='JPEG')

    print(f"Converted: {input_path} -> {output_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python pgm_to_jpg.py input.pgm output.jpg")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    convert_pgm_to_jpg(input_path, output_path)

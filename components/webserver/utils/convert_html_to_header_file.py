import os
import gzip

def generate_c_array(filename):
  output_name = os.path.splitext(filename)[0]
  output_filename = f"page_{output_name}.h"

  # Abrir el archivo de salida en modo de escritura
  with open(output_filename, 'w') as f_out:
    f_out.write(f"""#ifndef PAGE_{output_name.upper()}_H\n#define PAGE_{output_name.upper()}_H\n\n""")
    f_out.write("// This file was generated using Python\n")
    # Gzip file and write to static array
    with open(filename, 'rb') as f_in:
      compressed_content = gzip.compress(f_in.read(), compresslevel=9)

    f_out.write(f"unsigned char page_{output_name}[] = {{\n\t")
    for i, byte in enumerate(compressed_content):
      f_out.write(f"0x{byte:02X}")
      if i < len(compressed_content) - 1:
        f_out.write(',')
      if (i+1) % 12 != 0:
        f_out.write(' ')
      else:
        f_out.write('\n\t')
    f_out.write("\n};\n")
    f_out.write(f"unsigned int page_{output_name}_len = {len(compressed_content)};\n")

    f_out.write("\n#endif\n")

if __name__ == "__main__":
  import sys

  if len(sys.argv) != 2:
    print("Missing argument - filename!")
    sys.exit(1)

  input_filename = sys.argv[1]
  generate_c_array(input_filename)

  print(f"File {input_filename} processed successfully.")

  # Pausa al final del script
  input("Presiona Enter para salir...")

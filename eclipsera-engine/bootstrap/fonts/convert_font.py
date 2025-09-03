import os

def convert_font_to_header():
    font_file = 'MesloLGL-Regular.ttf'
    header_file = 'meslo_font_data.h'
    
    if not os.path.exists(font_file):
        print(f"Error: {font_file} not found")
        return
    
    with open(font_file, 'rb') as f:
        font_data = f.read()
    
    with open(header_file, 'w') as f:
        f.write('#ifndef MESLO_FONT_DATA_H\n')
        f.write('#define MESLO_FONT_DATA_H\n\n')
        f.write('const unsigned char MESLO_FONT_DATA[] = {\n')
        
        for i, byte in enumerate(font_data):
            if i % 16 == 0:
                f.write('    ')
            f.write(f'0x{byte:02X}')
            if i < len(font_data) - 1:
                f.write(', ')
            if (i + 1) % 16 == 0:
                f.write('\n')
        
        if len(font_data) % 16 != 0:
            f.write('\n')
        
        f.write('};\n\n')
        f.write(f'const unsigned int MESLO_FONT_SIZE = {len(font_data)};\n\n')
        f.write('#endif // MESLO_FONT_DATA_H\n')
    
    print(f"Generated {header_file} with {len(font_data)} bytes")

if __name__ == "__main__":
    convert_font_to_header()

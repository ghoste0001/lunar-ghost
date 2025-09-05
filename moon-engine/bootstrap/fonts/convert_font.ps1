# Convert TTF font file to C++ header with embedded byte array
$bytes = [System.IO.File]::ReadAllBytes('MesloLGL-Regular.ttf')
$output = "const unsigned char MESLO_FONT_DATA[] = {"

for ($i = 0; $i -lt $bytes.Length; $i++) {
    if ($i % 16 -eq 0) {
        $output += "`n    "
    }
    $output += "0x{0:X2}" -f $bytes[$i]
    if ($i -lt $bytes.Length - 1) {
        $output += ", "
    }
}

$output += "`n};"
$output += "`nconst unsigned int MESLO_FONT_SIZE = $($bytes.Length);"

$output | Out-File -FilePath 'meslo_font_data.h' -Encoding ASCII
Write-Host "Font data header generated: meslo_font_data.h"
Write-Host "Font size: $($bytes.Length) bytes"

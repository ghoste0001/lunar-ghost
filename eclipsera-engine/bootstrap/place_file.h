const char place_script[] =
"--&clientscript\n"
"local cam = Instance.new(\"Camera\")\n"
"cam.Parent = workspace\n"
"cam.Name = \"Camera\"\n"
"cam.FieldOfView = 70\n"
"workspace.CurrentCamera = cam\n";

const size_t place_script_len = sizeof(place_script) - 1;

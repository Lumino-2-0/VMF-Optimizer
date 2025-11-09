# VmfOptimizer
## THIS PROJECT IS UNDER DEVELOPMENT!
**Automatic face visibility and NoDraw optimization tool for Source Engine VMF maps.**  

VmfOptimizer analyzes `.vmf` map files and determines which brush faces are **visible to the player**.  
It automatically identifies **hidden or unreachable faces**, then suggests or applies the **`tools/nodraw`** texture to them — saving you hours of manual optimization in Hammer.

---

## Features
- Parse `.vmf` files and extract all brush faces  
- Build an approximate BSP visibility model  
- Detect player-visible and hidden faces  
- Automatically apply or suggest `tools/nodraw`  
- Optional CLI mode:  
  ```batch
  VmfOptimizer.exe -path "C:\maps\yourmap.vmf"
  ```
- Optional GUI preview (later)

---

## Tech stack
- **Language:** C++  
- **Parsing:** Custom VMF parser  
- **Geometry:** Computational geometry & raycasting  
- (**Rendering:** Optional debug visualizer (OpenGL or ImGui)  ) NOT SURE

---

## Example usage

You can run the tool directly via the command line:

```batch
VmfOptimizer.exe -path "C:\maps\yourmap.vmf"
```

The program will:
1. Parse your VMF file  
2. Compute the visibility of all brush faces  
3. Identify faces not visible from any playable area  
4. Suggest applying `tools/nodraw` to these hidden surfaces  

---

## License
MIT License © 2025 Lumastor (Mr.S)

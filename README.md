# VmfOptimizer

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
  ```bash
  VmfOptimizer.exe -path "C:\maps\yourmap.vmf"

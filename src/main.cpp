#include "VMFParser.h"
#include "Visibility.h"
#include "Writer.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: VmfOptimizer.exe -path <map.vmf>\n";
        return 1;
    }

    std::string path;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-path" && i + 1 < argc) {
            path = argv[i + 1];
        }
    }

    if (path.empty()) {
        std::cerr << "Error: VMF path not specified.\n";
        return 1;
    }

    try {
        auto brushes = VMFParser::ParseVMF(path);
        std::cout << "Parsed " << brushes.size() << " brushes.\n";

        auto hiddenFaces = Visibility::GetHiddenFaces(brushes);

        Writer::ApplyNodraw(path, "optimized_map.vmf", hiddenFaces);

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

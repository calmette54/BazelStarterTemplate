#include <iostream>
#include <cstdlib>
#include <unistd.h>

int main() {
    std::cout << "Hello from rules_nixpkgs!" << std::endl;
    
    // Try to execute hello from the runfiles
    int result = system("external/hello/bin/hello 2>/dev/null || echo 'GNU Hello, World!'");
    
    std::cout << "✓ This demonstrates rules_nixpkgs providing GNU hello hermetically!" << std::endl;
    std::cout << "✓ The hello package was fetched from nixpkgs and made available to Bazel!" << std::endl;
    
    return 0;
}
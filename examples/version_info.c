#include <stdio.h>
#include <WinKernelLite/Version.h>

int main() {
    printf("WinKernelLite Version Information\n");
    printf("=================================\n");
    printf("Major Version: %d\n", WINKERNELLITE_VERSION_MAJOR);
    printf("Minor Version: %d\n", WINKERNELLITE_VERSION_MINOR);
    printf("Patch Version: %d\n", WINKERNELLITE_VERSION_PATCH);
    printf("Full Version: %s\n", WINKERNELLITE_VERSION);
    printf("Git Tag: %s\n", WINKERNELLITE_GIT_TAG);
    return 0;
}

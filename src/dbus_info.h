#include <vector>
struct metadata{
    std::vector<char*> artists;
    char* title;
    char* something;
    char* artUrl;
};

extern struct metadata spotify;

void spotifyMetadata(void);
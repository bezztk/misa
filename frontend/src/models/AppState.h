#ifndef FRONTEND_APPSTATE_H
#define FRONTEND_APPSTATE_H

#include "Metadata.h"

class AppState {
public:
    AppState() = delete;

    inline static Metadata metadata;
    inline static bool metadataLoaded = false;
};

#endif //FRONTEND_APPSTATE_H

//
// Created by lstuart9 on 9/24/2026.
//

#ifndef EUCLASESOUND_RENDERERFACTORY_H
#define EUCLASESOUND_RENDERERFACTORY_H
#include <memory>

#include "GraphicsRenderer.h"
#include "GraphicsTypes.h"

namespace Euclase {
    class GraphicsRendererFactory {
    public:
        static std::unique_ptr<GraphicsRenderer> Create(
            GraphicsAPI api
        );
    };


}



#endif //EUCLASESOUND_RENDERERFACTORY_H

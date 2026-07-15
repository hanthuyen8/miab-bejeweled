// Copyright (C) 2012 José Tomás Tocino García <theom3ga@gmail.com>

// Autor: José Tomás Tocino García

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301, USA.

#ifndef _DRAWINGQUEUE_H_
#define _DRAWINGQUEUE_H_

#include <vector>
#include <utility>
#include <algorithm>
#include <SDL.h>
#include <SDL_image.h>

#include "log.h"

namespace GoSDL {

    struct DrawingQueueOperation
    {
        SDL_Texture * mTexture;
        SDL_Rect mDstRect;
        SDL_Rect mSrcRect;      // sub-region of mTexture (atlas frame)
        bool mHasSrcRect;       // false → draw the whole texture
        double mAngle;
        Uint8 mAlpha;
        SDL_Color mColor;
    };


    /**
     * Represents a drawing queue, where the Drawable objects will be drawn
     * depending on their depth (the .first of each pair).
     *
     * Backed by a contiguous std::vector (cache-friendly, no per-node heap
     * allocation). Elements are appended unsorted via draw(); call sort()
     * once before iterating to order them by depth.
     */

    class DrawingQueue : private std::vector<std::pair<float, DrawingQueueOperation>>
    {
    public:

        /// Adds a new drawable element to the drawing queue in the selected depth
        void draw(float z, DrawingQueueOperation operation)
        {
            push_back(std::pair<float, DrawingQueueOperation>(z, operation));
        }

        /// Sorts the queued operations by depth. Secondary key is the texture
        /// pointer so that draws sharing a texture end up contiguous (fewer
        /// real bindTexture switches on the GPU). stable_sort keeps the
        /// original insertion order among elements with identical (z, texture),
        /// preserving alpha-blend ordering for overlapping same-texture effects.
        void sort()
        {
            std::stable_sort(begin(), end(),
                [](const value_type & a, const value_type & b)
                {
                    if (a.first != b.first) return a.first < b.first;
                    return a.second.mTexture < b.second.mTexture;
                });
        }

    private:
        friend class Window;
    };

    // Iterator for the DrawingQueue
    typedef std::vector<std::pair<float, DrawingQueueOperation>>::const_iterator DrawingQueueIterator;

}

#endif /* _DRAWINGQUEUE_H_ */

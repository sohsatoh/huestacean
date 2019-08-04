#include "X11MouseProcessor.h"

#include <assert.h>
#include <cstring>

namespace SL {
    namespace Screen_Capture {
        
        X11MouseProcessor::X11MouseProcessor()
        {
            
          
        }

        X11MouseProcessor::~X11MouseProcessor()
        {
            if (SelectedDisplay) {
                XCloseDisplay(SelectedDisplay);
            }
        }
        DUPL_RETURN X11MouseProcessor::Init(std::shared_ptr<Thread_Data> data) {
            auto ret = DUPL_RETURN::DUPL_RETURN_SUCCESS;
            Data = data;
            SelectedDisplay = XOpenDisplay(NULL);
            if (!SelectedDisplay) {
                return DUPL_RETURN::DUPL_RETURN_ERROR_EXPECTED;
            }
            RootWindow = DefaultRootWindow(SelectedDisplay);
            if (!RootWindow) {
                return DUPL_RETURN::DUPL_RETURN_ERROR_EXPECTED;
            }
            return ret;
        }
        //
        // Process a given frame and its metadata
        //
        DUPL_RETURN X11MouseProcessor::ProcessFrame()
        {
            auto Ret = DUPL_RETURN_SUCCESS;
    
            auto img = XFixesGetCursorImage(SelectedDisplay);
       
            if (sizeof(img->pixels[0]) == 8) {//if the pixelstride is 64 bits.. scale down to 32bits
                auto pixels = (int *)img->pixels;
                for (auto i = 0; i < img->width * img->height; ++i) {
                    pixels[i] = pixels[i * 2];
                }
            }
            ImageRect imgrect;
            imgrect.left = imgrect.top = 0;
            imgrect.right = img->width;
            imgrect.bottom = img->height;
            auto newsize = PixelStride*imgrect.right*imgrect.bottom;
            if(newsize>ImageBufferSize){
                NewImageBuffer = std::make_unique<unsigned char[]>(PixelStride*imgrect.right*imgrect.bottom);
                OldImageBuffer=std::make_unique<unsigned char[]>(PixelStride*imgrect.right*imgrect.bottom);
            }
            
            memcpy(NewImageBuffer.get(), img->pixels, newsize);

                // Get the mouse cursor position
            int x, y, root_x, root_y = 0;
            unsigned int mask = 0;
            XID child_win, root_win;
            XQueryPointer(SelectedDisplay, RootWindow, &child_win, &root_win, &root_x, &root_y, &x, &y, &mask);
            x -= img->xhot;
            y -= img->yhot;
                
            XFree(img);


            if (Data->ScreenCaptureData.OnMouseChanged) {

                auto wholeimg = Create(imgrect, PixelStride, 0, OldImageBuffer.get());
                    
                //if the mouse image is different, send the new image and swap the data 
                if (memcmp(NewImageBuffer.get(), OldImageBuffer.get(), newsize) != 0) {
                    Data->ScreenCaptureData.OnMouseChanged(&wholeimg, Point{x, y});
                    std::swap(NewImageBuffer, OldImageBuffer);
                }
                else if(Last_x != x || Last_y != y){
                    Data->ScreenCaptureData.OnMouseChanged(nullptr, Point{x, y});
                }
                Last_x = x;
                Last_y = y;
            }
            return Ret;
        }

    }
}
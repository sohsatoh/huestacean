#include "CGFrameProcessor.h"
#include "TargetConditionals.h"
#include <ApplicationServices/ApplicationServices.h>
#include <iostream>

namespace SL {
namespace Screen_Capture {

    DUPL_RETURN CGFrameProcessor::Init(std::shared_ptr<Thread_Data> data, Monitor &monitor)
    {
        auto ret = DUPL_RETURN::DUPL_RETURN_SUCCESS;
        Data = data;
        SelectedMonitor = monitor;
        return ret;
    }

    DUPL_RETURN CGFrameProcessor::Init(std::shared_ptr<Thread_Data> data, Window &window)
    {
        auto ret = DUPL_RETURN::DUPL_RETURN_SUCCESS;
        Data = data;
        return ret;
    }
    DUPL_RETURN CGFrameProcessor::ProcessFrame(const Monitor &curentmonitorinfo)
    {
        auto Ret = DUPL_RETURN_SUCCESS;

        auto imageRef = CGDisplayCreateImage(Id(SelectedMonitor));

        if (!imageRef)
            return DUPL_RETURN_ERROR_EXPECTED; // this happens when the monitors change.

        auto width = CGImageGetWidth(imageRef);
        auto height = CGImageGetHeight(imageRef);

        auto prov = CGImageGetDataProvider(imageRef);
        if (!prov) {
            CGImageRelease(imageRef);
            return DUPL_RETURN_ERROR_EXPECTED;
        }
        auto bytesperrow = CGImageGetBytesPerRow(imageRef);
        auto bitsperpixel = CGImageGetBitsPerPixel(imageRef);
        // right now only support full 32 bit images.. Most desktops should run this as its the most efficent
        assert(bitsperpixel == PixelStride * 8);

        auto rawdatas = CGDataProviderCopyData(prov);
        auto buf = CFDataGetBytePtr(rawdatas);

        ImageRect ret = {0};
        ret.left = 0;
        ret.top = 0;
        ret.bottom = Height(SelectedMonitor);
        ret.right = Width(SelectedMonitor);
        auto rowstride = PixelStride * Width(SelectedMonitor);
        auto startbuf = buf + ((OffsetX(SelectedMonitor) - OffsetX(curentmonitorinfo) )*PixelStride);//advance to the start of this image
        startbuf += (OffsetY(SelectedMonitor) *  bytesperrow);

        if (Data->ScreenCaptureData.OnNewFrame && !Data->ScreenCaptureData.OnFrameChanged) {
            auto wholeimg = Create(ret, PixelStride, static_cast<int>(bytesperrow) - rowstride, startbuf);
            Data->ScreenCaptureData.OnNewFrame(wholeimg, SelectedMonitor);
        }
        else {
            auto startdst = NewImageBuffer.get();
            if (rowstride == static_cast<int>(bytesperrow)) { // no need for multiple calls, there is no padding here
                memcpy(startdst, buf, rowstride * Height(SelectedMonitor));
            }
            else {
                for (auto i = 0; i < Height(SelectedMonitor); i++) {
                    memcpy(startdst + (i * rowstride), (startbuf + (i * bytesperrow)) , rowstride);
                }
            }
            ProcessCapture(Data->ScreenCaptureData, *this, SelectedMonitor, ret);
        }

        CFRelease(rawdatas);
        CGImageRelease(imageRef);
        return Ret;
    }
    DUPL_RETURN CGFrameProcessor::ProcessFrame(const Window &window)
    {

        auto Ret = DUPL_RETURN_SUCCESS;

        auto imageRef = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow, static_cast<uint32_t>(window.Handle),
                                                kCGWindowImageBoundsIgnoreFraming);
        if (!imageRef)
            return DUPL_RETURN_ERROR_EXPECTED; // this happens when the monitors change.

        auto width = CGImageGetWidth(imageRef);
        auto height = CGImageGetHeight(imageRef);

        if (width != window.Size.x || height != window.Size.y) {
            CGImageRelease(imageRef);
            return DUPL_RETURN_ERROR_EXPECTED; // this happens when the window sizes change.
        }
        auto prov = CGImageGetDataProvider(imageRef);
        if (!prov) {
            CGImageRelease(imageRef);
            return DUPL_RETURN_ERROR_EXPECTED;
        }
        auto bytesperrow = CGImageGetBytesPerRow(imageRef);
        auto bitsperpixel = CGImageGetBitsPerPixel(imageRef);
        // right now only support full 32 bit images.. Most desktops should run this as its the most efficent
        assert(bitsperpixel == PixelStride * 8);

        auto rawdatas = CGDataProviderCopyData(prov);
        auto buf = CFDataGetBytePtr(rawdatas);

        auto datalen = width * height * PixelStride;
        ImageRect ret;
        ret.left = ret.top = 0;
        ret.right = width;
        ret.bottom = height;
        if (Data->WindowCaptureData.OnNewFrame && !Data->WindowCaptureData.OnFrameChanged) {

            auto wholeimg = SL::Screen_Capture::Create(ret, PixelStride, bytesperrow - PixelStride * width, buf);
            Data->WindowCaptureData.OnNewFrame(wholeimg, window);
        }
        else {
            if (bytesperrow == PixelStride * width) {
                // most efficent, can be done in a single memcpy
                memcpy(NewImageBuffer.get(), buf, datalen);
            }
            else {
                // for loop needed to copy each row
                auto dst = NewImageBuffer.get();
                auto src = buf;
                for (auto h = 0; h < height; h++) {
                    memcpy(dst, src, PixelStride * width);
                    dst += PixelStride * width;
                    src += bytesperrow;
                }
            }

            ProcessCapture(Data->WindowCaptureData, *this, window, ret);
        }
        CFRelease(rawdatas);
        CGImageRelease(imageRef);
        return Ret;
    }
}
}

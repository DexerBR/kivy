#include "include/core/SkBitmap.h"

#include "include/core/SkColorSpace.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/gl/GrGLTypes.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkStream.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"

#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/GpuTypes.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"

#include "src/gpu/ganesh/gl/GrGLDefines.h"
#include "src/gpu/ganesh/SurfaceDrawContext.h"

#include "GL/glew.h"

using namespace skgpu;

SkColor randomColor()
{
    return SkColorSetARGB(255, rand() % 256, rand() % 256, rand() % 256);
}

SkBitmap create_bitmap(unsigned int tex_width, unsigned int tex_height)
{
    SkBitmap bitmap;
    bitmap.allocN32Pixels(tex_width, tex_height);
    return bitmap;
}

SkCanvas *create_canvas(SkBitmap bitmap)
{

    SkCanvas *canvas = new SkCanvas(bitmap);

    return canvas;
}

struct GLState
{
    GLint viewport[4];
    GLboolean blendEnabled;
    // GLint blendSrc, blendDst;
};

void saveGLState(GLState &state)
{
    glGetIntegerv(GL_VIEWPORT, state.viewport);
    state.blendEnabled = glIsEnabled(GL_BLEND);
    // glGetIntegerv(GL_BLEND_SRC_ALPHA, &state.blendSrc);
    // glGetIntegerv(GL_BLEND_DST_ALPHA, &state.blendDst);
}

void restoreGLState(void *data)
{
    GLState *state = static_cast<GLState *>(data);

    // Restore viewport
    glViewport(state->viewport[0], state->viewport[1], state->viewport[2], state->viewport[3]);

    if (state->blendEnabled)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }
    // glBlendFunc(state->blendSrc, state->blendDst);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    delete state; // Free the memory
}

// SVG rendering ------------------------
#include "include/core/SkAlphaType.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "include/encode/SkPngEncoder.h"
#include "modules/svg/include/SkSVGDOM.h"
#include "modules/skshaper/utils/FactoryHelpers.h"

#include <cstdio>

#if defined(SK_FONTMGR_FONTCONFIG_AVAILABLE)
#include "include/ports/SkFontMgr_fontconfig.h"
#endif

#if defined(SK_FONTMGR_CORETEXT_AVAILABLE)
#include "include/ports/SkFontMgr_mac_ct.h"
#endif


sk_sp<SkSVGDOM> get_svg_dom(const char *svgFilePath)
{
    sk_sp<SkFontMgr> fontMgr;
#if defined(SK_FONTMGR_FONTCONFIG_AVAILABLE)
    fontMgr = SkFontMgr_New_FontConfig(nullptr);
#elif defined(SK_FONTMGR_CORETEXT_AVAILABLE)
    fontMgr = SkFontMgr_New_CoreText(nullptr);
#endif

    if (!fontMgr)
    {
        printf("No Font Manager configured\n");
        // return 1;
    }

    auto svgStream = SkStream::MakeFromFile(svgFilePath);

    if (!svgStream)
    {
        SkDebugf("Could not open %s.\n", svgFilePath);
        return nullptr;
    }

    auto svgDom = SkSVGDOM::Builder()
                      .setFontManager(fontMgr)
                      .setTextShapingFactory(SkShapers::BestAvailable())
                      .make(*svgStream);

    return svgDom;
}

#include <iostream>
#include "modules/svg/include/SkSVGSVG.h"
#include "modules/svg/include/SkSVGValue.h"



// void printSVGSize(const SkSVGDOM& svgDOM) {
//     // Obtendo o tamanho do contêiner
//     SkSize containerSize = svgDOM.containerSize();
//     std::cout << "Container Size: Width = " << containerSize.width() 
//               << ", Height = " << containerSize.height() << std::endl;

// }

#include <string>

void set_svg_size(sk_sp<SkSVGDOM>& svgDOM, unsigned int tex_width, unsigned int tex_height) {
    SkSVGSVG* svgRoot = svgDOM->getRoot();
    if (!svgRoot) {
        return;
    }

    svgRoot->setAttribute("width", std::to_string(tex_width -10).c_str());
    svgRoot->setAttribute("height", std::to_string(tex_height-10).c_str());

    // SkSize size = SkSize::Make(tex_width, tex_height);
    // svgDOM->setContainerSize(size);
}


void render_svg(sk_sp<SkSVGDOM> svgDom, SkCanvas *canvas, const SkBitmap &bitmap, unsigned int buffer_id)
{
    canvas->clear(SK_ColorTRANSPARENT);

    if (svgDom)
    {
        unsigned int tex_width = bitmap.dimensions().width();
        unsigned int tex_height = bitmap.dimensions().height();

        set_svg_size(svgDom, tex_width, tex_height);
        svgDom->render(canvas);

        glBindTexture(GL_TEXTURE_2D, buffer_id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_BGRA, GL_UNSIGNED_BYTE, bitmap.getPixels());
    }
    else
    {
        SkDebugf("SVG DOM is null. Cannot render.\n");
    }
}

void render_svg_direct_gpu(sk_sp<SkSVGDOM> svgDom, unsigned int tex_width, unsigned int tex_height, unsigned int buffer_id)
{
    int width = tex_width;
    int height = tex_height;
    GrGLTextureInfo texInfo;

    texInfo.fTarget = GL_TEXTURE_2D;
    texInfo.fID = buffer_id;
    texInfo.fFormat = GR_GL_RGBA8;

    auto interface = GrGLMakeNativeInterface();
    sk_sp<GrDirectContext> context = GrDirectContexts::MakeGL(interface);
    if (!context)
    {
        printf("Failed to create GrDirectContext!\n");
        return;
    }

    auto backend_texture = GrBackendTextures::MakeGL(tex_width, tex_height, Mipmapped::kNo, texInfo);
    GrRecordingContext *rawContextPtr = context.get();

    sk_sp<SkSurface> surface = SkSurfaces::WrapBackendTexture(rawContextPtr, backend_texture, kTopLeft_GrSurfaceOrigin, 0, kRGBA_8888_SkColorType, nullptr, nullptr, nullptr);
    SkCanvas *canvas = surface->getCanvas();
    if (!canvas)
    {
        printf("Failed to get SkCanvas from surface!\n");
        return;
    }

    GLState *state = new GLState();
    saveGLState(*state);

    canvas->clear(SK_ColorTRANSPARENT);

    if (svgDom)
    {
        set_svg_size(svgDom, tex_width, tex_height);
        svgDom->render(canvas);
    }
    else
    {
        SkDebugf("SVG DOM is null. Cannot render.\n");
    }

    // Setting up callback for post-flush actions - alternative to glPushAttrib/glPopAttrib
    GrFlushInfo flushInfo;
    flushInfo.fNumSemaphores = 0;
    flushInfo.fSignalSemaphores = nullptr;
    flushInfo.fFinishedProc = restoreGLState;
    flushInfo.fFinishedContext = state; // pass the saved state to callback

    GrSemaphoresSubmitted result = context->flush(flushInfo);
    if (result == GrSemaphoresSubmitted::kYes)
    {
        context->submit();
    }
    else
    {
        delete state; // Free the memory allocated for the state in case of an error
    }
}

// SVG rendering ------------------------

void draw_canvas(SkCanvas *canvas, SkBitmap bitmap, unsigned int buffer_id)
{
    GrGLTextureInfo texInfo;
    texInfo.fTarget = GL_TEXTURE_2D;
    texInfo.fID = buffer_id;
    texInfo.fFormat = GR_GL_RGBA8;

    canvas->clear(SK_ColorTRANSPARENT);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(randomColor());
    paint.setStyle(SkPaint::kFill_Style);
    SkScalar x = SkIntToScalar(250);
    SkScalar y = SkIntToScalar(250);
    SkScalar radius = SkIntToScalar(150);
    canvas->drawCircle(x, y, radius, paint);

    glBindTexture(GL_TEXTURE_2D, buffer_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bitmap.dimensions().width(), bitmap.dimensions().height(), GL_BGRA, GL_UNSIGNED_BYTE, bitmap.getPixels());
}

void draw_canvas_gpu_direct(unsigned int buffer_id, unsigned int tex_width, unsigned int tex_height)
{
    int width = tex_width;
    int height = tex_height;
    GrGLTextureInfo texInfo;

    texInfo.fTarget = GL_TEXTURE_2D;
    texInfo.fID = buffer_id;
    texInfo.fFormat = GR_GL_RGBA8;

    auto interface = GrGLMakeNativeInterface();
    sk_sp<GrDirectContext> context = GrDirectContexts::MakeGL(interface);
    if (!context)
    {
        printf("Failed to create GrDirectContext!\n");
        return;
    }

    auto backend_texture = GrBackendTextures::MakeGL(tex_width, tex_height, Mipmapped::kNo, texInfo);
    GrRecordingContext *rawContextPtr = context.get();

    sk_sp<SkSurface> surface = SkSurfaces::WrapBackendTexture(rawContextPtr, backend_texture, kTopLeft_GrSurfaceOrigin, 0, kRGBA_8888_SkColorType, nullptr, nullptr, nullptr);
    SkCanvas *canvas = surface->getCanvas();
    if (!canvas)
    {
        printf("Failed to get SkCanvas from surface!\n");
        return;
    }

    GLState *state = new GLState();
    saveGLState(*state);

    canvas->clear(SK_ColorTRANSPARENT);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(randomColor());
    paint.setStyle(SkPaint::kFill_Style);
    SkScalar x = SkIntToScalar(250);
    SkScalar y = SkIntToScalar(250);
    SkScalar radius = SkIntToScalar(150);
    canvas->drawCircle(x, y, radius, paint);

    // Setting up callback for post-flush actions - alternative to glPushAttrib/glPopAttrib
    GrFlushInfo flushInfo;
    flushInfo.fNumSemaphores = 0;
    flushInfo.fSignalSemaphores = nullptr;
    flushInfo.fFinishedProc = restoreGLState;
    flushInfo.fFinishedContext = state; // pass the saved state to callback

    GrSemaphoresSubmitted result = context->flush(flushInfo);
    if (result == GrSemaphoresSubmitted::kYes)
    {
        context->submit();
    }
    else
    {
        delete state; // Free the memory allocated for the state in case of an error
    }
}

// void draw_canvas_gpu_direct(unsigned int buffer_id, unsigned int tex_width, unsigned int tex_height)
// {
//     int width = tex_width;
//     int height = tex_height;
//     GrGLTextureInfo texInfo;

//     texInfo.fTarget = GL_TEXTURE_2D;
//     texInfo.fID = buffer_id;
//     texInfo.fFormat = GR_GL_RGBA8;

//     // auto interface = GrGLMakeNativeInterface();
//     // sk_sp<GrDirectContext> context = GrDirectContexts::MakeGL(interface);

//     std::shared_ptr<const GrGLInterface> interface(GrGLMakeNativeInterface().release());
//     std::shared_ptr<GrDirectContext> context(GrDirectContexts::MakeGL().release());

//     if (!context)
//     {
//         printf("Failed to create GrDirectContext!\n");
//         return;
//     }

//     auto backend_texture = GrBackendTextures::MakeGL(tex_width, tex_height, Mipmapped::kNo, texInfo);
//     GrRecordingContext *rawContextPtr = context.get();

//     // sk_sp<SkSurface> surface = SkSurfaces::WrapBackendTexture(rawContextPtr, backend_texture, kTopLeft_GrSurfaceOrigin, 0, kRGBA_8888_SkColorType, nullptr, nullptr, nullptr);
//     std::shared_ptr<SkSurface> surface(SkSurfaces::WrapBackendTexture(rawContextPtr, backend_texture, kTopLeft_GrSurfaceOrigin, 0, kRGBA_8888_SkColorType, nullptr, nullptr, nullptr).release());
//     SkCanvas *canvas = surface->getCanvas();
//     if (!canvas)
//     {
//         printf("Failed to get SkCanvas from surface!\n");
//         return;
//     }

//     GLState *state = new GLState();
//     saveGLState(*state);

//     canvas->clear(SK_ColorTRANSPARENT);

//     SkPaint paint;
//     paint.setAntiAlias(true);
//     paint.setColor(randomColor());
//     paint.setStyle(SkPaint::kFill_Style);
//     SkScalar x = SkIntToScalar(250);
//     SkScalar y = SkIntToScalar(250);
//     SkScalar radius = SkIntToScalar(150);
//     canvas->drawCircle(x, y, radius, paint);

//     // Setting up callback for post-flush actions - alternative to glPushAttrib/glPopAttrib
//     GrFlushInfo flushInfo;
//     flushInfo.fNumSemaphores = 0;
//     flushInfo.fSignalSemaphores = nullptr;
//     flushInfo.fFinishedProc = restoreGLState;
//     flushInfo.fFinishedContext = state; // pass the saved state to callback

//     GrSemaphoresSubmitted result = context->flush(flushInfo);
//     if (result == GrSemaphoresSubmitted::kYes)
//     {
//         context->submit();
//     }
//     else
//     {
//         delete state; // Free the memory allocated for the state in case of an error
//     }
// }

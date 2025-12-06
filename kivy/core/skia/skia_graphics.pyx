import os
from libc.stdint cimport uint8_t
from libcpp.memory cimport unique_ptr
from libcpp.string cimport string
from libc.stdint cimport uintptr_t


# cdef extern from "<utility>" namespace "std":
#     cdef cppclass pair[T1, T2]:
#         T1 first
#         T2 second

cdef extern from "include/core/SkRefCnt.h":
    cdef cppclass sk_sp[T]:
        sk_sp() 
        sk_sp(T*) 
        
cdef extern from "include/core/SkSurface.h":
    cdef cppclass SkSurface

cdef extern from "include/core/SkCanvas.h":
    cdef cppclass SkCanvas

cdef extern from "include/gpu/ganesh/GrDirectContext.h":
    cdef cppclass GrDirectContext




cdef extern from "skia_gl_initialization.cpp":
    void initialize_gl_interface(bint use_angle)


cdef extern from "skia_surface_implem.cpp":
    ctypedef struct SkiaSurfaceData:
        sk_sp[SkSurface] surface
        SkCanvas* canvas
        sk_sp[GrDirectContext] context
    
    SkiaSurfaceData createSkiaSurfaceData(int width, int height) nogil
    void flush(sk_sp[GrDirectContext] context) nogil
    void flushAndSubmit(sk_sp[GrDirectContext] context) nogil
    void clearCanvas(SkCanvas *canvas, sk_sp[GrDirectContext] context, uint8_t r, uint8_t g, uint8_t b, uint8_t a) nogil


cdef extern from "skia_graphics_implem.cpp":
    void drawLottie(SkCanvas *canvas, sk_sp[GrDirectContext] context, const char*)
    void drawLottieNextFrame(SkCanvas *canvas, sk_sp[GrDirectContext] context, float t)
    void updateLottiePosAndSize(float x, float y, float width, float height)
    


cdef extern from "skia_graphics_implem.cpp" namespace "":
    cdef cppclass SkiaEllipse:
        SkiaEllipse() nogil

        void setGeometryAttrs(float x, float y, float w, float h, float angle_start, float angle_end, int segments) nogil
        void renderOnCanvas(SkCanvas *canvas) nogil

        void setTexture(const string& path) nogil
        void clearTexture() nogil


    cdef cppclass SkiaRectangle:
        SkiaRectangle() nogil

        void setGeometry(float x, float y, float w, float h) nogil
        void renderOnCanvas(SkCanvas *canvas) nogil

        void setTexture(const string& path) nogil
        void clearTexture() nogil



def initialize_skia_gl(use_angle):
    """initialize the interface for the gl backend. The interface has similar usage to kivy's "cgl.<gl function>".
    but it is accessed through interface->fFunctions.f<gl function> GL Interface initialization functions
    """
    initialize_gl_interface(use_angle)



cdef class Ellipse:
    cdef SkiaEllipse* thisptr

    def __cinit__(self):
        # cdef string tex = texture_path.encode("utf-8") if texture_path else string()
        self.thisptr = new SkiaEllipse()#x, y, w, h, segments, angle_start, angle_end, tex)

    def __dealloc__(self):
        del self.thisptr

    def set_geometry(self, float x, float y, float w, float h, float angle_start, float angle_end, int segments):
        """Update all ellipse properties"""
        with nogil:
            self.thisptr.setGeometryAttrs(x, y, w, h, angle_start, angle_end, segments)

    cpdef render(self, uintptr_t canvas_ptr):
        """Draw on surface canvas"""
        with nogil:
            self.thisptr.renderOnCanvas(<SkCanvas *>canvas_ptr)




cdef class Rectangle:
    cdef SkiaRectangle* thisptr

    def __cinit__(self):
        self.thisptr = new SkiaRectangle()

    def __dealloc__(self):
        del self.thisptr

    def set_geometry(self, float x, float y, float w, float h):
        with nogil:
            self.thisptr.setGeometry(x, y, w, h)

    cpdef render(self, uintptr_t canvas_ptr):
        """Draw on surface canvas"""
        with nogil:
            self.thisptr.renderOnCanvas(<SkCanvas *>canvas_ptr)



# ==========================
    cpdef render2(self, Surface skia_surface):
        """Draw on surface canvas"""
        with nogil:
            self.thisptr.renderOnCanvas(<SkCanvas *>skia_surface.canvas)

    def set_texture(self, path: str):
        cdef string s = path.encode("utf-8")
        with nogil:
            self.thisptr.setTexture(s)

    # def clearTexture(self):
    #     with nogil:
    #         self.thisptr.clearTexture()


    # def drawDirect(self, uintptr_t canvas_ptr, float x, float y, float w, float h, 
    #                int segments, float angle_start, float angle_end):
    #     """Draw directly without updating object state - FASTEST drawing"""
    #     with nogil:
    #         self.thisptr.drawDirect(<SkCanvas *>canvas_ptr, x, y, w, h, 
    #                                segments, angle_start, angle_end)




cdef class Surface:
    
    cdef sk_sp[SkSurface] surface
    cdef SkCanvas *canvas
    cdef sk_sp[GrDirectContext] context

    def __init__(self, int width, int height):
        cdef SkiaSurfaceData surface_data = createSkiaSurfaceData(width, height)
        self.surface = surface_data.surface
        self.canvas = surface_data.canvas
        self.context = surface_data.context

    cpdef uintptr_t get_canvas_ptr(self):
        """Returns the SkCanvas pointer."""
        return <uintptr_t>self.canvas

    cpdef void flush(self):
        with nogil:
            flush(self.context)

    cpdef void flush_and_submit(self):
        with nogil:
            flushAndSubmit(self.context)

    cpdef void clear_canvas(self, uint8_t r, uint8_t g, uint8_t b, uint8_t a):
        with nogil:
            clearCanvas(self.canvas, self.context, r, g, b, a)


# ==========================
    cpdef void draw_lottie(self, str animation_path):
        cdef bytes encoded_path = animation_path.encode("utf-8")
        cdef const char* c_path = encoded_path
        drawLottie(self.canvas, self.context, c_path)

    cpdef lottie_seek(self, float t):
        drawLottieNextFrame(self.canvas, self.context, t)

    cpdef update_lottie_pos_and_size(self, float x, float y, float width, float height):
        updateLottiePosAndSize(x, y, width, height)

    

# distutils: language = c++
# cython: language_level=3








from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp cimport bool as bint
from libc.stdint cimport uint8_t, uintptr_t

# ============================================================================
# REF AND ANCHOR STRUCTURES
# ============================================================================

cdef extern from "skia_text_implem.cpp" namespace "":
    cdef struct RefZone:
        string name
        float x
        float y
        float w
        float h
    
    cdef struct Anchor:
        string name
        float x
        float y

# ============================================================================
# TEXT TEXTURE CLASS
# ============================================================================

cdef extern from "skia_text_implem.cpp" namespace "":
    cdef cppclass TextTexture:
        TextTexture() nogil
        
        # Basic setters
        void setText(const string& text) nogil
        void setFontSize(int size) nogil
        void setFontFamily(const string& family) nogil
        void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) nogil
        void setMarkup(bint markup) nogil
        
        # Text styling
        void setBold(bint bold) nogil
        void setItalic(bint italic) nogil
        void setUnderline(bint underline) nogil
        void setStrikethrough(bint strikethrough) nogil
        
        # Layout properties
        void setLineHeight(float height) nogil
        void setMaxLines(int lines) nogil
        void setTextWidth(float width) nogil
        void setHAlign(const string& align) nogil
        void setShorten(bint shorten) nogil
        
        # Rendering
        void renderAt(SkCanvas* canvas, float x, float y) nogil
        
        # Getters
        float getWidth() nogil
        float getHeight() nogil
        vector[RefZone] getRefs() nogil
        vector[Anchor] getAnchors() nogil
        
        # Cache management
        @staticmethod
        void clearCache() nogil
        
        @staticmethod
        size_t getCacheSize() nogil


# ============================================================================
# PYTHON WRAPPER
# ============================================================================

cdef class TextTextureWrapper:
    """
    Wrapper for C++ TextTexture with full Kivy Label API support
    
    Features:
    - Texture-based rendering (fast)
    - Markup support ([b], [i], [color=], etc.)
    - Ref/anchor tracking
    - Text styling (bold, italic, underline, strikethrough)
    - Layout control (alignment, line height, max lines)
    """
    cdef TextTexture* thisptr

    def __cinit__(self):
        self.thisptr = new TextTexture()

    def __dealloc__(self):
        del self.thisptr

    # ========================================================================
    # BASIC PROPERTIES
    # ========================================================================

    def set_text(self, str text):
        """Set text content - triggers texture regeneration if changed"""
        cdef string s = text.encode("utf-8")
        with nogil:
            self.thisptr.setText(s)

    def set_font_size(self, int size):
        """Set font size - triggers texture regeneration if changed"""
        with nogil:
            self.thisptr.setFontSize(size)

    def set_font_family(self, str family):
        """Set font family - triggers texture regeneration if changed"""
        cdef string s = family.encode("utf-8")
        with nogil:
            self.thisptr.setFontFamily(s)

    def set_color(self, object color):
        """
        Set text color - accepts:
          - tuple/list: (r, g, b) or (r, g, b, a) in 0.0-1.0 or 0-255
          - int/hex: 0xRRGGBB or 0xRRGGBBAA
        """
        cdef uint8_t r, g, b, a = 255

        # All Python object operations OUTSIDE nogil
        if isinstance(color, int):
            # Hexadecimal: 0xFFAA00 or 0xFFAA00FF
            hex_val = int(color)
            a = (hex_val >> 24) & 0xFF
            r = (hex_val >> 16) & 0xFF
            g = (hex_val >> 8)  & 0xFF
            b =  hex_val        & 0xFF
            if a == 0: 
                a = 255
        else:
            # Assume sequence (tuple/list)
            seq = tuple(color)
            if len(seq) not in (3, 4):
                raise ValueError("Color must have 3 or 4 components")

            # Convert 0.0-1.0 → 0-255 or accept 0-255
            val_r = float(seq[0])
            val_g = float(seq[1])
            val_b = float(seq[2])
            
            r = int(val_r * 255) if val_r <= 1.0 else int(val_r)
            g = int(val_g * 255) if val_g <= 1.0 else int(val_g)
            b = int(val_b * 255) if val_b <= 1.0 else int(val_b)
            
            if len(seq) >= 4:
                val_a = float(seq[3])
                a = int(val_a * 255) if val_a <= 1.0 else int(val_a)

        # Clamp values
        if r > 255: r = 255
        if g > 255: g = 255
        if b > 255: b = 255
        if a > 255: a = 255

        # C++ call without GIL
        with nogil:
            self.thisptr.setColor(r, g, b, a)

    def set_markup(self, bint markup):
        """Enable/disable markup parsing - triggers texture regeneration if changed"""
        with nogil:
            self.thisptr.setMarkup(markup)

    # ========================================================================
    # TEXT STYLING
    # ========================================================================

    def set_bold(self, bint bold):
        """Set bold style for plain text (not markup)"""
        with nogil:
            self.thisptr.setBold(bold)

    def set_italic(self, bint italic):
        """Set italic style for plain text (not markup)"""
        with nogil:
            self.thisptr.setItalic(italic)

    def set_underline(self, bint underline):
        """Set underline decoration"""
        with nogil:
            self.thisptr.setUnderline(underline)

    def set_strikethrough(self, bint strikethrough):
        """Set strikethrough decoration"""
        with nogil:
            self.thisptr.setStrikethrough(strikethrough)

    # ========================================================================
    # LAYOUT PROPERTIES
    # ========================================================================

    def set_line_height(self, float height):
        """Set line height multiplier (1.0 = normal)"""
        with nogil:
            self.thisptr.setLineHeight(height)

    def set_max_lines(self, int lines):
        """Set maximum number of lines (0 = unlimited)"""
        with nogil:
            self.thisptr.setMaxLines(lines)

    def set_text_width(self, float width):
        """Set text width constraint for wrapping (-1 = no constraint)"""
        with nogil:
            self.thisptr.setTextWidth(width)

    def set_halign(self, str align):
        """Set horizontal alignment: 'left', 'center', 'right', 'justify'"""
        cdef string s = align.encode("utf-8")
        with nogil:
            self.thisptr.setHAlign(s)

    def set_shorten(self, bint shorten):
        """Enable text shortening with ellipsis"""
        with nogil:
            self.thisptr.setShorten(shorten)

    # ========================================================================
    # RENDERING
    # ========================================================================

    cpdef render_at(self, uintptr_t canvas_ptr, float x, float y):
        """Render cached texture at position - FAST operation"""
        with nogil:
            self.thisptr.renderAt(<SkCanvas*>canvas_ptr, x, y)

    # ========================================================================
    # GETTERS
    # ========================================================================

    cpdef tuple get_size(self):
        """Get text dimensions (width, height)"""
        cdef float w, h
        with nogil:
            w = self.thisptr.getWidth()
            h = self.thisptr.getHeight()
        return (w, h)

    cpdef dict get_refs(self):
        """
        Get markup references for touch detection
        
        Returns:
            dict: {ref_name: [(x, y, width, height), ...]}
        """
        cdef vector[RefZone] refs
        with nogil:
            refs = self.thisptr.getRefs()
        
        result = {}
        cdef RefZone ref_zone
        for ref_zone in refs:
            name = ref_zone.name.decode('utf-8')
            zone = (ref_zone.x, ref_zone.y, ref_zone.w, ref_zone.h)
            if name not in result:
                result[name] = []
            result[name].append(zone)
        
        return result

    cpdef dict get_anchors(self):
        """
        Get markup anchors for positioning
        
        Returns:
            dict: {anchor_name: (x, y)}
        """
        cdef vector[Anchor] anchors
        with nogil:
            anchors = self.thisptr.getAnchors()
        
        result = {}
        cdef Anchor anchor
        for anchor in anchors:
            name = anchor.name.decode('utf-8')
            result[name] = (anchor.x, anchor.y)
        
        return result

    # ========================================================================
    # CACHE MANAGEMENT
    # ========================================================================

    @staticmethod
    def clear_cache():
        """Clear global texture cache"""
        TextTexture.clearCache()

    @staticmethod
    def get_cache_size():
        """Get number of cached textures"""
        cdef size_t size
        size = TextTexture.getCacheSize()
        return size
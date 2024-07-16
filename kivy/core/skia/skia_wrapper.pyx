cdef extern from "include/core/SkCanvas.h":
    cdef cppclass SkCanvas

cdef extern from "include/core/SkBitmap.h":
    cdef cppclass SkBitmap:
        pass

cdef extern from "include/core/SkRefCnt.h":
    cdef cppclass sk_sp[T]:
        sk_sp() except +
        sk_sp(T*) except +

cdef extern from "modules/svg/include/SkSVGDOM.h":
    cdef cppclass SkSVGDOM:
        SkSVGDOM() except +

cdef extern from "include/core/SkFontMgr.h":
    cdef cppclass SkFontMgr:
        SkFontMgr() except +



cdef extern from "skia_implem.cpp":
    SkBitmap create_bitmap(unsigned int tex_width, unsigned int tex_height) nogil
    SkCanvas *create_canvas(SkBitmap bitmap) nogil
    void draw_canvas(SkCanvas* canvas, SkBitmap bitmap, unsigned int tex_id) nogil
    void draw_canvas_gpu_direct(unsigned int tex_id, unsigned int tex_width, unsigned int tex_height) nogil


    sk_sp[SkSVGDOM] get_svg_dom(const char *svgFilePath) nogil
    void render_svg(sk_sp[SkSVGDOM] svgDom, SkCanvas *canvas, const SkBitmap &bitmap, unsigned int tex_id) nogil

    void render_svg_direct_gpu(sk_sp[SkSVGDOM] svgDom, unsigned int tex_width, unsigned int tex_height, unsigned int buffer_id) nogil




cpdef basic_draw_direct_gpu(tex_id, tex_width, tex_height):
    cdef unsigned int c_tex_id = tex_id
    cdef unsigned int c_tex_width = tex_width
    cdef unsigned int c_tex_height = tex_height
    with nogil:
        draw_canvas_gpu_direct(c_tex_id, c_tex_width, c_tex_height)



cdef class BasicDraw:

    cdef SkBitmap sk_bitmap
    cdef SkCanvas *sk_canvas
    cdef unsigned int tex_id

    
    def __cinit__(self, unsigned int tex_id, unsigned int tex_width, unsigned int tex_height):
        self.tex_id = tex_id
        with nogil:
            self.sk_bitmap = create_bitmap(tex_width, tex_height)
            self.sk_canvas = create_canvas(self.sk_bitmap)

    cpdef draw(self, unsigned int tex_width, unsigned int tex_height):
        with nogil:
            draw_canvas(self.sk_canvas, self.sk_bitmap, self.tex_id)



cdef class SkSvgDraw:

    cdef SkBitmap sk_bitmap
    cdef SkCanvas *sk_canvas
    cdef sk_sp[SkSVGDOM] svg_dom
    cdef unsigned int tex_id

    cdef unsigned int tex_width, tex_height

    
    def __cinit__(self, unsigned int tex_id, unsigned int tex_width, unsigned int tex_height):
        self.tex_id = tex_id
        with nogil:
            self.sk_bitmap = create_bitmap(tex_width, tex_height)
            self.sk_canvas = create_canvas(self.sk_bitmap)

    cpdef load_svg(self, str svg_file):
        cdef bytes svg_bytes = svg_file.encode('utf-8')
        cdef const char *svgFilePath = svg_bytes
        with nogil:
            self.svg_dom = get_svg_dom(svgFilePath)

    cpdef render_svg(self):
        with nogil:
            render_svg(self.svg_dom, self.sk_canvas, self.sk_bitmap, self.tex_id)

    cpdef update_svg_size(self):
        with nogil:
            render_svg(self.svg_dom, self.sk_canvas, self.sk_bitmap, self.tex_id)

    cpdef set_size(self, value):
        self.tex_width = value[0]
        self.tex_height = value[1]
        with nogil:
            self.sk_bitmap = create_bitmap(self.tex_width, self.tex_height)
            self.sk_canvas = create_canvas(self.sk_bitmap)

    cpdef set_texture_id(self, unsigned int tex_id):
        self.tex_id = tex_id





cdef class SkSvgDrawDirectGPU:

    cdef sk_sp[SkSVGDOM] svg_dom
    cdef unsigned int tex_id

    cdef unsigned int tex_width, tex_height
    
    def __cinit__(self, unsigned int tex_id, unsigned int tex_width, unsigned int tex_height):
        self.tex_id = tex_id
        self.tex_width = tex_width
        self.tex_height = tex_height

    cpdef load_svg(self, str svg_file):
        cdef bytes svg_bytes = svg_file.encode('utf-8')
        cdef const char *svgFilePath = svg_bytes
        with nogil:
            self.svg_dom = get_svg_dom(svgFilePath)

    cpdef render_svg(self):
        with nogil:
            render_svg_direct_gpu(self.svg_dom, self.tex_width, self.tex_height, self.tex_id)

    cpdef set_texture_id(self, unsigned int tex_id):
        self.tex_id = tex_id

    cpdef set_size(self, value):
        self.tex_width = value[0]
        self.tex_height = value[1]

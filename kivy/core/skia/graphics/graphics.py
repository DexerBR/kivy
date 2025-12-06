import time
from kivy.clock import triggered
from kivy.core.skia.graphics.abstract import (
    Instruction,
    RenderContext,
    Canvas,
    VertexInstruction,
)
from kivy.factory import Factory
from kivy.core.skia.skia_graphics import Surface, Ellipse, Rectangle


class SkiaState:
    active_render_context = None
    canvas_ptr = None

    @classmethod
    def set_render_context(cls, ctx):
        cls.active_render_context = ctx

    @classmethod
    def set_canvas_ptr(cls, ptr):
        cls.canvas_ptr = ptr


class SkiaRenderContext(RenderContext):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.skia_surface = None

        SkiaState.set_render_context(self)

    def draw(self):
        super().draw()
        self.flush_and_submit()

    def update_viewport(self, size):
        skia_surface = Surface(*map(int, size))
        SkiaState.set_canvas_ptr(skia_surface.get_canvas_ptr())
        self.skia_surface = skia_surface

    def flush_and_submit(self):
        self.skia_surface.flush_and_submit()

    def clear(self, clearcolor):
        self.skia_surface.clear_canvas(
            *list(map(lambda x: x * 255, clearcolor))
        )


class SkiaCanvas(Canvas):
    pass


class SkiaEllipse(VertexInstruction):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._x, self._y = kwargs.get("pos", (0, 0))
        self._w, self._h = kwargs.get("size", (100, 100))
        self._segments = kwargs.get("segments", 4)
        self._angle_start = kwargs.get("angle_start", 0)
        self._angle_end = kwargs.get("angle_end", 360)
        self._skia_ellipse = Ellipse()

        self.build()

    def build(self):
        """Sync Python properties to C++ object"""
        start_angle = 90 - self._angle_start
        end_angle = 90 - self._angle_end
        self._skia_ellipse.set_geometry(
            self._x,
            self._y,
            self._w,
            self._h,
            start_angle,
            end_angle,
            self._segments,
        )

    def apply(self):
        self._skia_ellipse.render(SkiaState.canvas_ptr)
        return super().apply()

    @property
    def pos(self):
        return (self._x, self._y)

    @pos.setter
    def pos(self, pos):
        x, y = pos
        if self._x == x and self._y == y:
            return
        self._x = x
        self._y = y
        self.flag_data_update()

    @property
    def size(self):
        return (self._w, self._h)

    @size.setter
    def size(self, size):
        w, h = size
        if self._w == w and self._h == h:
            return
        self._w = w
        self._h = h
        self.flag_data_update()

    @property
    def segments(self):
        return self._segments

    @segments.setter
    def segments(self, value):
        if self._segments == value:
            return
        self._segments = value
        self.flag_data_update()

    @property
    def angle_start(self):
        return self._angle_start

    @angle_start.setter
    def angle_start(self, value):
        if self._angle_start == value:
            return
        self._angle_start = value
        self.flag_data_update()

    @property
    def angle_end(self):
        return self._angle_end

    @angle_end.setter
    def angle_end(self, value):
        if self._angle_end == value:
            return
        self._angle_end = value
        self.flag_data_update()

    @property
    def texture(self):
        return getattr(self, "_texture", None)

    @texture.setter
    def texture(self, value):
        self._texture = value
        if value:
            self._skia_ellipse.set_texture(str(value))
        else:
            self._skia_ellipse.clear_texture()
        self.flag_data_update()


class SkiaRectangle(VertexInstruction):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self._x, self._y = kwargs.get("pos", (0, 0))
        self._w, self._h = kwargs.get("size", (100, 100))
        self._skia_rect = Rectangle()

        self.build()

    def build(self):
        self._skia_rect.set_geometry(self._x, self._y, self._w, self._h)

    def apply(self):
        self._skia_rect.render(SkiaState.canvas_ptr)
        return super().apply()

    @property
    def pos(self):
        return self._x, self._y

    @pos.setter
    def pos(self, value):
        x, y = value
        if (x, y) != (self._x, self._y):
            self._x, self._y = x, y
            self.flag_data_update()

    @property
    def size(self):
        return self._w, self._h

    @size.setter
    def size(self, value):
        w, h = value
        if (w, h) != (self._w, self._h):
            self._w, self._h = w, h
            self.flag_data_update()

    @property
    def texture(self):
        return getattr(self, "_texture", None)

    @texture.setter
    def texture(self, value):
        self._texture = value
        if value:
            self._skia_rect.set_texture(str(value))
        else:
            self._skia_rect.clear_texture()
        self.flag_data_update()


Factory.register("Rectangle", SkiaRectangle)
Factory.register("Ellipse", SkiaEllipse)

import os
# os.environ["KIVY_GL_BACKEND"] = "angle_sdl2"

import os
from kivy.app import App
from kivy.lang import Builder
from kivy.uix.floatlayout import FloatLayout
from kivy.graphics.texture import Texture
from kivy.properties import ListProperty, StringProperty, NumericProperty
from kivy.core.window import Window
from kivy.graphics.instructions import Instruction

from kivy.graphics import Rectangle
from kivy.factory import Factory
from pathlib import Path


from skia_wrapper import SkSvgDraw, SkSvgDrawDirectGPU



class SkiaSVG(Instruction):
    def __init__(self, pos=(0, 0), size=(100, 100), source="", **kwargs):
        super().__init__(**kwargs)
        self._pos = pos
        self._size = size
        self._source = source
        self._kv_texture = Texture.create(size=size, colorfmt="rgba")
        self._kv_texture.flip_vertical()
        self._kv_texture.bind()  # forces updating the tex.id otherwise it will be 0
        self._texture_container = Rectangle(texture=self._kv_texture)
        self._sk_svg_wrapper = SkSvgDraw(
            self._kv_texture.id, *self._kv_texture.size
        )

    @property
    def pos(self):
        return self._pos

    @pos.setter
    def pos(self, value):
        self._pos = value
        self._texture_container.pos = value

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, value):
        self._size = value
        self._texture_container.size = value
        self._update_attached_texture(value)

    def _update_attached_texture(self, value):
        self._kv_texture = Texture.create(size=value, colorfmt="rgba")
        self._kv_texture.flip_vertical()
        self._kv_texture.bind()  # forces updating the tex.id otherwise it will be 0
        self._texture_container.texture = self._kv_texture
        self._sk_svg_wrapper.set_texture_id(self._kv_texture.id)
        self._sk_svg_wrapper.set_size(value)
        if self.source:
            self._sk_svg_wrapper.render_svg()

    @property
    def source(self):
        return self._source

    @source.setter
    def source(self, value):
        self._source = value
        if not value:
            return
        self._sk_svg_wrapper.load_svg(value)
        self._sk_svg_wrapper.render_svg()


Factory.register("SkiaSVG", SkiaSVG)


class SkiaSVG_GPU(Instruction):
    def __init__(self, pos=(0, 0), size=(100, 100), source="", **kwargs):
        super().__init__(**kwargs)
        self._pos = pos
        self._size = size
        self._source = source
        self._kv_texture = Texture.create(size=size, colorfmt="rgba")
        self._kv_texture.flip_vertical()
        self._kv_texture.bind()  # forces updating the tex.id otherwise it will be 0
        self._texture_container = Rectangle(texture=self._kv_texture)
        self._sk_svg_wrapper = SkSvgDrawDirectGPU(
            self._kv_texture.id, *self._kv_texture.size
        )

    @property
    def pos(self):
        return self._pos

    @pos.setter
    def pos(self, value):
        self._pos = value
        self._texture_container.pos = value

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, value):
        self._size = value
        self._texture_container.size = value
        self._update_attached_texture(value)

    def _update_attached_texture(self, value):
        self._kv_texture = Texture.create(size=value, colorfmt="rgba")
        self._kv_texture.flip_vertical()
        self._kv_texture.bind()  # forces updating the tex.id otherwise it will be 0
        self._texture_container.texture = self._kv_texture
        self._sk_svg_wrapper.set_texture_id(self._kv_texture.id)

        self._sk_svg_wrapper.set_size(value)
        if self.source:
            self._sk_svg_wrapper.render_svg()

    @property
    def source(self):
        return self._source

    @source.setter
    def source(self, value):
        self._source = value
        if not value:
            return
        self._sk_svg_wrapper.load_svg(value)
        self._sk_svg_wrapper.render_svg()


Factory.register("SkiaSVG_GPU", SkiaSVG_GPU)


class UI(FloatLayout):
    Builder.load_string(r"""
<UI>
    canvas:
        Color:
            rgba: 1, 0, 1, 0.2
        Ellipse:
            pos: self.x, 200
            size: self.width, self.height / 2


        # SVG bg
        Color:
            rgba: 1, 1, 1, 0.2
        Rectangle:
            pos: root.touch_pos[0] - (250 * root.svg_scale), root.touch_pos[1] - (250 * root.svg_scale)
            size: 500 * root.svg_scale, 500 * root.svg_scale

        Color:
            rgba: 1, 1, 1, 1
        # SkiaSVG_GPU:
        SkiaSVG:
            pos: root.touch_pos[0] - (250 * root.svg_scale), root.touch_pos[1] - (250 * root.svg_scale)
            size: 500 * root.svg_scale, 500 * root.svg_scale
            source: root.svg_source

        Color:
            rgba: 0, 0, 1, 0.2
        Ellipse:
            pos: self.x + 200, 200
            size: self.width, self.height


    Button:
        text: 'Update svg file'
        size_hint: 0.3, 0.2
        pos_hint: {'center_x': 0.5, 'center_y': 0.5}
        on_release:
            root.svg_source = root.get_next_svg()
                        

    Slider:
        size_hint_y: None
        height: 50
        min: 1
        max: 2
        step: 0.25
        on_value:
            root.svg_scale = self.value
                        
""")

    touch_pos = ListProperty([0, 0])

    svg_source = StringProperty()
    svg_scale = NumericProperty(1)

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.directory = Path(__file__).resolve().parent / "svgs"
        self.svg_files = [file for file in os.listdir(self.directory) if file.lower().endswith('.svg')]
        self.svg_file_index = 0

    def on_touch_move(self, touch):
        if touch.pos[1] < 100:
            return
        self.touch_pos = touch.pos
        return super().on_touch_move(touch)
    

    def get_next_svg(self):
        if self.svg_file_index >= len(self.svg_files):
            self.svg_file_index = 0
        file_path = os.path.join(self.directory, self.svg_files[self.svg_file_index])
        self.svg_file_index += 1
        return file_path



class MyApp(App):
    def build(self):
        return UI()


if __name__ == "__main__":
    MyApp().run()

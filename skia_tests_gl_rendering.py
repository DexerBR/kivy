import os
# os.environ["KIVY_GL_BACKEND"] = "angle_sdl2"

from kivy.app import App
from kivy.lang import Builder
from kivy.uix.floatlayout import FloatLayout
from kivy.graphics.texture import Texture
from kivy.properties import ObjectProperty, ListProperty
from kivy.clock import Clock
from kivy.core.window import Window


from kivy.core.skia.skia_wrapper import BasicDraw, basic_draw_direct_gpu


class UI(FloatLayout):
    Builder.load_string(r"""
<UI>
    canvas:
        Color:
            rgba: 1, 0, 1, 0.2
        Ellipse:
            pos: self.x, 200
            size: self.width, self.height / 2

        Color:
            rgba: 1, 1, 1, 1
        Ellipse:
            pos: 100, 200
            size: 300, 300
            texture: root.tex_1

        Color:
            rgba: 1, 1, 1, 1
        Ellipse:
            pos: root.touch_pos[0] - 250, root.touch_pos[1] - 250
            size: 500, 500
            texture: root.tex_2

        Color:
            rgba: 0, 0, 1, 0.2
        Ellipse:
            pos: self.x + 200, 200
            size: self.width, self.height


    Button:
        text: 'Update texture'
        size_hint: 0.3, 0.2
        pos_hint: {'center_x': 0.5, 'center_y': 0.5}
        on_release:
            # CPU
            # root.run_1()
            
            # GPU
            root.run_2()
""")

    tex_1 = ObjectProperty(None, allownone=True)
    tex_2 = ObjectProperty(None, allownone=True)

    touch_pos = ListProperty([0, 0])

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.tex_1 = Texture.create(size=(500, 500), colorfmt="rgba")
        self.tex_1.flip_vertical()
        self.tex_1.bind()  # forces updating the tex.id otherwise it will be 0
        self._skia_draw_1 = BasicDraw(
            self.tex_1.id, self.tex_1.size[0], self.tex_1.size[1]
        )

        self.tex_2 = Texture.create(size=(500, 500), colorfmt="rgba")
        self.tex_2.flip_vertical()
        self.tex_2.bind()  # forces updating the tex.id otherwise it will be 0
        self._skia_draw_2 = BasicDraw(
            self.tex_2.id, self.tex_2.size[0], self.tex_2.size[1]
        )

    def on_touch_move(self, touch):
        self.touch_pos = touch.pos
        return super().on_touch_move(touch)

    def run_1(self):
        self._skia_draw_1.draw(self.tex_1.size[0], self.tex_1.size[1])
        self.canvas.ask_update()

        self._skia_draw_2.draw(self.tex_2.size[0], self.tex_2.size[1])
        self.canvas.ask_update()

        # stress test
        def callback(_):
            self._skia_draw_1.draw(self.tex_1.size[0], self.tex_1.size[1])
            self.canvas.ask_update()

            self._skia_draw_2.draw(self.tex_2.size[0], self.tex_2.size[1])
            self.canvas.ask_update()

        Clock.schedule_interval(callback, 1 / 30)

    def run_2(self):
        basic_draw_direct_gpu(
            self.tex_2.id, self.tex_2.size[0], self.tex_1.size[1]
        )
        self.canvas.ask_update()

        basic_draw_direct_gpu(
            self.tex_1.id, self.tex_1.size[0], self.tex_1.size[1]
        )
        self.canvas.ask_update()

        # stress test
        def callback(_):
            basic_draw_direct_gpu(
                self.tex_2.id, self.tex_2.size[0], self.tex_1.size[1]
            )
            self.canvas.ask_update()

            basic_draw_direct_gpu(
                self.tex_1.id, self.tex_1.size[0], self.tex_1.size[1]
            )
            self.canvas.ask_update()

        Clock.schedule_interval(callback, 1 / 30)


class MyApp(App):
    def build(self):
        return UI()


if __name__ == "__main__":
    MyApp().run()

"""
After running `python main.py -m monitor` and hitting the "update texture" button 10 times

After ~3 min:

|       | CPU | GPU | MEM   | FPS |
|-------|-----|-----|-------|-----|
| run_1 | 5   | 29  | 102MB | 81  |
| run_2 | 14  | 35  | 81MB  | 26  |
"""

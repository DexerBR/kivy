import os

os.environ["GRAPHICS_ENGINE"] = "skia"
from kivy.app import App
from kivy.uix.label import Label


class TestApp(App):
    def on_ref_press(self, instance, ref_name):
        print(f"Ref pressed: {ref_name}")

    def build(self):
        label = Label(
            text=(
                "[size=48][b][color=#ff0000]Breaking[/color] [color=#00ff00]News[/color][/b][/size]\n"
                "[size=18][i]Kivy discovers [u]Skia[/u] rendering[/i][/size]\n\n"
                "[size=16]The [b][ref=discovery]discovery[/ref][/b] includes [color=#ff8800]GPU acceleration[/color], "
                "[i]better text[/i], and [s]old limits[/s] are gone. "
                "[ref=more]Read more[/ref] about this [b][i]breakthrough[/i][/b].[/size]\n\n"
                "[size=14]Formula: H[sub]2[/sub]O and E=mc[sup]2[/sup][/size]\n\n"
                "[size=14][color=#888888][ref=team]Kivy Team[/ref] | Dec 2025[/color][/size]"
            ),
            markup=True,
        )

        label.bind(on_ref_press=self.on_ref_press)
        return label


TestApp().run()

from kivy.core.skia.graphics.abstract import Instruction
from kivy.uix.widget import Widget
from kivy.properties import (
    StringProperty,
    NumericProperty,
    ColorProperty,
    ListProperty,
    BooleanProperty,
    OptionProperty,
    ObjectProperty,
    DictProperty,
    VariableListProperty,
)
from kivy.clock import Clock
from kivy.core.skia.graphics.graphics import SkiaState
from kivy.core.skia.skia_graphics import TextTextureWrapper


class SkiaTextOptimized(Instruction):
    """Skia text rendering instruction with full styling support"""

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._texture = TextTextureWrapper()

        # Extract and store all properties
        self._text = kwargs.get("text", "")
        self._x, self._y = kwargs.get("pos", (0, 0))
        self._font_size = kwargs.get("font_size", 15)
        self._color = kwargs.get("color", (1, 1, 1, 1))
        self._font_family = kwargs.get("font_family", "Arial")
        self._markup = kwargs.get("markup", False)
        self._bold = kwargs.get("bold", False)
        self._italic = kwargs.get("italic", False)
        self._underline = kwargs.get("underline", False)
        self._strikethrough = kwargs.get("strikethrough", False)
        self._line_height = kwargs.get("line_height", 1.0)
        self._max_lines = kwargs.get("max_lines", 0)
        self._text_width = kwargs.get("text_width", -1)
        self._halign = kwargs.get("halign", "left")

        self._update_texture()

    def _update_texture(self):
        """Update all texture properties"""
        self._texture.set_text(self._text)
        self._texture.set_font_size(int(self._font_size))
        self._texture.set_font_family(self._font_family)
        self._texture.set_color(tuple(self._color))
        self._texture.set_markup(self._markup)
        self._texture.set_bold(self._bold)
        self._texture.set_italic(self._italic)
        self._texture.set_underline(self._underline)
        self._texture.set_strikethrough(self._strikethrough)
        self._texture.set_line_height(self._line_height)
        self._texture.set_max_lines(self._max_lines)
        self._texture.set_text_width(self._text_width)
        self._texture.set_halign(self._halign)

    def build(self):
        pass

    def apply(self):
        if self._text and SkiaState.canvas_ptr:
            try:
                self._texture.render_at(SkiaState.canvas_ptr, self._x, self._y)
            except Exception as e:
                print(f"ERROR in SkiaTextOptimized.apply(): {e}")
        return super().apply()

    # Property setters with efficient change detection
    @property
    def text(self):
        return self._text

    @text.setter
    def text(self, value):
        if self._text != value:
            self._text = value
            self._texture.set_text(value)
            self.flag_data_update()

    @property
    def pos(self):
        return (self._x, self._y)

    @pos.setter
    def pos(self, pos):
        x, y = pos
        if self._x != x or self._y != y:
            self._x, self._y = x, y
            self.flag_data_update()

    @property
    def font_size(self):
        return self._font_size

    @font_size.setter
    def font_size(self, value):
        if self._font_size != value:
            self._font_size = value
            self._texture.set_font_size(int(value))
            self.flag_data_update()

    @property
    def color(self):
        return self._color

    @color.setter
    def color(self, value):
        self._color = tuple(value)
        self._texture.set_color(tuple(value))
        self.flag_data_update()

    @property
    def bold(self):
        return self._bold

    @bold.setter
    def bold(self, value):
        if self._bold != value:
            self._bold = value
            self._texture.set_bold(value)
            self.flag_data_update()

    @property
    def italic(self):
        return self._italic

    @italic.setter
    def italic(self, value):
        if self._italic != value:
            self._italic = value
            self._texture.set_italic(value)
            self.flag_data_update()

    @property
    def text_width(self):
        return self._text_width

    @text_width.setter
    def text_width(self, value):
        if self._text_width != value:
            self._text_width = value
            self._texture.set_text_width(value)
            self.flag_data_update()

    @property
    def halign(self):
        return self._halign

    @halign.setter
    def halign(self, value):
        if self._halign != value:
            self._halign = value
            self._texture.set_halign(value)
            self.flag_data_update()


class SkiaCoreLabel:
    """Core label for text measurement with full feature support"""

    def __init__(
        self,
        text="",
        font_size=15,
        font_family=None,
        markup=False,
        bold=False,
        italic=False,
        underline=False,
        strikethrough=False,
        line_height=1.0,
        max_lines=0,
        text_width=-1,
        halign="left",
    ):
        self.text = text
        self.font_size = font_size
        self.font_family = font_family or "Arial"
        self.markup = markup
        self.bold = bold
        self.italic = italic
        self.underline = underline
        self.strikethrough = strikethrough
        self.line_height = line_height
        self.max_lines = max_lines
        self.text_width = text_width
        self.halign = halign

        self._texture = TextTextureWrapper()
        self._size = (0, 0)
        self.refresh()

    def get_extents(self):
        if not self.text:
            return (0, 0)

        self._texture.set_text(self.text)
        self._texture.set_font_size(int(self.font_size))
        self._texture.set_font_family(self.font_family)
        self._texture.set_markup(self.markup)
        self._texture.set_bold(self.bold)
        self._texture.set_italic(self.italic)
        self._texture.set_underline(self.underline)
        self._texture.set_strikethrough(self.strikethrough)
        self._texture.set_line_height(self.line_height)
        self._texture.set_max_lines(self.max_lines)
        self._texture.set_text_width(self.text_width)
        self._texture.set_halign(self.halign)

        return self._texture.get_size()

    def refresh(self):
        if not self.text:
            self._size = (0, 0)
            return
        self._size = self.get_extents()


class SkiaLabel(Widget):
    """
    Skia-based Label with full Kivy API compatibility

    :Events:
        `on_ref_press`
            Fired when user clicks on [ref] tag in markup
    """

    __events__ = ["on_ref_press"]

    # Core text properties
    text = StringProperty("")
    font_size = NumericProperty(15)
    font_name = StringProperty("Arial")
    color = ColorProperty([1, 1, 1, 1])

    # Text sizing & layout
    text_size = ListProperty([None, None])
    halign = OptionProperty(
        "auto", options=["left", "center", "right", "justify", "auto"]
    )
    valign = OptionProperty(
        "bottom", options=["bottom", "middle", "center", "top"]
    )
    padding = VariableListProperty([0, 0, 0, 0], length=4)

    # Text styling
    bold = BooleanProperty(False)
    italic = BooleanProperty(False)
    underline = BooleanProperty(False)
    strikethrough = BooleanProperty(False)
    line_height = NumericProperty(1.0)

    # Markup & references
    markup = BooleanProperty(False)
    refs = DictProperty({})
    anchors = DictProperty({})

    # Text shortening
    shorten = BooleanProperty(False)
    shorten_from = OptionProperty("center", options=["left", "center", "right"])
    is_shortened = BooleanProperty(False)
    split_str = StringProperty("")
    max_lines = NumericProperty(0)
    strip = BooleanProperty(False)

    # Outline
    outline_width = NumericProperty(None, allownone=True)
    outline_color = ColorProperty([0, 0, 0, 1])

    # Disabled state
    disabled_color = ColorProperty([1, 1, 1, 0.3])
    disabled_outline_color = ColorProperty([0, 0, 0, 1])

    # Texture
    texture = ObjectProperty(None, allownone=True)
    texture_size = ListProperty([0, 0])
    mipmap = BooleanProperty(False)

    def __init__(self, **kwargs):
        self._trigger_refresh = Clock.create_trigger(self._update_texture, -1)
        self._trigger_position = Clock.create_trigger(self._update_position, -1)
        
        super(SkiaLabel, self).__init__(**kwargs)

        self._label = SkiaCoreLabel(
            text=self.text,
            font_size=self.font_size,
            font_family=self.font_name,
            markup=self.markup,
            bold=self.bold,
            italic=self.italic,
            underline=self.underline,
            strikethrough=self.strikethrough,
            line_height=self.line_height,
            max_lines=self.max_lines,
            text_width=self.text_size[0] if self.text_size[0] else -1,
            halign=self.halign if self.halign != "auto" else "left",
        )

        self._instruction = None
        self._create_instruction()

        # Bind all properties that affect rendering
        self.bind(
            text=self._on_text_change,
            font_size=self._on_property_change,
            color=self._on_property_change,
            font_name=self._on_property_change,
            markup=self._on_property_change,
            bold=self._on_property_change,
            italic=self._on_property_change,
            underline=self._on_property_change,
            strikethrough=self._on_property_change,
            line_height=self._on_property_change,
            max_lines=self._on_property_change,
            text_size=self._on_layout_change,
            halign=self._on_layout_change,
            valign=self._on_layout_change,
            disabled=self._on_disabled_change,
            # CRITICAL: Bind position AND size changes
            pos=self._trigger_position,
            size=self._trigger_position,
            texture_size=self._trigger_position,
        )

        self._trigger_refresh()

    def _create_instruction(self):
        """Create optimized SkiaText instruction"""
        effective_halign = self.halign if self.halign != "auto" else "left"

        # Calculate initial position based on Widget defaults
        render_pos = self._calculate_render_position()

        self._instruction = SkiaTextOptimized(
            text=self.text,
            pos=render_pos,
            font_size=self.font_size,
            color=tuple(self.disabled_color if self.disabled else self.color),
            font_family=self.font_name,
            markup=self.markup,
            bold=self.bold,
            italic=self.italic,
            underline=self.underline,
            strikethrough=self.strikethrough,
            line_height=self.line_height,
            max_lines=self.max_lines,
            text_width=self.text_size[0] if self.text_size[0] else -1,
            halign=effective_halign,
        )
        self.canvas.add(self._instruction)

    def _calculate_render_position(self):
        """
        Calculate render position based on widget pos/size and text alignment
        Mimics native Label behavior:
        - Texture always centered in widget (both H and V)
        - halign/valign only affect text position INSIDE texture when text_size is set
        
        CRITICAL: Round to integer pixels to avoid blur from sub-pixel rendering
        """
        # Get texture dimensions
        tw, th = self.texture_size
        
        # Widget dimensions  
        wx, wy = self.pos
        ww, wh = self.size
        
        # Native behavior: texture is ALWAYS centered in widget
        # (halign/valign don't affect this - they only affect text inside texture)
        x = wx + (ww - tw) / 2.0
        y = wy + (wh - th) / 2.0
        
        # CRITICAL: Round to integer pixels for crisp rendering
        # Float positions cause blur due to sub-pixel interpolation
        return (int(round(x)), int(round(y)))

    def _update_position(self, *args):
        """Update instruction position when widget moves/resizes"""
        if self._instruction:
            self._instruction.pos = self._calculate_render_position()

    def _on_text_change(self, *args):
        """Handle text changes"""
        if self._instruction:
            self._instruction.text = self.text
        if self._label:
            self._label.text = self.text
        self._trigger_refresh()

    def _on_property_change(self, *args):
        """Handle styling property changes"""
        if self._instruction:
            self._instruction.font_size = self.font_size
            self._instruction.color = tuple(
                self.disabled_color if self.disabled else self.color
            )
            self._instruction.bold = self.bold
            self._instruction.italic = self.italic
            # Trigger full update
            self._instruction.text = self.text

        if self._label:
            self._label.font_size = self.font_size
            self._label.font_family = self.font_name
            self._label.markup = self.markup
            self._label.bold = self.bold
            self._label.italic = self.italic
            self._label.underline = self.underline
            self._label.strikethrough = self.strikethrough
            self._label.line_height = self.line_height
            self._label.max_lines = self.max_lines

        self._trigger_refresh()

    def _on_layout_change(self, *args):
        """Handle layout property changes"""
        effective_halign = self.halign if self.halign != "auto" else "left"
        text_width = self.text_size[0] if self.text_size[0] else -1

        if self._instruction:
            self._instruction.text_width = text_width
            self._instruction.halign = effective_halign

        if self._label:
            self._label.text_width = text_width
            self._label.halign = effective_halign

        self._trigger_refresh()

    def _on_disabled_change(self, *args):
        """Handle disabled state changes"""
        if self._instruction:
            self._instruction.color = tuple(
                self.disabled_color if self.disabled else self.color
            )

    def _update_texture(self, *args):
        """Update texture_size and refs/anchors"""
        if not self._label:
            return

        self._label.refresh()
        self.texture_size = list(self._label._size)

        # Get refs and anchors from C++ layer
        if self.markup and self._label._texture:
            self.refs = self._label._texture.get_refs()
            self.anchors = self._label._texture.get_anchors()
        else:
            self.refs = {}
            self.anchors = {}

    def texture_update(self, *args):
        """Force texture recreation (Kivy API compatibility)"""
        self._update_texture(*args)

    def on_touch_down(self, touch):
        """Handle touch for ref detection"""
        if super(SkiaLabel, self).on_touch_down(touch):
            return True

        if not len(self.refs):
            return False

        # Calculate touch position relative to texture
        tx = touch.pos[0] - (self.center_x - self.texture_size[0] / 2.0)
        ty = touch.pos[1] - (self.center_y - self.texture_size[1] / 2.0)
        ty = self.texture_size[1] - ty

        # Check if touch is inside any ref zone
        for uid, zones in self.refs.items():
            for zone in zones:
                x, y, w, h = zone
                if x <= tx <= w and y <= ty <= h:
                    self.dispatch("on_ref_press", uid)
                    return True

        return False

    def on_ref_press(self, ref):
        """Event handler for ref press"""
        pass


def escape_markup(text):
    """
    Escape markup characters in text
    """
    text = str(text)
    text = text.replace("&", "&amp;")
    text = text.replace("[", "&bl;")
    text = text.replace("]", "&br;")
    return text


__all__ = ("SkiaLabel", "escape_markup")
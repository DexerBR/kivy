#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// STL
#include <cmath>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <regex>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

// Core Skia
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkData.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkRRect.h"
#include "include/core/SkRect.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkStream.h"
#include "include/core/SkString.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTypeface.h"

// Effects and filters
#include "include/effects/SkImageFilters.h"

// SkParagraph (text layout)
#include "modules/skparagraph/include/FontCollection.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphBuilder.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/include/TextStyle.h"

// Skottie
#include "modules/skottie/include/Skottie.h"

// Platform-specific font manager
#ifdef _WIN32
#include "include/ports/SkTypeface_win.h"
#elif defined(__APPLE__)
#include "include/ports/SkFontMgr_mac_ct.h"
#elif defined(__ANDROID__)
#include "include/ports/SkFontMgr_android.h"
#else
#include "include/ports/SkFontMgr_fontconfig.h"
#endif

using namespace skia::textlayout;

// ============================================================================
// FONT MANAGEMENT
// ============================================================================

static sk_sp<SkFontMgr> g_font_manager;
static sk_sp<FontCollection> g_font_collection;

static sk_sp<SkFontMgr> getPlatformFontMgr() {
    if (!g_font_manager) {
#ifdef _WIN32
        g_font_manager = SkFontMgr_New_DirectWrite();
#elif defined(__APPLE__)
        g_font_manager = SkFontMgr_New_CoreText(nullptr);
#elif defined(__ANDROID__)
        g_font_manager = SkFontMgr_New_Android(nullptr);
#else
        g_font_manager = SkFontMgr_New_FontConfig(nullptr);
#endif
    }
    return g_font_manager;
}

static sk_sp<FontCollection> getFontCollection() {
    if (!g_font_collection) {
        g_font_collection = sk_make_sp<FontCollection>();
        g_font_collection->setDefaultFontManager(getPlatformFontMgr());
    }
    return g_font_collection;
}

void loadFont(const char* font_path) {
    auto typeface = getPlatformFontMgr()->makeFromFile(font_path, 0);
    if (!typeface) {
        std::cerr << "WARNING: Failed to load font: " << font_path << '\n';
    }
}

void debugListFonts() {
    auto mgr = getPlatformFontMgr();
    std::cout << "\n=== Available Fonts (" << mgr->countFamilies() << ") ===\n";
    for (int i = 0; i < std::min(20, mgr->countFamilies()); ++i) {
        SkString name;
        mgr->getFamilyName(i, &name);
        std::cout << i << ": " << name.c_str() << "\n";
    }
    std::cout << "==============================\n\n";
}

// ============================================================================
// MARKUP PARSER (with ref/anchor support)
// ============================================================================

struct MarkupStyle {
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    std::string font_family;
    int font_size = -1;
    SkColor color = SK_ColorWHITE;
    bool has_color = false;
};

struct MarkupSegment {
    std::string text;
    MarkupStyle style;
};

class MarkupParser {
private:
    static std::string unescape(const std::string& s) {
        std::string r = s;
        size_t pos = 0;
        while ((pos = r.find("&bl;", pos)) != std::string::npos) {
            r.replace(pos, 4, "[");
            pos += 1;
        }
        pos = 0;
        while ((pos = r.find("&br;", pos)) != std::string::npos) {
            r.replace(pos, 4, "]");
            pos += 1;
        }
        pos = 0;
        while ((pos = r.find("&amp;", pos)) != std::string::npos) {
            r.replace(pos, 5, "&");
            pos += 1;
        }
        return r;
    }

    static SkColor parseColor(const std::string& hex) {
        std::string h = hex;
        if (!h.empty() && h[0] == '#') {
            h = h.substr(1);
        }
        if (h.size() == 6) {
            int r = std::stoi(h.substr(0, 2), nullptr, 16);
            int g = std::stoi(h.substr(2, 2), nullptr, 16);
            int b = std::stoi(h.substr(4, 2), nullptr, 16);
            return SkColorSetARGB(255, r, g, b);
        }
        if (h.size() == 8) {
            int a = std::stoi(h.substr(6, 2), nullptr, 16);
            int r = std::stoi(h.substr(0, 2), nullptr, 16);
            int g = std::stoi(h.substr(2, 2), nullptr, 16);
            int b = std::stoi(h.substr(4, 2), nullptr, 16);
            return SkColorSetARGB(a, r, g, b);
        }
        return SK_ColorWHITE;
    }

    static bool startsWith(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() &&
            str.compare(0, prefix.size(), prefix) == 0;
    }

public:
    struct Result {
        std::vector<MarkupSegment> segments;
        std::vector<std::pair<std::string, size_t>> refs;     // (name, start_char_pos)
        std::vector<std::pair<std::string, size_t>> anchors;  // (name, char_pos)
    };

    static std::vector<MarkupSegment> parseSimple(const std::string& markup_text) {
        std::vector<MarkupSegment> segments;
        std::stack<MarkupStyle> stack;
        MarkupStyle cur;
        stack.push(cur);
        std::string buf;
        size_t i = 0;

        while (i < markup_text.size()) {
            if (markup_text[i] == '[') {
                if (!buf.empty()) {
                    segments.push_back({ unescape(buf), cur });
                    buf.clear();
                }
                size_t end = markup_text.find(']', i + 1);
                if (end == std::string::npos) {
                    buf += markup_text[i++];
                    continue;
                }
                std::string tag = markup_text.substr(i + 1, end - i - 1);
                i = end + 1;

                if (tag == "b") {
                    cur.bold = true;
                }
                else if (tag == "/b") {
                    cur.bold = false;
                }
                else if (tag == "i") {
                    cur.italic = true;
                }
                else if (tag == "/i") {
                    cur.italic = false;
                }
                else if (tag == "u") {
                    cur.underline = true;
                }
                else if (tag == "/u") {
                    cur.underline = false;
                }
                else if (tag == "s") {
                    cur.strikethrough = true;
                }
                else if (tag == "/s") {
                    cur.strikethrough = false;
                }
                else if (startsWith(tag, "font_family=")) {
                    cur.font_family = tag.substr(12);
                }
                else if (tag == "/font_family") {
                    cur.font_family.clear();
                }
                else if (startsWith(tag, "size=")) {
                    cur.font_size = std::stoi(tag.substr(5));
                }
                else if (tag == "/size") {
                    cur.font_size = -1;
                }
                else if (startsWith(tag, "color=")) {
                    cur.color = parseColor(tag.substr(6));
                    cur.has_color = true;
                }
                else if (tag == "/color") {
                    cur.has_color = false;
                }
            }
            else {
                buf += markup_text[i++];
            }
        }
        if (!buf.empty()) {
            segments.push_back({ unescape(buf), cur });
        }
        return segments;
    }

    static Result parseWithRefs(const std::string& markup_text) {
        Result res;
        std::stack<MarkupStyle> stack;
        MarkupStyle cur;
        stack.push(cur);

        std::string buf;
        size_t char_pos = 0;
        size_t i = 0;
        std::string active_ref;
        size_t ref_start = 0;

        while (i < markup_text.size()) {
            if (markup_text[i] == '[') {
                size_t end = markup_text.find(']', i + 1);
                if (end == std::string::npos) {
                    buf += markup_text[i++];
                    char_pos++;
                    continue;
                }

                if (!buf.empty()) {
                    res.segments.push_back({ unescape(buf), cur });
                    buf.clear();
                }

                std::string tag = markup_text.substr(i + 1, end - i - 1);
                i = end + 1;

                if (startsWith(tag, "ref=")) {
                    active_ref = tag.substr(4);
                    ref_start = char_pos;
                }
                else if (tag == "/ref") {
                    if (!active_ref.empty()) {
                        res.refs.emplace_back(active_ref, ref_start);
                        active_ref.clear();
                    }
                }
                else if (startsWith(tag, "anchor=")) {
                    res.anchors.emplace_back(tag.substr(7), char_pos);
                }
                // Apply styles
                else if (tag == "b") { cur.bold = true; }
                else if (tag == "/b") { cur.bold = false; }
                else if (tag == "i") { cur.italic = true; }
                else if (tag == "/i") { cur.italic = false; }
                else if (tag == "u") { cur.underline = true; }
                else if (tag == "/u") { cur.underline = false; }
                else if (tag == "s") { cur.strikethrough = true; }
                else if (tag == "/s") { cur.strikethrough = false; }
                else if (startsWith(tag, "font_family=")) {
                    cur.font_family = tag.substr(12);
                }
                else if (tag == "/font_family") { cur.font_family.clear(); }
                else if (startsWith(tag, "size=")) {
                    cur.font_size = std::stoi(tag.substr(5));
                }
                else if (tag == "/size") { cur.font_size = -1; }
                else if (startsWith(tag, "color=")) {
                    cur.color = parseColor(tag.substr(6));
                    cur.has_color = true;
                }
                else if (tag == "/color") { cur.has_color = false; }
            }
            else {
                buf += markup_text[i++];
                char_pos++;
            }
        }
        if (!buf.empty()) {
            res.segments.push_back({ unescape(buf), cur });
        }
        return res;
    }
};

// ============================================================================
// TEXTURE-BASED TEXT RENDERING
// ============================================================================

struct RefZone {
    std::string name;
    float x, y, w, h;
};

struct Anchor {
    std::string name;
    float x, y;
};

class TextTexture {
private:
    sk_sp<SkImage> m_image;
    std::string m_text;
    int m_font_size = 15;
    std::string m_font_family = "Arial";
    SkColor m_color = SK_ColorWHITE;
    float m_width = 0.0f;
    float m_height = 0.0f;
    bool m_needs_update = true;
    bool m_markup = false;

    // Additional styles
    bool m_bold = false;
    bool m_italic = false;
    bool m_underline = false;
    bool m_strikethrough = false;
    float m_line_height = 1.0f;
    int m_max_lines = 0;
    float m_width_constraint = -1.0f;
    std::string m_halign = "left";
    bool m_shorten = false;

    // Refs & anchors
    std::vector<RefZone> m_refs;
    std::vector<Anchor> m_anchors;

    // Cache
    struct Key {
        std::string text;
        std::string family;
        std::string halign;
        int size;
        SkColor color;
        bool markup;
        bool bold;
        bool italic;
        bool underline;
        bool strikethrough;
        float line_height;
        float width_constraint;
        int max_lines;
        bool shorten;

        bool operator==(const Key& o) const {
            return text == o.text &&
                size == o.size &&
                family == o.family &&
                color == o.color &&
                markup == o.markup &&
                bold == o.bold &&
                italic == o.italic &&
                underline == o.underline &&
                strikethrough == o.strikethrough &&
                std::abs(line_height - o.line_height) < 0.001f &&
                max_lines == o.max_lines &&
                std::abs(width_constraint - o.width_constraint) < 0.001f &&
                halign == o.halign &&
                shorten == o.shorten;
        }
    };

    struct KeyHash {
        std::size_t operator()(const Key& k) const {
            std::size_t h = std::hash<std::string>{}(k.text);
            h ^= (std::hash<int>{}(k.size) << 1);
            h ^= (std::hash<std::string>{}(k.family) << 2);
            h ^= (std::hash<SkColor>{}(k.color) << 3);
            h ^= (std::hash<bool>{}(k.markup) << 4);
            h ^= (std::hash<bool>{}(k.bold) << 5);
            h ^= (std::hash<bool>{}(k.italic) << 6);
            h ^= (std::hash<std::string>{}(k.halign) << 7);
            return h;
        }
    };

    static std::unordered_map<Key, sk_sp<SkImage>, KeyHash>& cache() {
        static std::unordered_map<Key, sk_sp<SkImage>, KeyHash> c;
        return c;
    }

    void generate() const {
        auto* mutable_this = const_cast<TextTexture*>(this);

        if (!m_needs_update && m_image) return;

        Key key{
            m_text, m_font_family, m_halign, m_font_size, m_color, m_markup,
            m_bold, m_italic, m_underline, m_strikethrough,
            m_line_height, m_width_constraint, m_max_lines, m_shorten
        };

        auto it = cache().find(key);
        if (it != cache().end()) {
            mutable_this->m_image = it->second;
            mutable_this->m_needs_update = false;
            return;
        }

        // Parse with refs/anchors support
        auto parsed = MarkupParser::parseWithRefs(m_text);

        ParagraphStyle para_style;
        para_style.setHeight(m_line_height);

        if (m_max_lines > 0) {
            para_style.setMaxLines(m_max_lines);
            para_style.setEllipsis(u"...");
        }

        if (m_halign == "center") {
            para_style.setTextAlign(TextAlign::kCenter);
        }
        else if (m_halign == "right") {
            para_style.setTextAlign(TextAlign::kRight);
        }
        else if (m_halign == "justify") {
            para_style.setTextAlign(TextAlign::kJustify);
        }
        else {
            para_style.setTextAlign(TextAlign::kLeft);
        }

        TextStyle base;
        base.setFontSize(m_font_size);

        if (!m_font_family.empty()) {
            base.setFontFamilies({ SkString(m_font_family.c_str()) });
        }

        if (!m_markup) {
            if (m_bold || m_italic) {
                SkFontStyle::Weight w = m_bold ?
                    SkFontStyle::kBold_Weight : SkFontStyle::kNormal_Weight;
                SkFontStyle::Slant s = m_italic ?
                    SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant;
                base.setFontStyle(SkFontStyle(w, SkFontStyle::kNormal_Width, s));
            }

            if (m_underline || m_strikethrough) {
                unsigned dec = static_cast<unsigned>(TextDecoration::kNoDecoration);
                if (m_underline) {
                    dec = dec | static_cast<unsigned>(TextDecoration::kUnderline);
                }
                if (m_strikethrough) {
                    dec = dec | static_cast<unsigned>(TextDecoration::kLineThrough);
                }
                base.setDecoration(static_cast<TextDecoration>(dec));
                base.setDecorationStyle(TextDecorationStyle::kSolid);
            }
        }

        SkPaint p;
        p.setColor(m_color);
        p.setAntiAlias(true);
        base.setForegroundColor(p);

        para_style.setTextStyle(base);
        auto builder = ParagraphBuilder::make(para_style, getFontCollection());

        mutable_this->m_refs.clear();
        mutable_this->m_anchors.clear();

        if (m_markup) {
            // Use parsed segments with style info
            for (const auto& seg : parsed.segments) {
                TextStyle ts = base;

                if (seg.style.bold || seg.style.italic) {
                    SkFontStyle::Weight w = seg.style.bold ?
                        SkFontStyle::kBold_Weight : SkFontStyle::kNormal_Weight;
                    SkFontStyle::Slant s = seg.style.italic ?
                        SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant;
                    ts.setFontStyle(SkFontStyle(w, SkFontStyle::kNormal_Width, s));
                }

                if (seg.style.underline || seg.style.strikethrough) {
                    if (seg.style.underline && seg.style.strikethrough) {
                        unsigned dec = static_cast<unsigned>(TextDecoration::kUnderline) |
                            static_cast<unsigned>(TextDecoration::kLineThrough);
                        ts.setDecoration(static_cast<TextDecoration>(dec));
                    }
                    else if (seg.style.underline) {
                        ts.setDecoration(TextDecoration::kUnderline);
                    }
                    else if (seg.style.strikethrough) {
                        ts.setDecoration(TextDecoration::kLineThrough);
                    }
                    ts.setDecorationStyle(TextDecorationStyle::kSolid);
                }

                if (!seg.style.font_family.empty()) {
                    ts.setFontFamilies({ SkString(seg.style.font_family.c_str()) });
                }

                if (seg.style.font_size > 0) {
                    ts.setFontSize(seg.style.font_size);
                }

                if (seg.style.has_color) {
                    SkPaint paint;
                    paint.setColor(seg.style.color);
                    paint.setAntiAlias(true);
                    ts.setForegroundColor(paint);
                }

                builder->pushStyle(ts);
                builder->addText(seg.text.c_str(), seg.text.length());
                builder->pop();
            }
        }
        else {
            builder->addText(m_text.c_str(), m_text.length());
        }

        auto paragraph = builder->Build();
        float w = (m_width_constraint > 0) ? m_width_constraint : 100000.0f;
        paragraph->layout(w);

        mutable_this->m_width = std::ceil(paragraph->getLongestLine());
        mutable_this->m_height = std::ceil(paragraph->getHeight());

        if (m_width <= 0 || m_height <= 0) {
            mutable_this->m_image = nullptr;
            return;
        }

        // ===== PROCESS ANCHORS =====
        if (m_markup && !parsed.anchors.empty()) {
            for (const auto& anchor_pair : parsed.anchors) {
                std::string name = anchor_pair.first;
                size_t char_pos = anchor_pair.second;

                std::vector<TextBox> boxes = paragraph->getRectsForRange(
                    char_pos, char_pos + 1,
                    RectHeightStyle::kMax,
                    RectWidthStyle::kTight
                );

                if (!boxes.empty()) {
                    Anchor anch;
                    anch.name = name;
                    anch.x = boxes[0].rect.fLeft;
                    anch.y = boxes[0].rect.fTop;  // ← Mantém coordenada Skia original (Y cresce pra baixo)
                    mutable_this->m_anchors.push_back(anch);
                }
            }
        }

        // ===== PROCESS REFS =====
if (m_markup && !parsed.refs.empty()) {
    for (const auto& ref_pair : parsed.refs) {
        std::string name = ref_pair.first;
        size_t start_pos = ref_pair.second;

        // Find ref end (next ref or end of text)
        size_t end_pos = m_text.length();
        for (const auto& other : parsed.refs) {
            if (other.second > start_pos) {
                end_pos = std::min(end_pos, other.second);
            }
        }

        // Get bounding boxes for the ref range
        std::vector<TextBox> boxes = paragraph->getRectsForRange(
            start_pos, end_pos,
            RectHeightStyle::kMax,
            RectWidthStyle::kTight
        );

        // Store each box (Skia coords, Y growing downward)
        for (const auto& box : boxes) {
            RefZone zone;
            zone.name = name;
            zone.x = box.rect.fLeft;
            zone.y = box.rect.fTop;        // coord Skia
            zone.w = box.rect.fRight;      // mesma base dos anchors
            zone.h = box.rect.fBottom;     // coord Skia
            mutable_this->m_refs.push_back(zone);
        }
    }
}

        auto surface = SkSurfaces::Raster(
            SkImageInfo::MakeN32Premul(
                static_cast<int>(m_width),
                static_cast<int>(m_height)
            )
        );

        if (!surface) return;

        auto canvas = surface->getCanvas();
        canvas->clear(SK_ColorTRANSPARENT);

        // Apply vertical flip ONCE during texture generation
        canvas->save();
        canvas->translate(0, m_height);
        canvas->scale(1, -1);

        paragraph->paint(canvas, 0, 0);
        canvas->restore();

        mutable_this->m_image = surface->makeImageSnapshot();
        cache()[key] = m_image;
        mutable_this->m_needs_update = false;
    }

public:
    void setText(const std::string& t) {
        if (m_text != t) {
            m_text = t;
            m_needs_update = true;
        }
    }

    void setFontSize(int s) {
        if (m_font_size != s) {
            m_font_size = s;
            m_needs_update = true;
        }
    }

    void setFontFamily(const std::string& f) {
        if (m_font_family != f) {
            m_font_family = f;
            m_needs_update = true;
        }
    }

    void setColor(SkColor c) {
        if (m_color != c) {
            m_color = c;
            m_needs_update = true;
        }
    }

    void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        setColor(SkColorSetARGB(a, r, g, b));
    }

    void setMarkup(bool m) {
        if (m_markup != m) {
            m_markup = m;
            m_needs_update = true;
        }
    }

    void setBold(bool b) {
        if (m_bold != b) {
            m_bold = b;
            m_needs_update = true;
        }
    }

    void setItalic(bool i) {
        if (m_italic != i) {
            m_italic = i;
            m_needs_update = true;
        }
    }

    void setUnderline(bool u) {
        if (m_underline != u) {
            m_underline = u;
            m_needs_update = true;
        }
    }

    void setStrikethrough(bool s) {
        if (m_strikethrough != s) {
            m_strikethrough = s;
            m_needs_update = true;
        }
    }

    void setLineHeight(float h) {
        if (std::abs(m_line_height - h) > 0.001f) {
            m_line_height = h;
            m_needs_update = true;
        }
    }

    void setMaxLines(int l) {
        if (m_max_lines != l) {
            m_max_lines = l;
            m_needs_update = true;
        }
    }

    void setTextWidth(float w) {
        if (std::abs(m_width_constraint - w) > 0.001f) {
            m_width_constraint = w;
            m_needs_update = true;
        }
    }

    void setHAlign(const std::string& a) {
        if (m_halign != a) {
            m_halign = a;
            m_needs_update = true;
        }
    }

    void setShorten(bool s) {
        if (m_shorten != s) {
            m_shorten = s;
            m_needs_update = true;
        }
    }

    void renderAt(SkCanvas* canvas, float x, float y) {
        generate();
        if (!m_image) return;

        // Image is already flipped, just draw it directly
        // Adjust y to account for text height (Kivy uses bottom-left origin)
        canvas->drawImage(m_image, x, y - m_height, SkSamplingOptions(SkFilterMode::kLinear));
    }

    float getWidth() const {
        generate();
        return m_width;
    }

    float getHeight() const {
        generate();
        return m_height;
    }

    std::vector<RefZone> getRefs() const {
        generate();
        return m_refs;
    }

    std::vector<Anchor> getAnchors() const {
        generate();
        return m_anchors;
    }

    static void clearCache() {
        cache().clear();
    }

    static size_t getCacheSize() {
        return cache().size();
    }
};
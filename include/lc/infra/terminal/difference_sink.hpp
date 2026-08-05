#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <lc/infra/diff/script.hpp>
#include <lc/infra/terminal/display_width.hpp>
#include <lc/infra/terminal/style.hpp>

namespace lc::infra::terminal::detail {

inline void append_styled(std::string& output,
                          style value,
                          std::string_view text)
{
    output.append(escape(value));
    output.append(text);
    output.append("\x1b[0m");
}

struct rendered_line {
    std::string text;
    std::size_t width = 0;
    std::optional<diff::operation> open_change;
};

struct render_frame {
    rendered_line actual;
    rendered_line expected;
};

class difference_sink {
public:
    explicit difference_sink(bool colors)
        : colors_(colors), frames_(1)
    {}

    void append_both(std::string_view text)
    {
        const std::size_t width = visible_width(text);
        append_raw(frames_.back().actual, text, width);
        append_raw(frames_.back().expected, text, width);
    }

    void append_actual(diff::operation kind, std::string_view text)
    {
        append_changed(frames_.back().actual, kind, text);
    }

    void append_expected(diff::operation kind, std::string_view text)
    {
        append_changed(frames_.back().expected, kind, text);
    }

    void begin_cell()
    {
        frames_.emplace_back();
    }

    void end_cell()
    {
        render_frame cell = std::move(frames_.back());
        frames_.pop_back();
        close_change(cell.actual);
        close_change(cell.expected);
        const std::size_t width =
            std::max(cell.actual.width, cell.expected.width);
        cell.actual.text.append(width - cell.actual.width, ' ');
        cell.expected.text.append(width - cell.expected.width, ' ');
        append_raw(frames_.back().actual, cell.actual.text, width);
        append_raw(frames_.back().expected, cell.expected.text, width);
    }

    std::pair<std::string, std::string> finish() &&
    {
        close_change(frames_.front().actual);
        close_change(frames_.front().expected);
        return {std::move(frames_.front().actual.text),
                std::move(frames_.front().expected.text)};
    }

private:
    void close_change(rendered_line& line) const
    {
        if (!line.open_change) return;
        if (colors_) {
            line.text.append("\x1b[0m");
        } else if (*line.open_change == diff::operation::removed) {
            line.text.append("-]");
            line.width += 2;
        } else {
            line.text.append("+}");
            line.width += 2;
        }
        line.open_change.reset();
    }

    void open_change(rendered_line& line, diff::operation kind) const
    {
        if (line.open_change == kind) return;
        close_change(line);
        line.open_change = kind;
        if (colors_) {
            line.text.append(escape(
                kind == diff::operation::removed
                    ? style::failure
                    : style::success));
        } else if (kind == diff::operation::removed) {
            line.text.append("[-");
            line.width += 2;
        } else {
            line.text.append("{+");
            line.width += 2;
        }
    }

    void append_raw(rendered_line& line,
                    std::string_view text,
                    std::size_t width) const
    {
        close_change(line);
        line.text.append(text);
        line.width += width;
    }

    void append_changed(rendered_line& line,
                        diff::operation kind,
                        std::string_view text) const
    {
        open_change(line, kind);
        line.text.append(text);
        line.width += visible_width(text);
    }

    bool colors_;
    std::vector<render_frame> frames_;
};

} // namespace lc::infra::terminal::detail

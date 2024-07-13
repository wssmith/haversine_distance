#ifndef WS_SCOPEDINDENT_HPP
#define WS_SCOPEDINDENT_HPP

#include <ios>
#include <ostream>
#include <streambuf>
#include <string>

class scoped_indent final : public std::streambuf
{
public:
    explicit scoped_indent(std::ostream& dest, int indent = 2)
        : m_indent( indent, ' ' )
        , m_dest{ dest.rdbuf() }
        , m_owner{ &dest }
    {
        m_owner->rdbuf(this);
    }

    virtual ~scoped_indent() override
    {
        if (m_owner != nullptr)
            m_owner->rdbuf(m_dest);
    }

    scoped_indent(const scoped_indent&) = delete;
    scoped_indent& operator=(const scoped_indent&) = delete;
    scoped_indent(scoped_indent&&) noexcept = delete;
    scoped_indent& operator=(scoped_indent&&) noexcept = delete;

protected:
    virtual int overflow(int ch) override
    {
        const bool found_new_line = (ch == '\n');

        if (m_at_start_of_line && !found_new_line)
        {
            m_dest->sputn(m_indent.data(), static_cast<std::streamsize>(m_indent.size()));
        }

        m_at_start_of_line = found_new_line;

        return m_dest->sputc(static_cast<char>(ch));
    }

private:
    std::string m_indent;
    std::streambuf* m_dest{ nullptr };
    std::ostream* m_owner{ nullptr };
    bool m_at_start_of_line{ true };
};

#endif

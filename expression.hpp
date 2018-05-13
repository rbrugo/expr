/**
 * @author      : Riccardo Brugo (brugo.riccardo@gmail.com)
 * @file        : expression
 * @created     : Wednesday May 09, 2018 12:40:00 CEST
 * @license     : MIT
 * */

#ifndef EXPRESSION_HPP
#define EXPRESSION_HPP

#include <map>
#include <memory>
#include <variant>
#include <functional>

namespace expr
{

using nothing  = std::monostate;
using const_t  = double;
using unary_f  = std::function<const_t(const_t)>;
using binary_f = std::function<const_t(const_t,const_t)>;
using param_t  = char;
//struct param_t
//{
    //explicit param_t(char n) : name{n}, coefficient{1}, exponent{1} { };
    //char name;
    //const_t coefficient;
    //const_t exponent;
//};

using variant_t = std::variant<nothing, const_t, param_t, unary_f, binary_f>;

struct node
{
    template <
        typename T,
        typename = std::enable_if <
            std::is_constructible_v<variant_t, std::decay_t<T> > &&
            not std::is_same_v<std::decay_t<T>, node>
            , void
        >
    >
    explicit node(T && src) : content{src} {}

    variant_t content;
    std::shared_ptr<node> left;
    std::shared_ptr<node> right;
};

class expression
{
    std::shared_ptr<node> _head;
    std::map<char, const_t> _dictionary;

public:
    enum class policy { build, optimize, };

    explicit expression(std::string && source);
    explicit expression(std::string const & source);
    explicit expression(policy p, std::string && source);
    explicit expression(policy p, std::string const & source);

    expression & build(std::string const & src);
    expression & build(std::string && src);
    expression & build(policy p, std::string const & src);
    expression & build(policy p, std::string && src);
    expression & optimize();
    std::optional<const_t> eval() const;
    std::optional<const_t> eval(char x, const_t const & value) const;

    expression & set_param(char name, const_t const & value);

    std::optional<std::function<const_t(const_t)>> as_unary(char x = 'x') const &;
    std::optional<std::function<const_t(const_t)>> as_unary(char x = 'x') &&;
    explicit operator bool() const { return _head != nullptr; }
private:
    std::vector<variant_t> parse(std::string && src);
    std::string preparse(std::string && src) const noexcept;
    std::vector<variant_t> parse_impl(std::string_view src);
    expression & build_impl(std::string const & src);
    expression & build_impl(std::string && src);
    void optimize_impl(std::shared_ptr<node> & head);
    const_t eval_impl(std::shared_ptr<node> const & head) const;
    const_t eval_impl(std::shared_ptr<node> const & head, char x, const_t const & value) const;

};

inline auto compute(std::string const & source)
    -> std::optional<const_t>
{
    expression expr{source};
    return expr.eval();
}

inline auto compute(std::string && source)
    -> std::optional<const_t>
{
    expression expr{std::move(source)};
    return expr.eval();
}

inline auto compute(std::string const & source, char x, const_t const & value)
    -> std::optional<const_t>
{
    expression expr{source};
    return expr.eval(x, value);
}

inline auto compute(std::string && source, char x, const_t const & value)
    -> std::optional<const_t>
{
    expression expr{std::move(source)};
    return expr.eval(x, value);
}
inline auto parse_function( std::string const & source, char x, expression::policy p)
    -> std::optional<std::function<const_t(const_t)>>
{
    expression expr{p, source};
    return std::move(expr).as_unary(x);
}

inline auto parse_function( std::string &&   source, char x, expression::policy p)
    -> std::optional<std::function<const_t(const_t)>>
{
    expression expr{p, std::move(source)};
    return std::move(expr).as_unary(x);
}



} // namespace expr

#endif /* EXPRESSION_HPP */


/**
 * @author      : Riccardo Brugo (brugo.riccardo@gmail.com)
 * @file        : expression
 * @created     : Wednesday May 09, 2018 16:29:51 CEST
 * @license     : MIT
 * */

#include <regex>
#include <cmath>
#include <cctype>
#include "expression.hpp"


namespace expr
{

namespace detail
{
    template <class... Args> struct overload : Args... { using Args::operator()...; };
    template <class... Args> overload(Args...) -> overload<Args...>;

    bool evalutable(std::shared_ptr<node> const & node)
    {
        if ( ! node ) {
            return false;
        }
        if ( std::holds_alternative<const_t>(node->content) ) {
            return true;
        }
        if ( std::holds_alternative<binary_f>(node->content) ) {
            return evalutable(node->left) && evalutable(node->right);
        }
        if ( std::holds_alternative<unary_f>(node->content) ) {
            return evalutable(node->left);
        }

        return false;
    }

    auto inline sign_priority(char x) noexcept
    {
        x = std::tolower(x);
        if ( x == '+' || x =='-' ) {
            return 0;
        }
        if ( x == '*' || x == '/' ) {
            return 1;
        }
        if ( x == '^' ) {
            return 2;
        }
        if ( x == 's' || x == 'c' || x == 't' || x == 'e' || x == 'l' || x == 'a' || x == 'v') {
            return 3;
        }
        return -1;
    }

    inline bool stronger_sign(char A, char B) noexcept
    {
        return sign_priority(A) > sign_priority(B);
    }

    inline bool is_binary_f(char ch)
    {
        return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^' || ch == '%';
    }

    namespace function
    {
        auto constexpr plus       = [](const_t const & a, const_t const & b) { return a + b; };
        auto constexpr minus      = [](const_t const & a, const_t const & b) { return a - b; };
        auto constexpr multiplies = [](const_t const & a, const_t const & b) { return a * b; };
        auto constexpr divides    = [](const_t const & a, const_t const & b) { return a / b; };
        auto constexpr modulus    = [](const_t const & a, const_t const & b) { return static_cast<long>(a) % static_cast<long>(b); };
        auto constexpr pow        = [](const_t const & a, const_t const & b) { return std::pow(a,b); };

        auto constexpr sin        = [](const_t const & a) { return std::sin(a); };
        auto constexpr cos        = [](const_t const & a) { return std::cos(a); };
        auto constexpr tan        = [](const_t const & a) { return std::tan(a); };
        auto constexpr asin       = [](const_t const & a) { return std::asin(a); };
        auto constexpr acos       = [](const_t const & a) { return std::acos(a); };
        auto constexpr atan       = [](const_t const & a) { return std::atan(a); };
        auto constexpr exp        = [](const_t const & a) { return std::exp(a); };
        auto constexpr ln         = [](const_t const & a) { return std::log(a); };
        auto constexpr abs        = [](const_t const & a) { return std::abs(a); };
        auto constexpr sqrt       = [](const_t const & a) { return std::sqrt(a); };
        auto constexpr cbrt       = [](const_t const & a) { return std::cbrt(a); };
        //sinh
        //cosh
        //tanh
        //asinh
        //acosh
        //atanh
        //ceil
        //floor
        //log10
    } // namespace function

    inline std::function<double(double, double)> sign_to_binary(char ch)
    {
        switch (ch) {
            case '+':
                return function::plus;
            case '-':
                return function::minus;
            case '*':
                return function::multiplies;
            case '/':
                return function::divides;
            case '^':
                return function::pow;
            case '%':
                return function::modulus;
            default:
                std::string text = "Fount bad operator without correspective function: ";
                text.push_back(ch);
                throw std::logic_error{std::move(text)};
        }
    }

    inline std::function<double(double)> sign_to_unary(char ch)
    {
        switch (ch) {
            case 's':
                return function::sin;
            case 'c':
                return function::cos;
            case 't':
                return function::tan;
            case 'S':
                return function::asin;
            case 'C':
                return function::acos;
            case 'T':
                return function::atan;
            case 'l':
                return function::ln;
            case 'e':
                return function::exp;
            case '|':
                return function::abs;
            case 'v':
                return function::sqrt;
            case 'V':
                return function::cbrt;
            default:
                std::string error = "Found bad operator without correspective function: ";
                error.push_back(ch);
                throw std::logic_error{std::move(error)};
        }
    }
} // namespace detail

namespace regex
{
    inline std::regex const real  { "^(\\d)+(\\.(\\d)*)?([Ee](\\+|-)?(\\d)+)?" };
    inline std::regex const oper  { "^[+\\-*/^%]" };
    inline std::regex const valid { "((\\d)+(\\.(\\d)*)?([Ee](\\+|-)?(\\d)+)?)|([+\\-*/^%])" };
    inline std::regex const sin   { "^sin" };
    inline std::regex const cos   { "^cos" };
    inline std::regex const tan   { "^t(an|g)" };
    inline std::regex const asin  { "^asin" };
    inline std::regex const acos  { "^acos" };
    inline std::regex const atan  { "^at(an|g)" };
    inline std::regex const ln    { "^ln" };
    inline std::regex const exp   { "^exp" };
    inline std::regex const abs   { "^abs" };
    inline std::regex const sqrt  { "^sqrt" };
    inline std::regex const cbrt  { "^cbrt" };
    inline std::regex const fun   { "^((a?sin)|(a?cos)|(a?t(an|g))|(ln)|(exp)|(abs)|((sq|cb)rt))" };
    //inline std::regex const log   { "log" };
} // namespace regex

expression::expression(std::string const & source) :
    expression{ expression::policy::build, std::string{source} }
{ ; }

expression::expression(expression::policy p, std::string const & source) :
    expression{ p, std::string{source} }
{ ; }

expression::expression(std::string && source) : expression(expression::policy::build, std::move(source))
{ ; }

expression::expression(expression::policy p, std::string && source)
{
    this->build(p, std::move(source));
    //if ( p == expression::policy::optimize ) {
        //this->optimize();
    //}
}

std::vector<variant_t> expression::parse(std::string && src)
{
    if ( src.empty() ) {
        return { const_t{0} };
    }

    auto preparsed = this->preparse( std::move(src) );
    return this->parse_impl( preparsed );
}

std::string expression::preparse(std::string && src) const noexcept
{
    for ( std::size_t i = src.find('(', 1); i < std::string::npos; i = src.find('(', i + 1) ) {
        if ( src.at(i-1) == ')' || static_cast<bool>(std::isdigit(src.at(i-1))) ) {
            src.insert(i, 1, '*');
        }
    }
    return src;
}

std::vector<variant_t> expression::parse_impl(std::string_view line)
{
    std::vector<variant_t> buffer;
    std::vector<char> sign_buffer;

    // Insert a leading 0 if the expression starts with a sign
    if ( detail::is_binary_f(line[0]) ) {
        buffer.emplace_back(0.);
    }

    while ( ! line.empty() ) {
        auto begin = line.cbegin();
        auto end   = line.cend();
        std::match_results<decltype(begin)> match;

        // Match a real number
        if ( std::regex_search(begin, end, match, regex::real) ) {
            auto const & real = match[0].str();
            line.remove_prefix( real.size() );
            buffer.emplace_back( std::stod(real) );
        }
        // Match an operator
        else if ( detail::is_binary_f(*begin) ) {
            auto operation = *begin;
            while ( ! sign_buffer.empty() && ! detail::stronger_sign(operation, sign_buffer.back() ) ) {
                if ( detail::is_binary_f( sign_buffer.back() ) ) {
                    buffer.emplace_back( detail::sign_to_binary( sign_buffer.back() ) );
                }
                else {
                    buffer.emplace_back( detail::sign_to_unary( sign_buffer.back() ) );
                }

                sign_buffer.pop_back();
            }
            sign_buffer.emplace_back(operation);
            line.remove_prefix(1);
        }
        // Match an open parenthesis
        else if ( *begin == '(' ) {
            int counter  = 1;
            size_t index = 1;

            while ( index < line.size() ) {
                if ( line.at(index) == '(' ) {
                    ++counter;
                }
                else if ( line.at(index) == ')' ) {
                    --counter;
                }
                if ( counter == 0 ) {
                    break;
                }
                ++index;
            }

            if ( index == line.size() ) {
                throw std::logic_error{"Unterminated parenthesis"};
            }
            if ( index > 1 ) {
                for ( auto & symbol : parse_impl( line.substr(1, index - 1) ) ) {
                    buffer.emplace_back( std::move(symbol) );
                }
            }
            line.remove_prefix(index + 1);
        }
        // Match a closed parenthesis - error
        else if ( *begin == ')' ) {
            throw std::logic_error{"Closed parentheses without an opening correspective"};
        }
        // Match a space
        else if ( static_cast<bool>(std::isspace(*begin)) ) {
            line.remove_prefix(1);
        }
        // Match a function
        else if ( std::regex_search(begin, end, match, regex::fun) ) {

            char operation;
            switch (*begin) {
                case 'a':
                    if ( *(begin + 1) == 'b' ) { operation = '|'; }
                    else { operation = std::toupper(*(begin + 1)); }
                    break;
                case 's':
                    if ( *(begin + 1) == 'q' ) { operation = 'v'; }
                    else { operation = 's'; }
                    break;
                case 'c':
                    if ( *(begin + 1) == 'b' ) { operation = 'V'; }
                    else { operation = 'c'; }
                    break;
                default:
                    operation = *begin;
            }
            sign_buffer.push_back(operation);

            line.remove_prefix(match[0].length());
        }
        // Match something else
        else {
            if ( std::toupper(*begin) == 'P' && std::toupper(*(begin + 1)) == 'I' ) {
                buffer.emplace_back( const_t{3.141592653589793} );
                line.remove_prefix(2);
                continue;
            }
            if ( *begin == 'e' ) {
                buffer.emplace_back( const_t{ std::exp(1) } );
                line.remove_prefix(1);
                continue;
            }

            auto invalid_token = [&]() {
                if ( std::regex_search(begin, end, match, regex::valid) ) {
                    return std::string{match.prefix()};
                }
                else {
                    return std::string{line.data(), line.size()};
                }
            }();

            if ( invalid_token.size() > 1 ) {
                throw std::invalid_argument{
                    invalid_token + " Unexpected token in parsing (name parameters must be 1 char long)"
                };
            }
            buffer.emplace_back( param_t{ invalid_token.at(0) } );
            line.remove_prefix(1);
        }
    }

    std::for_each(sign_buffer.rbegin(), sign_buffer.rend(), [&](auto token) {
            if ( detail::is_binary_f(token) ) {
                buffer.emplace_back( detail::sign_to_binary(token) );
            }
            else {
                buffer.emplace_back( detail::sign_to_unary(token) );
            }
    });

    if ( buffer.empty() ) {
        buffer.emplace_back( const_t{0} );
    }
    return buffer;
}

expression & expression::build(std::string const & src)
{
    return this->build_impl(std::string{src});
}

expression & expression::build(std::string && src)
{
    return this->build_impl(std::move(src));
}

expression & expression::build(expression::policy p, std::string const & src)
{
    return expression::build(p, std::string{src});
}

expression & expression::build(expression::policy p, std::string && src)
{
    this->build_impl(std::move(src));
    if ( p == expression::policy::optimize ) {
        this->optimize();
    }
    return *this;
}

expression & expression::build_impl(std::string && src)
{
    auto symbols = parse(std::move(src));
    auto it      = symbols.crbegin();
    auto end     = symbols.crend();

    _head = std::make_shared<node>( *it );
    if ( std::holds_alternative<const_t>(*it) ) {
        if ( symbols.size() > 1 ) {
            throw std::logic_error{"Bad parsing or semantics"};
        }
        return *this;
    }
    else if ( std::holds_alternative<unary_f>(*it) || std::holds_alternative<binary_f>(*it) ) {
        if ( symbols.size() == 1 ) {
            throw std::logic_error{"Function or operator without arguments"};
        }
    }
    ++it;

    std::stack<decltype(_head.get())> stack;
    stack.push(_head.get());

    while ( it != end ) {
        auto & top = stack.top();
        std::shared_ptr<node>  * ptr;

        if ( ! top->left ) {
            ptr = &top->left;
        }
        else if ( ! top->right ) {
            ptr = &top->right;
        }
        else {
            stack.pop();
            continue;
        }
        *ptr = std::make_shared<node>(*it);

        if ( std::holds_alternative<unary_f>(*it) ) {
            stack.push( ptr->get() );
            stack.top()->right = std::make_shared<node>(nothing{});
        }
        else if ( std::holds_alternative<binary_f>(*it) ) {
            stack.push( ptr->get() );
        }
        ++it;
    }

    return *this;
}

expression & expression::optimize()
{
    if ( _head ) {
        this->optimize_impl(_head);
    }
    return *this;
}

void expression::optimize_impl(std::shared_ptr<node> & node)
{
    //Optimization for binary
    if ( std::holds_alternative<binary_f>(node->content) ) {
        optimize_impl(node->left);
        optimize_impl(node->right);
        //Optimize if the content is without parameters
        if ( detail::evalutable(node) ) {
            node->content = const_t{eval_impl(node)};
        }
        //Compose [f]->[g->[x,y],h->[z,w]] into [f.°h->[z,w]]->g[x,y] and then reiterate as unary
        else
        {
            if ( std::holds_alternative<unary_f>(node->right->content) )
            {
                auto left  = std::move(node->left);
                auto right = std::move(node->right);
                auto new_function =
                [
                    f = std::move(std::get<binary_f>(node->content)),
                    g = std::move(std::get<unary_f>(right->content))
                ]
                (const_t const & a, const_t const & b) {
                    return f(a, g(b));
                };
                node = std::make_shared<expr::node>(std::move(new_function));
                node->left  = std::move(left);
                node->right = std::move(right->left);
            }
            if ( std::holds_alternative<unary_f>(node->left->content) )
            {
                auto left  = std::move(node->left);
                auto right = std::move(node->right);
                auto new_function =
                [
                    f = std::move(std::get<binary_f>(node->content)),
                    g = std::move(std::get<unary_f> (left->content))
                ]
                (const_t const & a, const_t const & b) {
                    return f(g(a), b);
                };
                node = std::make_shared<expr::node>(std::move(new_function));
                node->left  = std::move(left->left); //std::move(left);
                node->right = std::move(right); //std::move(right->left);
            }
        }
    }
    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
    //Optimization for unary
    else if ( std::holds_alternative<unary_f>(node->content) ) {
        optimize_impl(node->left);
        //Optimize if the content is without parameters
        if ( detail::evalutable(node->left) ) {
            node->content = const_t{eval_impl(node)};
        }
        else {
            //Compose [f]->[g]->[x] into [f°g]->[x]
            if ( std::holds_alternative<unary_f>(node->left->content) ) {
                auto tmp = std::move(node->left);
                auto new_function =
                [
                    f = std::get<unary_f>(node->content),
                    g = std::get<unary_f>( tmp->content)
                ]
                (const_t const & a) {
                    return f(g(a));
                };
                node = std::make_shared<expr::node>(std::move(new_function));
                node->left = std::move(tmp->left);
            }
            //Compose [f]->[g]->[x,y] into [f°g]->[x,y]
            else if ( std::holds_alternative<binary_f>(node->left->content) ) {
                auto tmp = std::move(node->left);
                auto new_function = binary_f{
                    [
                        f = std::get<unary_f>(node->content),
                        g = std::get<binary_f>( tmp->content)
                    ]
                    (const_t const & a, const_t const & b) {
                        return f(g(a, b));
                    }
                };
                node = std::make_shared<expr::node>(std::move(new_function));
                node->left  = std::move(tmp->left);
                node->right = std::move(tmp->right);
            }
        }
    }
}

std::optional<const_t> expression::eval() const
{
    if ( ! _head ) { return {}; }
    return this->eval_impl(_head);
}

std::optional<const_t> expression::eval(char ch, const_t const & value) const
{
    if ( ! _head ) { return {}; }
    return this->eval_impl(_head, ch, value);
}

const_t expression::eval_impl(std::shared_ptr<node> const & head) const
{
    return std::visit(
            detail::overload{
                [ ](const_t const & value) {
                    return value;
                },
                [&](param_t const & param) {
                    //if ( _dictionary.find(param.name) == _dictionary.end() ) {
                        //throw std::logic_error{std::string{"Unassigned parameter "} + param.name};
                    //}
                    //return std::pow(_dictionary.at(param.name), param.exponent) * param.coefficient;
                    auto it = _dictionary.find(param);
                    if ( it == _dictionary.end() ) {
                        throw std::logic_error{std::string{"Unassigned parameter "} + param};
                    }
                    return it->second;
                },
                [&](unary_f const & unary) {
                    return unary( eval_impl(head->left) );
                },
                [&](binary_f const & binary) {
                    return binary( eval_impl(head->right), eval_impl(head->left) );
                },
                [ ](nothing) {
                    throw std::logic_error{"Found (literally) nothing..."};
                    return const_t{0};
                }
            }, head->content
    );
}

const_t expression::eval_impl(std::shared_ptr<node> const & head, char x, const_t const & value) const
{
    return std::visit(
            detail::overload{
                [ ](const_t const & value) {
                    return value;
                },
                [&](param_t const & param) {
                    //if ( param.name == x ) {
                        //return std::pow(value, param.exponent) * param.coefficient;
                    if ( param == x ) {
                        return value;
                    }
                    //if ( _dictionary.find(param.name) == _dictionary.end() ) {
                    if ( auto it = _dictionary.find(param); it == _dictionary.end() ) {
                        //throw std::logic_error{std::string{"Unassigned parameter "} + param.name };
                        throw std::logic_error{std::string{"Unassigned parameter "} + param };
                    }
                    //return std::pow(_dictionary.at(param.name), param.exponent) * param.coefficient;
                    else {
                        return it->second;
                    }
                },
                [&](unary_f const & unary) {
                    return unary( eval_impl(head->left, x, value) );
                },
                [&](binary_f const & binary) {
                    return binary( eval_impl(head->right, x, value), eval_impl(head->left, x, value) );
                },
                [ ](nothing) {
                    throw std::logic_error{"Found (literally) nothing..."};
                    return const_t{0};
                }
            }, head->content
    );
}

expression & expression::set_param(char name, const_t const & value)
{
    _dictionary.insert_or_assign(name, value);
    return *this;
}

std::optional<std::function<const_t(const_t)>> expression::as_unary(char ch) const &
{
    using result_t = std::optional<std::function<const_t(const_t)>>;
return (_head == nullptr)
        ? result_t{std::nullopt}
        : [f=*this,ch](const_t const & x) {
        return f.eval(ch, x).value();
    };
}

std::optional<std::function<const_t(const_t)>> expression::as_unary(char ch) &&
{
    using result_t = std::optional<std::function<const_t(const_t)>>;
return (_head == nullptr)
        ? result_t{std::nullopt}
        : [f=std::move(*this),ch](const_t const & x) {
        return f.eval(ch, x).value();
    };
}

} // namespace expr

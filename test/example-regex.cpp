#include <josa/visitor.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace std::string_literals;

//--------------------------------------------------------------------------------------------------
//
//  This example code is based on the paper "Regular-expression derivatives reexamined" by
//  Owens, Reppy and Turon.
//
//  Link: https://www.khoury.northeastern.edu/home/turon/re-deriv.pdf
//
//  We will use the following syntax (for parsing or printing regular expressions); in order of
//  precedence:
//
//  a       an element of the regular expression alphabet - see isValidChar function
//  #       empty set
//  ()      empty string
//  (r)     r parenthesized for precedence
//  ~r      complement of r
//  r*      Kleene closure of r
//  rs      Concatenation of r and s
//  x&y     Intersection (logical and) of x and y
//  x|y     Union (logical or) of x and y
//
//  The use of Josa.Visitor is demonstrated in:
//
//      getPrecedence (match function)
//      RegexToString (struct with enable_dispatch)
//      RegexClone (struct with enable_dispatch)
//      RegexNullable (struct with enable_dispatch)
//      RegexDerivative (struct with enable_dispatch)
//
//--------------------------------------------------------------------------------------------------


//  This will define the alphabet for our regular expressions:
//
auto isValidChar(const char ch) -> bool
{
    return ch >= 'a' && ch <= 'z'
        || ch >= 'A' && ch <= 'Z'
        || ch >= '0' && ch <= '9';
}

//--------------------------------------------------------------------------------------------------
//
//  Regular expression AST
//
//--------------------------------------------------------------------------------------------------

class RegexExpr
{
public:

    virtual ~RegexExpr() = default;

protected:

    RegexExpr() = default;
};

using RegexExprPtr = std::unique_ptr<RegexExpr>;

class UnaryOp : public RegexExpr
{
public:

    explicit UnaryOp(RegexExprPtr pExpr)
        :   pExpr_{std::move(pExpr)}
    {
        if (!pExpr_)
            throw std::runtime_error{"nullptr in UnaryOp constructor"};
    }

    auto expr() const -> const RegexExpr&
    {
        return *pExpr_;
    }

private:

    RegexExprPtr pExpr_;
};

class BinaryOp : public RegexExpr
{
public:

    BinaryOp(RegexExprPtr pExpr1, RegexExprPtr pExpr2)
        :   pExpr1_{std::move(pExpr1)}, pExpr2_{std::move(pExpr2)}
    {
        if (!pExpr1_ || !pExpr2_)
            throw std::runtime_error{"nullptr in BinaryOp constructor"};
    }

    auto expr1() const -> const RegexExpr&
    {
        return *pExpr1_;
    }

    auto expr2() const -> const RegexExpr&
    {
        return *pExpr2_;
    }

private:

    RegexExprPtr pExpr1_;
    RegexExprPtr pExpr2_;
};

class Concatenation final : public BinaryOp
{
public:

    using BinaryOp::BinaryOp;
};

class KleeneStar final : public UnaryOp
{
public:

    using UnaryOp::UnaryOp;
};

class Complement final : public UnaryOp
{
public:

    using UnaryOp::UnaryOp;
};

class Intersection final : public BinaryOp
{
public:

    using BinaryOp::BinaryOp;
};

class Union final : public BinaryOp
{
public:

    using BinaryOp::BinaryOp;
};

class EmptySet final : public RegexExpr
{
public:

    EmptySet() = default;
};

class EmptyString final : public RegexExpr
{
public:

    EmptyString() = default;
};

class Character final : public RegexExpr
{
public:

    explicit Character(const char ch)
        :   ch_{ch}
    {
        if (!isValidChar(ch))
            throw std::runtime_error{"Character out of range"};
    }

    auto get() const -> char
    {
        return ch_;
    }

private:

    char ch_;
};

//
//  Some helper functions to create Regex AST objects, and eliminate some basic
//  redundencies, such as concatenating empty strings.
//

auto makeEmptySet() -> RegexExprPtr
{
    return std::make_unique<EmptySet>();
}

auto makeEmptyString() -> RegexExprPtr
{
    return std::make_unique<EmptyString>();
}

auto makeConcatenation(RegexExprPtr pExpr1, RegexExprPtr pExpr2) -> RegexExprPtr
{
    if (dynamic_cast<EmptySet*>(pExpr1.get()) || dynamic_cast<EmptySet*>(pExpr2.get()))
        return makeEmptySet();

    if (dynamic_cast<EmptyString*>(pExpr1.get()))
        return pExpr2;

    if (dynamic_cast<EmptyString*>(pExpr2.get()))
        return pExpr1;

    return std::make_unique<Concatenation>(std::move(pExpr1), std::move(pExpr2));
}

auto makeUnion(RegexExprPtr pExpr1, RegexExprPtr pExpr2) -> RegexExprPtr
{
    if (dynamic_cast<EmptySet*>(pExpr1.get()))
        return pExpr2;

    if (dynamic_cast<EmptySet*>(pExpr2.get()))
        return pExpr1;

    return std::make_unique<Union>(std::move(pExpr1), std::move(pExpr2));
}

auto makeIntersection(RegexExprPtr pExpr1, RegexExprPtr pExpr2) -> RegexExprPtr
{
    if (dynamic_cast<EmptySet*>(pExpr1.get()) || dynamic_cast<EmptySet*>(pExpr2.get()))
        return makeEmptySet();

    return std::make_unique<Intersection>(std::move(pExpr1), std::move(pExpr2));
}

auto makeKleeneStar(RegexExprPtr pExpr) -> RegexExprPtr
{
    if (dynamic_cast<EmptyString*>(pExpr.get()))
        return makeEmptyString();

    if (dynamic_cast<KleeneStar*>(pExpr.get()))
        return pExpr;

    return std::make_unique<KleeneStar>(std::move(pExpr));
}

auto makeComplement(RegexExprPtr pExpr) -> RegexExprPtr
{
    return std::make_unique<Complement>(std::move(pExpr));
}

auto makeCharacter(const char c) -> RegexExprPtr
{
    return std::make_unique<Character>(c);
}

//  End of regular expression AST
//--------------------------------------------------------------------------------------------------


//  This type is used to inform Josa.Visitor about classes in the regular expression AST. Note that
//  only includes the ultimate base class and concrete classes, not intermediate classes (UnaryOp
//  and BinaryOp).
//
using RegexHierarchy = josa::visitor::hierarchy<
    josa::visitor::base_type<RegexExpr>,
    josa::visitor::concrete_types<Concatenation, Union, Intersection, EmptySet, EmptyString, Character, KleeneStar, Complement>
>;

//--------------------------------------------------------------------------------------------------

struct RegexSyntaxError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

//  RegexParser is a basic recursive descent parser to convert a regular expression string, e.g. 
//  "(a|b)*c", to a regular expression AST.
//
class RegexParser
{
public:

    static auto parse(std::string_view s) -> RegexExprPtr
    {
        if (s.empty())
            return makeEmptyString();

        auto [s2, pExpr2] = parseExpr(s);

        if (!s2.empty())
            throw RegexSyntaxError{"unexpected character '"s + s2.front() + "'"s};

        return std::move(pExpr2);
    }

private:

    static auto parseExpr(const std::string_view s) -> std::pair<std::string_view, RegexExprPtr>
    {
        if (s.empty() || s.front() == ')')
            return {s, makeEmptyString()};

        return parseUnion(s);
    }

    static auto parseUnion(const std::string_view s) -> std::pair<std::string_view, RegexExprPtr>
    {
        if (s.empty())
            throw RegexSyntaxError{"unexpected end of regex string"};

        auto [s2, pExpr] = parseIntersection(s);

        if (!s2.empty() && s2.front() == '|')
        {
            auto [s3, pExpr2] = parseUnion(s2.substr(1));
            return {s3, makeUnion(std::move(pExpr), std::move(pExpr2))};
        }

        return {s2, std::move(pExpr)};
    }

    static auto parseIntersection(std::string_view s) -> std::pair<std::string_view, RegexExprPtr>
    {
        if (s.empty())
            throw RegexSyntaxError{"unexpected end of regex string"};

        auto [s2, pExpr2] = parseConcatenation(s);

        if (!s2.empty() && s2.front() == '&')
        {
            auto [s3, pExpr3] = parseIntersection(s2.substr(1));
            return {s3, makeIntersection(std::move(pExpr2), std::move(pExpr3))};
        }

        return {s2, std::move(pExpr2)};
    }

    static auto parseConcatenation(const std::string_view s) -> std::pair<std::string_view, RegexExprPtr>
    {
        if (s.empty())
            throw RegexSyntaxError{"unexpected end of regex string"};

        auto [s2, pExpr] = parseKleeneStar(s);

        if (!s2.empty() && s2.front() != ')' && s2.front() != '&' && s2.front() != '|')
        {
            auto [s3, pExpr2] = parseConcatenation(s2);
            return {s3, makeConcatenation(std::move(pExpr), std::move(pExpr2))};
        }

        return {s2, std::move(pExpr)};
    }

    static auto parseKleeneStar(const std::string_view s) -> std::pair<std::string_view, RegexExprPtr>
    {
        if (s.empty())
            throw RegexSyntaxError{"unexpected end of regex string"};

        auto [s2, pExpr2] = parseComplement(s);

        if (!s2.empty() && s2.front() == '*')
        {
            while (!s2.empty() && s2.front() == '*')
                s2 = s2.substr(1);

            return {s2, makeKleeneStar(std::move(pExpr2))};
        }

        return {s2, std::move(pExpr2)};
    }

    static auto parseComplement(std::string_view s) -> std::pair<std::string_view, RegexExprPtr>
    {
        if (s.empty())
            throw RegexSyntaxError{"unexpected end of regex string"};

        if (s.front() == '~')
        {
            auto [s2, pExpr2] = parseComplement(s.substr(1));
            return {s2, makeComplement(std::move(pExpr2))};
        }

        return parseAtomic(s);
    }

    static auto parseAtomic(std::string_view s) -> std::pair<std::string_view, RegexExprPtr>
    {
        if (s.empty())
            throw RegexSyntaxError{"unexpected end of regex string"};

        if (s.front() == '(')
        {
            auto [s2, pExpr2] = parseExpr(s.substr(1));

            if (s2.empty() || s2.front() != ')')
                throw RegexSyntaxError{"missing closing parenthesis"};

            return {s2.substr(1), std::move(pExpr2)};
        }

        if (s.front() == '#')
            return {s.substr(1), makeEmptySet()};

        if (isValidChar(s.front()))
            return {s.substr(1), makeCharacter(s.front())};

        throw RegexSyntaxError{"unexpected character '"s + s.front() + "'"s};
    }
};

auto operator ""_rx(const char* s, size_t len) -> RegexExprPtr
{
    return RegexParser::parse(s);
}

//--------------------------------------------------------------------------------------------------

//  Function to get the precedence level of various regular expression operators so they can be
//  parenthesized correctly in the RegexToString functions. This demonstrates the use of the
//  Josa.Visitor match function and a 'default' case (since not all classes in hierarchy are
//  operators with precedence).
//  
auto getPrecedence(const RegexExpr& node) -> int
{
    return josa::visitor::dispatcher<RegexHierarchy>::match(node)
    (
        [](const Complement&)    { return -1; },
        [](const KleeneStar&)    { return -2; },
        [](const Concatenation&) { return -3; },
        [](const Intersection&)  { return -4; },
        [](const Union&)         { return -5; },

        //  Default case:
        [](const RegexExpr&)     { return 0; }          // [](auto){return 0;} would also work
    );
}

//--------------------------------------------------------------------------------------------------

struct RegexToString : josa::visitor::enable_dispatch<RegexToString, RegexHierarchy>
{
    auto operator () (const Union& node) const -> std::string
    {
        const auto s1 = getPrecedence(node) > getPrecedence(node.expr1()) ? "(" + visit(node.expr1()) + ")" : visit(node.expr1());
        const auto s2 = getPrecedence(node) > getPrecedence(node.expr2()) ? "(" + visit(node.expr2()) + ")" : visit(node.expr2());

        return s1 + "|" + s2;
    }

    auto operator () (const Intersection& node) const -> std::string
    {
        const auto s1 = getPrecedence(node) > getPrecedence(node.expr1()) ? "(" + visit(node.expr1()) + ")" : visit(node.expr1());
        const auto s2 = getPrecedence(node) > getPrecedence(node.expr2()) ? "(" + visit(node.expr2()) + ")" : visit(node.expr2());

        return s1 + "&" + s2;
    }

    auto operator () (const Concatenation& node) const -> std::string
    {
        const auto s1 = getPrecedence(node) > getPrecedence(node.expr1()) ? "(" + visit(node.expr1()) + ")" : visit(node.expr1());
        const auto s2 = getPrecedence(node) > getPrecedence(node.expr2()) ? "(" + visit(node.expr2()) + ")" : visit(node.expr2());

        return s1 + s2;
    }

    auto operator () (const EmptySet&) const -> std::string
    {
        return "#"s;
    }

    auto operator () (const EmptyString&) const -> std::string
    {
        return "()"s;
    }

    auto operator () (const KleeneStar& node) const -> std::string
    {
        const auto s = getPrecedence(node) > getPrecedence(node.expr()) ? "(" + visit(node.expr()) + ")" : visit(node.expr());
        return s + "*";
    }

    auto operator () (const Complement& node) const -> std::string
    {
        const auto s = getPrecedence(node) > getPrecedence(node.expr()) ? "(" + visit(node.expr()) + ")" : visit(node.expr());
        return "~" + s;
    }

    auto operator () (const Character& node) const -> std::string
    {
        return ""s + node.get();
    }
};

auto toString(const RegexExpr& rx) -> std::string
{
    return RegexToString{}.visit(rx);
}

auto toString(const RegexExprPtr& pRx) -> std::string
{
    return toString(*pRx);
}

//--------------------------------------------------------------------------------------------------

struct RegexClone : josa::visitor::enable_dispatch<RegexClone, RegexHierarchy>
{
    auto operator () (const EmptySet&) const -> RegexExprPtr
    {
        return std::make_unique<EmptySet>();
    }

    auto operator () (const EmptyString&) const -> RegexExprPtr
    {
        return std::make_unique<EmptyString>();
    }

    auto operator () (const Character& node) const -> RegexExprPtr
    {
        return std::make_unique<Character>(node.get());
    }

    auto operator () (const Concatenation& node) const -> RegexExprPtr
    {
        return std::make_unique<Concatenation>(visit(node.expr1()), visit(node.expr2()));
    }

    auto operator () (const Union& node) const -> RegexExprPtr
    {
        return std::make_unique<Union>(visit(node.expr1()), visit(node.expr2()));
    }

    auto operator () (const Intersection& node) const -> RegexExprPtr
    {
        return std::make_unique<Intersection>(visit(node.expr1()), visit(node.expr2()));
    }

    auto operator () (const Complement& node) const -> RegexExprPtr
    {
        return std::make_unique<Complement>(visit(node.expr()));
    }

    auto operator () (const KleeneStar& node) const -> RegexExprPtr
    {
        return std::make_unique<KleeneStar>(visit(node.expr()));
    }
};

auto clone(const RegexExpr& rx) -> RegexExprPtr
{
    return RegexClone{}.visit(rx);
}

auto clone(const RegexExprPtr& pRx) -> RegexExprPtr
{
    return clone(*pRx);
}

//--------------------------------------------------------------------------------------------------

struct RegexNullable : josa::visitor::enable_dispatch<RegexNullable, RegexHierarchy>
{
    auto operator () (const EmptySet&) const -> bool
    {
        return false;
    }

    auto operator () (const EmptyString&) const -> bool
    {
        return true;
    }

    auto operator () (const Concatenation& node) const -> bool
    {
        return visit(node.expr1()) && visit(node.expr2());
    }

    auto operator () (const Union& node) const -> bool
    {
        return visit(node.expr1()) || visit(node.expr2());
    }

    auto operator () (const KleeneStar&) const -> bool
    {
        return true;
    }

    auto operator () (const Intersection& node) const -> bool
    {
        return visit(node.expr1()) && visit(node.expr2());
    }

    auto operator () (const Complement& node) const -> bool
    {
        return !visit(node.expr());
    }

    auto operator () (const Character&) const -> bool
    {
        return false;
    }
};

auto isNullable(const RegexExpr& rx) -> bool
{
    return RegexNullable{}.visit(rx);
}

auto isNullable(const RegexExprPtr& pRx) -> bool
{
    return isNullable(*pRx);
}

//--------------------------------------------------------------------------------------------------

struct RegexDerivative : josa::visitor::enable_dispatch<RegexDerivative, RegexHierarchy>
{
    auto operator () (const EmptyString&, const char) const -> RegexExprPtr
    {
        return makeEmptySet();        
    }

    auto operator () (const EmptySet&, const char) const -> RegexExprPtr
    {
        return makeEmptySet();        
    }

    auto operator () (const Character& node, const char c) const -> RegexExprPtr
    {
        if (node.get() == c)
            return makeEmptyString();

        return makeEmptySet();
    }

    auto operator () (const Concatenation& node, const char c) const -> RegexExprPtr
    {
        if (isNullable(node.expr1()))
        {
            return makeUnion(
                makeConcatenation(visit(node.expr1(), c), clone(node.expr2())),
                visit(node.expr2(), c));
        }

        return makeConcatenation(visit(node.expr1(), c), clone(node.expr2()));
    }

    auto operator () (const Union& node, const char c) const -> RegexExprPtr
    {
        return makeUnion(visit(node.expr1(), c), visit(node.expr2(), c));
    }

    auto operator () (const Intersection& node, const char c) const -> RegexExprPtr
    {
        return makeIntersection(visit(node.expr1(), c), visit(node.expr2(), c));
    }

    auto operator () (const Complement& node, const char c) const -> RegexExprPtr
    {
        return makeComplement(visit(node.expr(), c));
    }

    auto operator () (const KleeneStar& node, const char c) const -> RegexExprPtr
    {
        return makeConcatenation(visit(node.expr(), c), clone(node));
    }
};

auto getDerivative(const RegexExpr& rx, const char c) -> RegexExprPtr
{
    return RegexDerivative{}.visit(rx, c);
}

auto getDerivative(const RegexExprPtr& pRx, const char c) -> RegexExprPtr
{
    return getDerivative(*pRx, c);
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("example-regex")
{
    const auto r = "(one|two|three|four|five)*END"_rx;
    const auto d1 = getDerivative(r,'t');

    CHECK(toString(d1) == "(wo|hree)(one|two|three|four|five)*END");

    const auto d2 = getDerivative(d1,'w');

    CHECK(toString(d2) == "o(one|two|three|four|five)*END");

    const auto d3 = getDerivative(r,'E');

    CHECK(toString(d3) == "ND");
}

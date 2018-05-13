# expr
A mathematical expressions parser written in C++17

`expr` is a simple library to get a function or a result from a string representing a mathematical
function.

The first case is to get a function from a string. It is possible to achieve this in two ways:
using an `expression` or the `parse_function` function.
```cpp
expr::expression F{"x^2+a*x+1/sin(x)"};
auto f = F.set_param('a', 2).as_unary();

auto g = expr::expression{"x^2+2*x+1/sin(x)"};
assert(f(1) == g(1));
```
Parsing a string this way internally creates a binary tree where each node has a value - a constant,
a parameter or a function with one or two leaves.
An expression like `3*2+5*x` will be saved as-is; and each call to the new functor will calculate
each time every operation. If you need to use the function a lot of times, you should optimize it
during the creation:
```cpp
expr::expression F{"3*2+5*x"};  //(+ (* 3 2) (* 5 x))
F.optimize();                   //(+ 6 ([*5] x))

F.build(expr::expression::policy::optimize, "sin(abs(x)) + 6*4/2"); //([+12] ([sin°abs] x))
f = F.as_unary();

auto g = expr::parse_function("55*x+cos(pi)"); //([+(-1)] ([55*] x)
```
An optimization calculates every numeric operation and combines as many functions as possible,
reducing the depth of the tree and so the number of operations to do in a single calculation.

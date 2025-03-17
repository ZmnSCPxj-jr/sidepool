First things first, do not be a jerk.

Rules:

1. No bare `new`, no bare `delete`.
   - Just RAII.
   - Use `std::make_unique`.
     `std::make_shared` if you really have to (e.g.
     passing a lambda that captures it to a
     `.then()` function in `Sidepool::Io` code).
   - Even if you will give the object to C anyway,
     *first* use `std::make_unique` *then* use
     `.release()` to the C code, this is safer
     under future refactorings.
   - Remember to `std::unique_ptr<>()` ASAP when
     taking the pointer back from C code.
   - Instead of `delete`ing pointers taken from C
     code in the `free` function of C-exposed
     interfaces, feed it to a temporary
     `std::unique_ptr<>()`, which does the same but
     enforces "no bare `delete`".
   - Overload `operator new` and `operator delete`
     is fine in test code that is supposed to test
     for memleaks (and just does something like
     increment/decrement number-of-allocations
     counters), but is not acceptable anywhere else.
2. No static-lifetime non-POD objects.
3. Everything must be in `namespace Sidepool`
   except the C interface in `libsidepool.h`,
   where everything should be prefixed
   `libsidepool_` instead.
4. `Sidepool::Io` is awesome, use it as much as
   possible.
   - Gets you tail-call-optimization for free
     (if you remember to yield LOL).
     - So yes make sure to pass `Sidepool::Idler&`
       to everything that will need
       tail-call-optimization (i.e. everything).
   - Async everywhere is awesome, just ask Scheme
     nerds about `call-with-current-continuation`.
     We call it "`Sidepool::Io` constructor" in
     these parts.
   - Did you know you can just `operator+` two
     `Sidepool::Io<void>`s?
     It even works with `operator+=`.
5. Every `.cpp` file must start with
   `#ifdef HAVE_CONFIG_H #include"config.h" #endif`.
6. For both `.cpp` and `.hpp`, include our own
   headers first, in ASCII order (capital `Z`
   before small `a`), with `#include"foo.hpp"`,
   then system headers in ASCII order with
   `#include<cstdio>`.
7. In `.hpp`, prefer forward-declaration of
   our types over inclusion of the header defining
   them.
   - Unfortunately some C++ compilers will complain
     if you forward-declare `struct` as `class`
     or vice-versa, but it is what it is.
   - This rule massively speeds up compilation in
     practice, you should do this for all your C++
     projects.
8. Use pointer-to-implementation as much as
   possible.
   - i.e. declare a private `Impl` class in the
     `.hpp`, the actual class there is just a
     `private: std::unique_ptr<Impl> pimpl`, and
     delegate its methods to the implementation
     class that is fully defined in the `.cpp`.
     Note that you need to explicitly declare a
     destructor in the `.hpp`, and put an
     empty-bodied destructor in the `.cpp` after
     you have fully declared the `Impl` class.
   - This is another rule that massively speeds up
     compilation.
9. `auto var = Typename();`, not `Typename var;`.
   Yes `auto var = std::string("foo");`, not
   `std::string var = "foo";`.
   - This makes consistent use of `std::make_unique`
     much more natural.
10. Install `valgrind` on your system if possible.
   The tests use `valgrind` if they can and we
   really ought to have `valgrind`-safety.
   If you are using MacOS, sucks to be you, have
   you considered it is the year of the Linux
   desktop already.


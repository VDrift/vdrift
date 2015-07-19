File extensions
---------------

Header files have .h extensions. Source implementation files have .cpp extensions.

Code
----

VDrift code is formatted using all tabs and no spaces. The following example demonstrates VDrift's coding style.

    class EXAMPLECLASS
    {
    private:
            int data;
    public:
            EXAMPLECLASS();
            ~EXAMPLECLASS();
            void MemberFunction();
    };
    // Add the sum of the number 1 - 10 to data.
    void EXAMPLECLASS::MemberFunction()
    {
            // loop and add i to data  <-- Useless comment, should be avoided.
            for (int i = 1; i <= 10; i++)
            {
                    data += i;
            }
    }

Indentation and Naming
----------------------

Notice the use of ALL CAPS for class names. Function names should be presented in MixedCase?. Opening and closing curly brackets ({}) should be on their own line, and should not be indented. The enclosed statements however, should be indented.

Control statements should have one space between the statement and the left paren. There should be no padding inside of the parens unless it helps readability, use your judgement. Function calls should have no spaces between the function name and the left paren.

Commenting
----------

Do not overuse comments. Only comment code which has side effects, is not clear at first glance, or includes complex operations. It is OK to comment a section of code with a description of what that section does.

It is a good idea to comment most non-trivial classes, methods, and instance variables. When commenting code, keep in mind that using [Doxygen](http://doxygen.org/) style comments will help to generate better documenation. See [Source code documentation](Source_code_documentation.md) for more information.

Inheritance
-----------

Inheritance is generally discouraged *except* in simple cases where the base class is abstract and there is only one level of inheritance.

In general, excessive use of inheritance compromises code readability and could be easily fixed by using a member variable instead of inheritance. For example, instead of making CAR inherit from ENGINE, the CAR should contain an ENGINE member.

Coupling/Dependency
-------------------

Coupling (or dependency) is the degree to which each program module relies on each one of the other modules. Coupling should be eliminated between unrelated modules. De-coupling modules leads to greater code cohesion, and high cohesion is associated with several desirable traits of software including robustness, reliability, reusability, and understandability whereas low cohesion is associated with undesirable traits such as being difficult to maintain, difficult to test, difficult to reuse, and even difficult to understand.

Program module (class) dependencies should be structured in a tree form, where higher level modules know about lower level modules, but lower level modules don't know about each other. For example, if there is a GAME module that contains a RENDERER module and a SETTINGS module, the code inside the GAME module should query the SETTINGS module and then initialize the RENDERER appropriately. That is, the GAME module would tell the SETTINGS module to go read the configuration file, ask the SETTINGS class "what's the display resolution supposed to be? how many bits per pixel? etc", and then make its calls to the RENDERER saying "okay, set up a display with this resolution, this many bits per pixel, etc". The RENDERER should *not* know anything about the SETTINGS module and should not access it. The advantage is that now the RENDERER doesn't need to know anything about a SETTINGS subsystem (and vice versa). If the SETTINGS subsystem is later rewritten, it doesn't (and shouldn't ever) cause any changes to code in the RENDERER. The RENDERER is now decoupled from the SETTINGS subsystem, and can now be re-used in a different project with a completely different SETTINGS subsystem. Decoupling also makes thread-safe coding easy.

Globals and singletons cause excessive coupling due to their global-access properties and should be completely avoided.

Namespaces
----------

Classes are declared in the global namespace, but can also be put in custom namespaces if appropriate.

No include file should pollute the global namespace with a "using" directive. That is, lines such as "using namespace std" should not occur in any header (.h) file. All "using" directives should be put into the implementation source files (.cpp). This is because any source file that includes a header with a using directive will have its global namespace unexpectedly polluted in whatever way the header file specifies.

For "using" directives in .cpp files, specific "using" declarations are preferred to "using namespace" declarations. That is, prefer "using std::string", "using std::endl", etc to "using namespace std". This is for readability.

Testing
-------

Unit testing is encouraged. VDrift comes with the [QuickTest](http://quicktest.sourceforge.net) unit testing framework. To use, simply \#include "unittest.h" in your source code implementation files.

<Category:Development>

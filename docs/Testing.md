VDrift includes a very simple unit testing framework for C++ code. It is derived from [QuickTest](http://quicktest.sourceforge.net/).

Running the Tests
-----------------

Unit tests are compiled by default. To execute run:

* Ubuntu: `build/vdrift -test`
* macOS: `build/vdrift.app/Contents/MacOS/vdrift -test`

### Results

The results are written to STDOUT. An example:

    [-------------- RUNNING UNIT TESTS --------------]
    src/matrix4.cpp(26): 'matrix4_test' FAILED: value1 (1) should be close to value2 (0)
    src/matrix4.cpp(27): 'matrix4_test' FAILED: value1 (10) should be close to value2 (20)
    src/matrix4.cpp(28): 'matrix4_test' FAILED: value1 (-1.19209e-07) should be close to value2 (-1)
    src/matrix4.cpp(33): 'matrix4_test' FAILED: value1 (1) should be close to value2 (0)
    src/matrix4.cpp(34): 'matrix4_test' FAILED: value1 (10) should be close to value2 (0)
    src/matrix4.cpp(35): 'matrix4_test' FAILED: value1 (-1.19209e-07) should be close to value2 (1)
    Results: 29 succeeded, 1 failed
    [-------------- UNIT TESTS FINISHED -------------]

Writing New Tests
-----------------

Consult the [QuickTest How to Use It](http://quicktest.sourceforge.net/usage.html) and the [QuickTest API Reference](http://quicktest.sourceforge.net/api.html) for details on how to write unit tests using QuickTest.

### Example Tests

To look at some example test code already in VDrift, look at **src/\*.cpp** files which contain the macro `QT_TEST`.

<Category:Development>

Many of VDrift's text-based data and configuration files are written in a format native to the game. The format is designed to be simple and easy to read, while flexibly storing many different kinds of data. It allows settings to be categorized into sections.

This format is defined by the *CONFIG* class in **include/config.h** and **src/config.cpp**.

Features
--------

### Encoding

Files in this format are interpreted as plain ASCII text. UTF-8 is not yet supported.

### Include directives

Lines that start with **include** will load sections and settings from another file written in the format. The file may be named using a relative path, which follows the **include** keyword and a single space. If the same section exists in both files, a union of the two sections will result in the file which had the include directive. Values from the including file override those in the included file. Circular includes are repressed.

### Comments and Whitespace

Comments begin with **\#** and can occur at any point in a line. Everything following the **\#** is ignored until the end of the line.

Whitespace before and after an identifier, name, or value is ignored; however, all spaces are preserved inside both section and setting identifiers. Line breaks are significant -- each line may contain only one section heading, **include** directive, or setting.

### Sections

Sections (categories) are defined by a heading line with an identifier only. The section identifier may be placed between optional **\[** and **\]** brackets. Identifiers are case-sensitive.

Sections are categorical (flat), not hierarchical (nested). It is possible to create a category hierarchy by carefully naming each section, but the format does not provide any mechanism for managing or traversing a hierarchy.

Settings included in a section are all those on lines following the section heading, until the next section heading or the end of the file.

### Settings

Settings (data items) are defined by a line with the form ***name* = *value***, where *name* is the identifier for the setting and *value* is the data for the setting. No restrictions are placed on the type, length, or formatting of the value stored in a setting. Setting identifiers are case-sensitive.

Settings before the first section heading have no section. They are referenced by their identifiers preceded by a **.** character.

#### Value types

The format employs a somewhat loose typing scheme. Settings do not need to have their types explicitly defined. In the game code, an item can be queried as any type, and depending on the value of the item, certain values can be interpreted differently when requested as different types.

Values can be requested as any of the following:

-   booleans
-   integer numbers
-   floating point numbers
-   strings
-   lists

For instance, the item **2nd.now** from the example above would have the value *1* if interpreted as an integer. If interpreted as a boolean, its value would be *true*; as a string, its value would be *"1"*; as a floating-point number, its value would be *1.0*.

#### Booleans

The following values will equal *true* and *false*, respectively, when interpreted as boolean values:

-   **true**, **false**
-   **yes**, **no**
-   **on**, **off**
-   **1**, **0**

#### Lists

List settings may be defined by writing a list of values separated by commas instead of just a single value.

Example
-------

A file in this format might look like this.

`name = Example`
`[ first ]`
`stuff = 567`
`blah = hello`
`radius = 0.555`
`[ 2nd ]`
`beans = on`
`now = 1`
`position = 5,6,7`

In the file, there are two categories, each with three data items. These items would be referenced by the following identifier strings, respectively:

-   **.name**
-   **first.stuff**
-   **first.blah**
-   **first.radius**
-   **2nd.beans**
-   **2nd.now**
-   **2nd.position**

<Category:Files>

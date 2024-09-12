# inxlib

Data structures and program control

# inxflow

TODO: planned library, not created yet

Basic data function control flow framework.
Allows for loading/storing files into user-defined structures,
and passing them to multiple defined commands that run in a script
like manner.

## Argument syntax

The command line takes arguments as follows:

	<exec>[<sep> <exec>]...

	<sep>: + | ++

	<exec>: <cmd>*

	<cmd>: -{cmd_name} {args}

The command line results in a set of exec groups split by `+|++`.
The `+` is only needed for function with no set number of arguments,
it is implicit otherwise.
The `++` separator splits between local groups, effecting what is kept.

## Data

Data can be referenced in arguments. All data will be contained in `@` delimits,
the final `@` may be omitted. Use `@@` to escape the character, and an argument
ending in a single `@` will end in the `@` character.

The syntax is defined below:

	@[op]?[group!]?[varname]@

The `group` defines what the data is managed by. There is no default group,
but certain arguments may default or expect data in a certain group.
Inbuilt groups are `var` and `file`, `var` storing single-line strings and
`file` storing binary files in memory.

The `op` defines an operation to perform on the data.
By default, this string is simply left as is, but the command interpreter
can refer to it to look up the data.
If `op` is `%`, then convert the data string at the command line level (print).
If no group is specified, then defaults to `var`.
E.g. `@%name@` will be replaced with the value in `var!name`.

The `varname` is the variable name inside the group.  Use of whitespace is discouraged,
and no `@$!` characters are permitted, otherwise any ASCII is allowed.
Varname is allowed to start with `$`, in which case it is a local variable, and will be
discarded upon a `++` separator.

Some commands expect data in an argument, in which case the `@` are optional.
Though `@` may be used to distinguish between data loaded in program vs data stored
in a file to be loaded by command, so using `@` is encouraged.

## Commands

-D {name} {value}
Stores var `name` with `value` string. Arg `name` is forced to be `var` grpup.

-L {name} {filename}
Load or deserialise data from `filename` and store in `name` (default `file` group).
Arg `filename` is a string, which will be a filename that loads the file into data
correctly.
Filenames stored in a `var` can be passed using the command line print syntax, e.g. `@%fname@`.

-S {name} {dest}
Save or serialise data from `name` to `dest`.
Arg `dest` is either a string, which is treated as a filename, or some other data.
When a data is given, will serialise (the same as -L) as a memory mapped file and pass to `name`
to deserialise as a `std::istream`.
If the user want to make a copy function between data types directly, use a user command instead.

-M rm|mv|cp {name} {name2}?
Either remove (rm) `name` (requires group), rename (mv) to `name2` (group optional, must match `name`),
or copies to `name2` if supported (group optional, must match `name`).
If no copy for a type was supplied, use -S. No `<sep>` are required for this command.

-C {command} {args}...
Runs a user command. As the number of arguments are not set, must always be delimited
by separators.
It is recommended to namespace command names in C++ styles, e.g. `lib1::start`, `lib1::run`, `lib2::run`.
Commands to resolve `@%` print statements, so it is recommended to avoid `@` characters in command name,
as the final string is taken as is.

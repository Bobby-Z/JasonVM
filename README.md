JasonVM
=======

JasonVM is a virtual machine that executes Jason code. An example HelloWorld program in Jason would look like this:
```
//HelloWorld.jason
{
	"public HelloWorld": new function(String[] args) //Application entry point
	{
		Console.Out.PrintLine("Hello world!!"), //Prints out a line to the output stream
		return 0 //Returns 0, meaning that this program exited without errors.
	}
}
```
This code doesn't need to be compiled, as it is compiled by a JIT compiler.

The Jason programming language is based on the Javascript Object Notation (hence the name "Jason"),
and is an extension to it, as it adds programmable functionality and more data types (e.g. functions, ints, floats, doubles, shorts, bytes, characters, strings, etc.).

This language was made to simplify UI creation, as it lets you create a hierchical structure, and you don't need to ever compile it: it is compiled during runtime.

###Syntax###
To see the language syntax definitions, go to the [JasonVM webpage](http://bobby-z.github.io/JasonVM/).

###Examples###
All of these examples are free to use for whatever purpose you want without giving any credit to me.
```
Example #1: InstantMessenger.jason
//WIP
```

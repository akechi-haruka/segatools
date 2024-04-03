additional seperate "extension" dlls that are needed for weird ass games that access dll functions via ordinal, so you can't combine everything

keep code to an absolute minimum or stuff will clash

confirmed functions you shouldn't use INSIDE of ex-dlls:

* dll_hook_push (use in caller after LoadLibrary)

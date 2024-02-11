program written in raw AVR C language, but written in arduino IDE due to DS3231 library compatibility.
the library is written in C++, therefore a pure C compilator is unable to use it.

the program uses domestic arduino lcd and wire(i2c) libraries. in case of their absense,
use libraries inside this folder.

DS3232 library is a non official library found on the web

calendar alarm functions have been removed for the specific requirements of the project. but after
a few small modifications in code can be used

alarm function is a program, not hadware function. the function interacts with the DS3231 to do its work.
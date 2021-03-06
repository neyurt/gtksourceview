// Regular expressions:
/abc/
x = /abc/;
function_with_regex_arg(/abc/);
[ /abc/, /def/];
{ regex: /abc/ };
(1 === 0) ? /abc/ : /def/;
/abc/ /* Comment */
/abc/ // Comment
var matches = /abc/.exec('Alphabet ... that should contain abc, right?');

// No regex here: 
a = [thing / thing, thing / thing];
x = a /b/ c / d;

// Character groups with backslashes:
/[ab\\]/ // a, b or backslash
/[ab\]]/ // a, b or ]
/\\[ab]/ // a or b preceded by backslash
/\[ab]/  // Literally "[ab]"

// Escape sequences:
'\b\f\n\r\t\v\0\'\"\\' // Single character escape
"\1\01\001"            // Octal escape
'\xA9'                 // Hexadecimal escape
"\u00a9"               // Unicode escape
'\u{1D306}'            // Unicode code point escape
/\cJ/                  // Control escape

// ES2015 binary and octal numbers:
let binary1 = 0b1010;
let binary2 = 0B00001111;
let octal1 = 0o0123;
let octal2 = 0O4567;

// Template strings
// ----------------
// Template strings are delimited by back-ticks (grave accent) and
// can span multiple lines. They allow for expressions that inside
// of a dollar-sign plus curly-bracket construct (${...}).

console.log(`The sum of 2 and 2 is ${2 + 2}`);

let y = 8;
let my_string = `This is a multiline
string that also contains
a template ${y + (4.1 - 2.2)}`;

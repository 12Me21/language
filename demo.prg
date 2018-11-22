'this is a comment
'(specifically chosen to annoy people who use apostrophes for strings)

'declaring variables:
var x
var y = 1

'literals
x = 1
x = true
'x = None
x = "string" 'strings and arrays are mutable and can be resized etc.
x = {0 = "zero", "0" = "different zero"} 'table
x = [1, 2, 3] '0 indexed, of course
x = def()
	return 7
end

'line breaks are allowed in certain cases:
?1+
1
?[
	1,
	2,
	3, 'allowed I guess... (not sure about this as it might hide errors)
]

'semicolons can be used to separate statements but are usually unnessesary:
?x;?y
?x?y

'lists:
?1,2,3 ' ? is "print"
?((1,2,3,4,5))

'functions:
var swap = def(a,b)
	return b,a
end
?swap(2,1)
'x, y = y, x

'(no builtin functions have been implemented yet)

var long_variable_name = 0
long_variable_name = long_variable_name + 1 'add 1
'@ refrences the value of the variable that is being assigned to:
long_variable_name = @ + 1 'add 1
long_variable_name = -@ 'negate
long_variable_name = 4 / (@ + 2) - 3

'constraints:
var z{z != 3} 'an error is thrown if z has a value of 3
'z = 3 'error
'var positive{@ > 0} = 1 '@ works like in =, and represents the new value (because constraints are checked after assignment)
'@ doesn't work this way (YET)
'positive = -1 'error

'control statements:
if x!=2 then x=2 endif
while x<10
	x=@+1
wend
repeat
	x=@-1
until x==0
'if x==0 then x=1 elseif x==1 then x=-1 else x=3 endif
'for hasn't been implemented yet oops

'operators:
'(most haven't been implemented yet)
'^ is exponentiation
'~ is bitwise not and bitwise xor (~a vs. a~b, like with subtraction and negation)
'? is print
'! is logical not
'and/or are logical and/or
'\ is floored division
'% is modulus (a%b = a-a\b*b)

' ` will never be used for anything
z=3
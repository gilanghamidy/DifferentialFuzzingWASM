# QuadJump

## Definition
QuadJump is a list of integers that satisfy the sequence as follow:

a, b, n > 0 

If n is even
```
a² + b, (a + 2)² + b, ... , (a + n - 2)² + b , (a + n)² + b, (a + n - 1)² + b, ... , (a + 3)² + b, (a + 1)² + b
```
If n is odd

```
a² + b, (a + 2)² + b, ... , (a + n - 1)² + b, (a + n)² + b, (a + n - 2) + b, ... , (a + 3)² + b, (a + 1)² + b
```

## Example
If a = 2, b = 1 and n = 5, then the list below satisfy QuadJump

```
5, 17, 37, 50, 26, 10
```

If a = 3, b = 5 and n = 6 then the list below satisfy QuadJump

```
14, 30, 54, 86, 69, 41, 21
```

## Problem Statement

Create a function CheckQuadJump that if given a list of non-negative integer, it returns True if it satisfies QuadJump, and False otherwise

### Test Case 1
#### Input
```
5 17 37 50 26 10
```
#### Output
```
True
```
### Test Case 2
#### Input
```
5 17 37 50 27 10
```
#### Output
```
False
```

### Test Case 3
#### Input
```
1 9 25 36 16 4
```
#### Output
```
True
```

